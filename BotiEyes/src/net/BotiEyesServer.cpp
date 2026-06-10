// BotiEyesServer — ESP32 UDP transport adapter implementation.

#ifdef ESP32

#include "BotiEyesServer.h"

#include <Arduino.h>   // millis()

namespace BotiEyes {
namespace net {

namespace {

// Apply a preset id (0..12) to the eyes at the given intensity.
// Order MUST match the controller's PRESETS list and PRESET_COUNT.
void applyPreset(BotiEyes& eyes, uint8_t presetId, float intensity) {
    switch (presetId) {
        case 0:  eyes.happy(intensity);     break;
        case 1:  eyes.sad(intensity);       break;
        case 2:  eyes.angry(intensity);     break;
        case 3:  eyes.calm(intensity);      break;
        case 4:  eyes.excited(intensity);   break;
        case 5:  eyes.tired(intensity);     break;
        case 6:  eyes.surprised(intensity); break;
        case 7:  eyes.anxious(intensity);   break;
        case 8:  eyes.content(intensity);   break;
        case 9:  eyes.curious(intensity);   break;
        case 10: eyes.thinking(intensity);  break;
        case 11: eyes.confused(intensity);  break;
        case 12: eyes.neutral(intensity);   break;
        default: break;  // already range-checked by the codec
    }
}

} // namespace

BotiEyesServer::BotiEyesServer(BotiEyes& eyes)
    : eyes_(eyes),
      port_(DEFAULT_UDP_PORT),
      started_(false),
      idleFallbackActive_(true),
      startMs_(0) {}

void BotiEyesServer::begin(uint16_t port) {
    port_ = port;
    udp_.begin(port_);
    started_ = true;
    startMs_ = millis();
    // Until a controller acquires the lock, run autonomous idle behavior.
    idleFallbackActive_ = true;
    eyes_.enableIdleBehavior(true);
}

void BotiEyesServer::poll() {
    if (!started_) {
        return;
    }
    const uint32_t nowMs = millis();

    // Drain every datagram currently buffered; never block waiting for more.
    int packetSize = udp_.parsePacket();
    while (packetSize > 0) {
        uint8_t buf[MAX_FRAME_SIZE];
        int toRead = packetSize;
        if (toRead > (int)sizeof(buf)) {
            toRead = (int)sizeof(buf);  // oversized → codec rejects on length
        }
        int n = udp_.read(buf, toRead);
        if (n > 0) {
            const uint32_t ip = (uint32_t)udp_.remoteIP();
            const uint16_t rport = udp_.remotePort();
            handleDatagram(buf, (size_t)n, ip, rport, nowMs);
        }
        packetSize = udp_.parsePacket();
    }
}

void BotiEyesServer::handleDatagram(const uint8_t* buf, size_t len,
                                    uint32_t ip, uint16_t port, uint32_t nowMs) {
    Command cmd;
    ReasonCode reason = CommandCodec::decode(buf, len, cmd);

    // Lifecycle opcodes are handled regardless of current lock holder.
    if (reason == REASON_OK && cmd.opcode == OP_ACQUIRE) {
        ReasonCode r = session_.onAcquire(ip, port, nowMs);
        sendAck(ip, port, cmd.seq, r == REASON_OK, r);
        return;
    }
    if (reason == REASON_OK && cmd.opcode == OP_RELEASE) {
        session_.onRelease(ip, port);
        sendAck(ip, port, cmd.seq, true, REASON_OK);
        return;
    }

    // All other opcodes require the sender to be the active lock holder.
    const bool holder = session_.isHolder(ip, port, nowMs);

    if (reason != REASON_OK) {
        // Malformed/invalid: NAK discrete commands so the controller learns;
        // streaming frames are dropped silently (observed via QUERY_STATUS).
        if (holder && !CommandCodec::isStreaming(cmd.opcode)) {
            sendAck(ip, port, cmd.seq, false, reason);
        }
        return;
    }

    if (!holder) {
        // Someone else (or nobody) holds the lock.
        if (!CommandCodec::isStreaming(cmd.opcode)) {
            sendAck(ip, port, cmd.seq, false, REASON_IN_USE);
        }
        return;
    }

    // Holder is alive and talking → refresh liveness on every valid frame.
    session_.onContact(ip, port, nowMs);

    switch (cmd.opcode) {
        case OP_HEARTBEAT:
            // Liveness already refreshed above; no reply needed.
            break;

        case OP_SET_EMOTION:
            session_.setEmotionTarget(cmd.seq, cmd.valenceMilli,
                                      cmd.arousalMilli, cmd.durationMs);
            break;

        case OP_SET_GAZE:
            session_.setGazeTarget(cmd.seq, cmd.gazeH, cmd.gazeV, cmd.durationMs);
            break;

        case OP_SET_PRESET:
            session_.queuePreset(cmd.presetId, cmd.intensityPct);
            sendAck(ip, port, cmd.seq, true, REASON_OK);
            break;

        case OP_BLINK:
            session_.queueBlink(cmd.durationMs);
            sendAck(ip, port, cmd.seq, true, REASON_OK);
            break;

        case OP_WINK:
            session_.queueWink(cmd.eye, cmd.durationMs);
            sendAck(ip, port, cmd.seq, true, REASON_OK);
            break;

        case OP_SET_IDLE:
            session_.queueIdle(cmd.idleEnable);
            sendAck(ip, port, cmd.seq, true, REASON_OK);
            break;

        case OP_QUERY_STATUS:
            sendStatus(ip, port);
            break;

        default:
            break;
    }
}

void BotiEyesServer::applyPending() {
    if (!started_) {
        return;
    }
    const uint32_t nowMs = millis();

    // Drive lock liveness; on active→released transition fall back to idle.
    if (session_.checkTimeout(nowMs)) {
        idleFallbackActive_ = true;
        eyes_.enableIdleBehavior(true);
    }

    const bool active = session_.isActive(nowMs);
    if (active && idleFallbackActive_) {
        // A controller is now driving: stop autonomous idle morphing.
        idleFallbackActive_ = false;
        eyes_.enableIdleBehavior(false);
    }

    PendingTargets& p = session_.pending();

    if (p.hasEmotion) {
        eyes_.setEmotion(p.valenceMilli / (float)VALENCE_SCALE,
                         p.arousalMilli / (float)AROUSAL_SCALE,
                         p.emotionDurationMs);
    }
    if (p.hasGaze) {
        eyes_.setEyePosition(p.gazeH, p.gazeV, p.gazeDurationMs);
    }
    if (p.actions & ACTION_PRESET) {
        applyPreset(eyes_, p.presetId, p.presetIntensityPct / (float)INTENSITY_SCALE);
    }
    if (p.actions & ACTION_IDLE) {
        eyes_.enableIdleBehavior(p.idleEnable != 0);
    }
    if (p.actions & ACTION_BLINK) {
        eyes_.blink(p.blinkDurationMs);
    }
    if (p.actions & ACTION_WINK) {
        eyes_.wink(p.winkEye == 1 ? RIGHT : LEFT, p.winkDurationMs);
    }

    session_.clearPending();
}

void BotiEyesServer::sendAck(uint32_t ip, uint16_t port, uint32_t seq,
                             bool accepted, uint8_t reason) {
    uint8_t buf[SIZE_ACK];
    size_t n = CommandCodec::encodeAck(seq, accepted, reason, buf);
    IPAddress dest(ip);
    udp_.beginPacket(dest, port);
    udp_.write(buf, n);
    udp_.endPacket();
}

void BotiEyesServer::sendStatus(uint32_t ip, uint16_t port) {
    float valence = 0.0f, arousal = 0.0f;
    eyes_.getCurrentEmotion(&valence, &arousal);
    int16_t h = 0, v = 0;
    eyes_.getEyePosition(&h, &v);

    StatusReport s;
    s.valenceMilli = (int16_t)(valence * VALENCE_SCALE);
    s.arousalMilli = (uint16_t)(arousal * AROUSAL_SCALE);
    s.gazeH = h;
    s.gazeV = v;
    s.idleEnabled = idleFallbackActive_ ? 1 : 0;
    s.connectionHealthy = session_.isActive(millis()) ? 1 : 0;
    s.uptimeMs = millis() - startMs_;

    uint8_t buf[SIZE_STATUS];
    size_t n = CommandCodec::encodeStatus(s, buf);
    IPAddress dest(ip);
    udp_.beginPacket(dest, port);
    udp_.write(buf, n);
    udp_.endPacket();
}

} // namespace net
} // namespace BotiEyes

#endif // ESP32
