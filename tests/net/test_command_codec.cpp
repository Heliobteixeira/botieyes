// Host-native tests for CommandCodec (US1/US4/US5; FR-007, FR-014, SC-007).

#include "test_harness.h"
#include "CommandCodec.h"
#include "NetProtocol.h"

#include <cstring>

using namespace BotiEyes::net;

// --- helpers ---
static void putHeader(uint8_t* b, uint8_t op, uint32_t seq) {
    b[0] = PROTOCOL_VERSION;
    b[1] = op;
    b[2] = (uint8_t)(seq & 0xFF);
    b[3] = (uint8_t)((seq >> 8) & 0xFF);
    b[4] = (uint8_t)((seq >> 16) & 0xFF);
    b[5] = (uint8_t)((seq >> 24) & 0xFF);
}
static void putI16(uint8_t* p, int16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)(((uint16_t)v >> 8) & 0xFF);
}
static void putU16(uint8_t* p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

// --- SET_EMOTION round-trip ---
static void test_set_emotion_roundtrip() {
    uint8_t b[SIZE_SET_EMOTION];
    putHeader(b, OP_SET_EMOTION, 42);
    putI16(b + 6, 350);    // valence +0.350
    putU16(b + 8, 550);    // arousal 0.550
    putU16(b + 10, 400);   // duration
    Command c;
    CHECK_EQ(CommandCodec::decode(b, sizeof(b), c), REASON_OK);
    CHECK_EQ(c.opcode, OP_SET_EMOTION);
    CHECK_EQ(c.seq, 42u);
    CHECK_EQ(c.valenceMilli, 350);
    CHECK_EQ(c.arousalMilli, 550);
    CHECK_EQ(c.durationMs, 400);
    CHECK(CommandCodec::isStreaming(OP_SET_EMOTION));
}

// --- out-of-range valence rejected (no clamping) ---
static void test_emotion_out_of_range() {
    uint8_t b[SIZE_SET_EMOTION];
    putHeader(b, OP_SET_EMOTION, 1);
    putI16(b + 6, 900);    // > +500
    putU16(b + 8, 550);
    putU16(b + 10, 400);
    Command c;
    CHECK_EQ(CommandCodec::decode(b, sizeof(b), c), REASON_OUT_OF_RANGE);

    putI16(b + 6, 100);
    putU16(b + 8, 2000);   // arousal > 1000
    CHECK_EQ(CommandCodec::decode(b, sizeof(b), c), REASON_OUT_OF_RANGE);
}

// --- truncated and oversized frames rejected ---
static void test_bad_length() {
    uint8_t b[SIZE_SET_EMOTION];
    putHeader(b, OP_SET_EMOTION, 1);
    putI16(b + 6, 0); putU16(b + 8, 500); putU16(b + 10, 400);
    Command c;
    // Truncated: claims SET_EMOTION but only header bytes present.
    CHECK_EQ(CommandCodec::decode(b, HEADER_SIZE, c), REASON_BAD_LENGTH);
    // Sub-header.
    CHECK_EQ(CommandCodec::decode(b, 3, c), REASON_BAD_LENGTH);
    // Oversized (> MAX_FRAME_SIZE).
    uint8_t big[64];
    putHeader(big, OP_HEARTBEAT, 1);
    CHECK_EQ(CommandCodec::decode(big, sizeof(big), c), REASON_BAD_LENGTH);
}

// --- bad version + unknown opcode ---
static void test_bad_version_and_opcode() {
    uint8_t b[SIZE_HEARTBEAT];
    putHeader(b, OP_HEARTBEAT, 1);
    b[0] = 99;  // wrong version
    Command c;
    CHECK_EQ(CommandCodec::decode(b, sizeof(b), c), REASON_BAD_VERSION);

    putHeader(b, 0x7F, 1);  // unknown opcode
    CHECK_EQ(CommandCodec::decode(b, sizeof(b), c), REASON_UNKNOWN_OPCODE);
}

// --- preset / gaze / blink / wink / idle (US4) ---
static void test_discrete_opcodes() {
    Command c;

    uint8_t preset[SIZE_SET_PRESET];
    putHeader(preset, OP_SET_PRESET, 5);
    preset[6] = 0;    // happy
    preset[7] = 100;  // intensity
    CHECK_EQ(CommandCodec::decode(preset, sizeof(preset), c), REASON_OK);
    CHECK_EQ(c.presetId, 0);
    CHECK_EQ(c.intensityPct, 100);
    preset[6] = 99;   // invalid preset id
    CHECK_EQ(CommandCodec::decode(preset, sizeof(preset), c), REASON_OUT_OF_RANGE);

    uint8_t gaze[SIZE_SET_GAZE];
    putHeader(gaze, OP_SET_GAZE, 6);
    putI16(gaze + 6, -45); putI16(gaze + 8, 30); putU16(gaze + 10, 300);
    CHECK_EQ(CommandCodec::decode(gaze, sizeof(gaze), c), REASON_OK);
    CHECK_EQ(c.gazeH, -45);
    CHECK_EQ(c.gazeV, 30);
    putI16(gaze + 6, -120);  // out of range
    CHECK_EQ(CommandCodec::decode(gaze, sizeof(gaze), c), REASON_OUT_OF_RANGE);

    uint8_t wink[SIZE_WINK];
    putHeader(wink, OP_WINK, 7);
    wink[6] = 1; putU16(wink + 7, 150);
    CHECK_EQ(CommandCodec::decode(wink, sizeof(wink), c), REASON_OK);
    CHECK_EQ(c.eye, 1);
    wink[6] = 5;  // invalid eye
    CHECK_EQ(CommandCodec::decode(wink, sizeof(wink), c), REASON_OUT_OF_RANGE);

    uint8_t idle[SIZE_SET_IDLE];
    putHeader(idle, OP_SET_IDLE, 8);
    idle[6] = 1;
    CHECK_EQ(CommandCodec::decode(idle, sizeof(idle), c), REASON_OK);
    CHECK_EQ(c.idleEnable, 1);
    idle[6] = 2;  // invalid
    CHECK_EQ(CommandCodec::decode(idle, sizeof(idle), c), REASON_OUT_OF_RANGE);
}

// --- ACK / STATUS encode (US5) ---
static void test_encode_ack_status() {
    uint8_t b[SIZE_STATUS];
    size_t n = CommandCodec::encodeAck(123, false, REASON_IN_USE, b);
    CHECK_EQ(n, (size_t)SIZE_ACK);
    CHECK_EQ(b[1], OP_ACK);
    // ackSeq little-endian at offset 6
    uint32_t ackSeq = (uint32_t)b[6] | ((uint32_t)b[7] << 8) |
                      ((uint32_t)b[8] << 16) | ((uint32_t)b[9] << 24);
    CHECK_EQ(ackSeq, 123u);
    CHECK_EQ(b[10], 0);            // accepted=false
    CHECK_EQ(b[11], REASON_IN_USE);

    StatusReport s;
    s.valenceMilli = -200; s.arousalMilli = 800;
    s.gazeH = -45; s.gazeV = 15;
    s.idleEnabled = 1; s.connectionHealthy = 1; s.uptimeMs = 123456;
    n = CommandCodec::encodeStatus(s, b);
    CHECK_EQ(n, (size_t)SIZE_STATUS);
    CHECK_EQ(b[1], OP_STATUS);
}

int main() {
    RUN(test_set_emotion_roundtrip);
    RUN(test_emotion_out_of_range);
    RUN(test_bad_length);
    RUN(test_bad_version_and_opcode);
    RUN(test_discrete_opcodes);
    RUN(test_encode_ack_status);
    TEST_MAIN_END();
}
