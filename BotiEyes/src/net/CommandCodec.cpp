// CommandCodec implementation — pure, platform-agnostic.

#include "CommandCodec.h"

namespace BotiEyes {
namespace net {

namespace {

// Little-endian readers (portable; no alignment/endianness assumptions).
inline uint16_t rdU16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
inline int16_t rdI16(const uint8_t* p) {
    return (int16_t)rdU16(p);
}
inline uint32_t rdU32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Little-endian writers.
inline void wrU16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}
inline void wrI16(uint8_t* p, int16_t v) { wrU16(p, (uint16_t)v); }
inline void wrU32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

inline size_t expectedSize(uint8_t opcode) {
    switch (opcode) {
        case OP_ACQUIRE:      return SIZE_ACQUIRE;
        case OP_RELEASE:      return SIZE_RELEASE;
        case OP_HEARTBEAT:    return SIZE_HEARTBEAT;
        case OP_SET_EMOTION:  return SIZE_SET_EMOTION;
        case OP_SET_PRESET:   return SIZE_SET_PRESET;
        case OP_SET_GAZE:     return SIZE_SET_GAZE;
        case OP_BLINK:        return SIZE_BLINK;
        case OP_WINK:         return SIZE_WINK;
        case OP_SET_IDLE:     return SIZE_SET_IDLE;
        case OP_QUERY_STATUS: return SIZE_QUERY_STATUS;
        default:              return 0;  // unknown
    }
}

} // namespace

ReasonCode CommandCodec::decode(const uint8_t* buf, size_t len, Command& out) {
    // Reject oversized or sub-header frames up front.
    if (len < HEADER_SIZE || len > MAX_FRAME_SIZE) {
        return REASON_BAD_LENGTH;
    }

    const uint8_t version = buf[0];
    const uint8_t opcode  = buf[1];
    if (version != PROTOCOL_VERSION) {
        return REASON_BAD_VERSION;
    }

    const size_t need = expectedSize(opcode);
    if (need == 0) {
        return REASON_UNKNOWN_OPCODE;
    }
    if (len != need) {
        return REASON_BAD_LENGTH;
    }

    out.version = version;
    out.opcode  = opcode;
    out.seq     = rdU32(buf + 2);

    const uint8_t* p = buf + HEADER_SIZE;  // payload start
    switch (opcode) {
        case OP_ACQUIRE:
        case OP_RELEASE:
        case OP_HEARTBEAT:
        case OP_QUERY_STATUS:
            break;  // no payload

        case OP_SET_EMOTION: {
            out.valenceMilli = rdI16(p);
            out.arousalMilli = rdU16(p + 2);
            out.durationMs   = rdU16(p + 4);
            if (out.valenceMilli < VALENCE_MIN_MILLI ||
                out.valenceMilli > VALENCE_MAX_MILLI ||
                (int16_t)out.arousalMilli < AROUSAL_MIN_MILLI ||
                (int16_t)out.arousalMilli > AROUSAL_MAX_MILLI) {
                return REASON_OUT_OF_RANGE;
            }
            break;
        }

        case OP_SET_PRESET: {
            out.presetId     = p[0];
            out.intensityPct = p[1];
            if (out.presetId >= PRESET_COUNT || out.intensityPct > 100) {
                return REASON_OUT_OF_RANGE;
            }
            break;
        }

        case OP_SET_GAZE: {
            out.gazeH      = rdI16(p);
            out.gazeV      = rdI16(p + 2);
            out.durationMs = rdU16(p + 4);
            if (out.gazeH < GAZE_H_MIN || out.gazeH > GAZE_H_MAX ||
                out.gazeV < GAZE_V_MIN || out.gazeV > GAZE_V_MAX) {
                return REASON_OUT_OF_RANGE;
            }
            break;
        }

        case OP_BLINK: {
            out.durationMs = rdU16(p);
            break;
        }

        case OP_WINK: {
            out.eye        = p[0];
            out.durationMs = rdU16(p + 1);
            if (out.eye > 1) {
                return REASON_OUT_OF_RANGE;
            }
            break;
        }

        case OP_SET_IDLE: {
            out.idleEnable = p[0];
            if (out.idleEnable > 1) {
                return REASON_OUT_OF_RANGE;
            }
            break;
        }

        default:
            return REASON_UNKNOWN_OPCODE;
    }

    return REASON_OK;
}

size_t CommandCodec::encodeAck(uint32_t ackSeq, bool accepted, uint8_t reason,
                               uint8_t* buf) {
    buf[0] = PROTOCOL_VERSION;
    buf[1] = OP_ACK;
    wrU32(buf + 2, 0);  // response seq unused
    wrU32(buf + HEADER_SIZE, ackSeq);
    buf[HEADER_SIZE + 4] = accepted ? 1 : 0;
    buf[HEADER_SIZE + 5] = reason;
    return SIZE_ACK;
}

size_t CommandCodec::encodeStatus(const StatusReport& s, uint8_t* buf) {
    buf[0] = PROTOCOL_VERSION;
    buf[1] = OP_STATUS;
    wrU32(buf + 2, 0);
    uint8_t* p = buf + HEADER_SIZE;
    wrI16(p,      s.valenceMilli);
    wrU16(p + 2,  s.arousalMilli);
    wrI16(p + 4,  s.gazeH);
    wrI16(p + 6,  s.gazeV);
    p[8] = s.idleEnabled ? 1 : 0;
    p[9] = s.connectionHealthy ? 1 : 0;
    wrU32(p + 10, s.uptimeMs);
    return SIZE_STATUS;
}

} // namespace net
} // namespace BotiEyes
