#ifndef BOTIEYES_NET_COMMAND_CODEC_H
#define BOTIEYES_NET_COMMAND_CODEC_H

// CommandCodec — pure, platform-agnostic encode/decode of BotiEyes network
// protocol frames. No Arduino/Wi-Fi dependencies; host-unit-testable.
//
// See contracts/network-protocol.md and data-model.md.

#include <stdint.h>
#include <stddef.h>
#include "NetProtocol.h"

namespace BotiEyes {
namespace net {

// A decoded command. Only the fields relevant to `opcode` are meaningful.
struct Command {
    uint8_t  version;
    uint8_t  opcode;
    uint32_t seq;

    // Emotion (OP_SET_EMOTION)
    int16_t  valenceMilli;   // -500..+500
    uint16_t arousalMilli;   // 0..1000
    // Gaze (OP_SET_GAZE)
    int16_t  gazeH;          // -90..+90
    int16_t  gazeV;          // -45..+45
    // Shared transition duration (emotion/gaze) and animation duration
    uint16_t durationMs;
    // Preset (OP_SET_PRESET)
    uint8_t  presetId;       // 0..12
    uint8_t  intensityPct;   // 0..100
    // Wink (OP_WINK)
    uint8_t  eye;            // 0=left, 1=right
    // Idle (OP_SET_IDLE)
    uint8_t  idleEnable;     // 0/1
};

// Device status payload (encoded into OP_STATUS).
struct StatusReport {
    int16_t  valenceMilli;
    uint16_t arousalMilli;
    int16_t  gazeH;
    int16_t  gazeV;
    uint8_t  idleEnabled;
    uint8_t  connectionHealthy;
    uint32_t uptimeMs;
};

class CommandCodec {
public:
    // Decode a received datagram into `out`.
    // Returns REASON_OK on success, or an error reason (BAD_LENGTH,
    // BAD_VERSION, OUT_OF_RANGE, UNKNOWN_OPCODE). On any error, `out` must not
    // be treated as valid and device state must remain unchanged.
    static ReasonCode decode(const uint8_t* buf, size_t len, Command& out);

    // Encode an ACK response. `buf` must hold at least SIZE_ACK bytes.
    // Returns the number of bytes written.
    static size_t encodeAck(uint32_t ackSeq, bool accepted, uint8_t reason,
                            uint8_t* buf);

    // Encode a STATUS response. `buf` must hold at least SIZE_STATUS bytes.
    // Returns the number of bytes written.
    static size_t encodeStatus(const StatusReport& status, uint8_t* buf);

    // True if this opcode is a best-effort streaming update (no ACK, newest-wins).
    static bool isStreaming(uint8_t opcode) {
        return opcode == OP_SET_EMOTION || opcode == OP_SET_GAZE;
    }
};

} // namespace net
} // namespace BotiEyes

#endif // BOTIEYES_NET_COMMAND_CODEC_H
