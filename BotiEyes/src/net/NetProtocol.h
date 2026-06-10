#ifndef BOTIEYES_NET_PROTOCOL_H
#define BOTIEYES_NET_PROTOCOL_H

// BotiEyes Network Control Protocol — shared wire constants (v1).
//
// This header is platform-agnostic (no Arduino/Wi-Fi dependencies) so it can be
// compiled and unit-tested on the host. See contracts/network-protocol.md.

#include <stdint.h>

namespace BotiEyes {
namespace net {

// Protocol version carried in every frame's first byte.
static const uint8_t PROTOCOL_VERSION = 1;

// Default UDP port the device listens on.
static const uint16_t DEFAULT_UDP_PORT = 4210;

// Frame layout: version(1) + opcode(1) + seq(4) + payload.
static const uint8_t  HEADER_SIZE = 6;
static const uint8_t  MAX_FRAME_SIZE = 32;

// Liveness / session timing (milliseconds).
static const uint32_t HEARTBEAT_INTERVAL_MS = 1000;  // controller → device
static const uint32_t SESSION_TIMEOUT_MS    = 5000;  // no-contact → release + idle

// Command opcodes (controller → device).
enum Opcode : uint8_t {
    OP_ACQUIRE      = 0x01,
    OP_RELEASE      = 0x02,
    OP_HEARTBEAT    = 0x03,
    OP_SET_EMOTION  = 0x10,
    OP_SET_PRESET   = 0x11,
    OP_SET_GAZE     = 0x12,
    OP_BLINK        = 0x13,
    OP_WINK         = 0x14,
    OP_SET_IDLE     = 0x15,
    OP_QUERY_STATUS = 0x20,
    // Responses (device → controller).
    OP_ACK          = 0x80,
    OP_STATUS       = 0x81
};

// Reason codes carried in an ACK response.
enum ReasonCode : uint8_t {
    REASON_OK             = 0,
    REASON_IN_USE         = 1,
    REASON_BAD_VERSION    = 2,
    REASON_BAD_LENGTH     = 3,
    REASON_OUT_OF_RANGE   = 4,
    REASON_UNKNOWN_OPCODE = 5
};

// Exact frame sizes per opcode (header + payload), in bytes.
static const uint8_t SIZE_ACQUIRE      = HEADER_SIZE;       // 6
static const uint8_t SIZE_RELEASE      = HEADER_SIZE;       // 6
static const uint8_t SIZE_HEARTBEAT    = HEADER_SIZE;       // 6
static const uint8_t SIZE_SET_EMOTION  = HEADER_SIZE + 6;   // 12: int16 v, uint16 a, uint16 dur
static const uint8_t SIZE_SET_PRESET   = HEADER_SIZE + 2;   // 8:  uint8 preset, uint8 intensity
static const uint8_t SIZE_SET_GAZE     = HEADER_SIZE + 6;   // 12: int16 h, int16 v, uint16 dur
static const uint8_t SIZE_BLINK        = HEADER_SIZE + 2;   // 8:  uint16 dur
static const uint8_t SIZE_WINK         = HEADER_SIZE + 3;   // 9:  uint8 eye, uint16 dur
static const uint8_t SIZE_SET_IDLE     = HEADER_SIZE + 1;   // 7:  uint8 enable
static const uint8_t SIZE_QUERY_STATUS = HEADER_SIZE;       // 6
static const uint8_t SIZE_ACK          = HEADER_SIZE + 6;   // 12: uint32 ackSeq, uint8 accepted, uint8 reason
static const uint8_t SIZE_STATUS       = HEADER_SIZE + 14;  // 20

// Fixed-point scales (floats are transmitted as scaled integers).
static const int16_t VALENCE_SCALE   = 1000;  // -0.5..+0.5 → -500..+500
static const int16_t AROUSAL_SCALE   = 1000;  //  0.0..1.0  → 0..1000
static const int16_t INTENSITY_SCALE = 100;   //  0.0..1.0  → 0..100

// Logical parameter limits (post-decode), matching the BotiEyes library.
static const int16_t VALENCE_MIN_MILLI = -500;
static const int16_t VALENCE_MAX_MILLI =  500;
static const int16_t AROUSAL_MIN_MILLI =  0;
static const int16_t AROUSAL_MAX_MILLI =  1000;
static const int16_t GAZE_H_MIN = -90;
static const int16_t GAZE_H_MAX =  90;
static const int16_t GAZE_V_MIN = -45;
static const int16_t GAZE_V_MAX =  45;
static const uint8_t PRESET_COUNT = 13;  // happy..neutral

} // namespace net
} // namespace BotiEyes

#endif // BOTIEYES_NET_PROTOCOL_H
