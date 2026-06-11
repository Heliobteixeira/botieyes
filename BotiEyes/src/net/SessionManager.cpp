// SessionManager implementation — pure, platform-agnostic.

#include "SessionManager.h"

namespace BotiEyes {
namespace net {

SessionManager::SessionManager()
    : active_(false),
      lastContactMs_(0),
      lastEmotionSeq_(0),
      lastGazeSeq_(0) {
    holder_.ip = 0;
    holder_.port = 0;
    pending_.clear();
    pending_.actions = 0;
}

ReasonCode SessionManager::onAcquire(uint32_t ip, uint16_t port, uint32_t nowMs) {
    // Free, or a stale lock that has timed out → grant to the new client.
    if (!active_ || (nowMs - lastContactMs_) > SESSION_TIMEOUT_MS) {
        active_ = true;
        holder_.ip = ip;
        holder_.port = port;
        lastContactMs_ = nowMs;
        // Reset per-channel sequence tracking for the new session.
        lastEmotionSeq_ = 0;
        lastGazeSeq_ = 0;
        return REASON_OK;
    }
    // Already held by the same client → idempotent OK (refresh liveness).
    if (holder_.equals(ip, port)) {
        lastContactMs_ = nowMs;
        return REASON_OK;
    }
    // Held by someone else.
    return REASON_IN_USE;
}

void SessionManager::onRelease(uint32_t ip, uint16_t port) {
    if (active_ && holder_.equals(ip, port)) {
        active_ = false;
    }
}

void SessionManager::onContact(uint32_t ip, uint16_t port, uint32_t nowMs) {
    if (active_ && holder_.equals(ip, port)) {
        lastContactMs_ = nowMs;
    }
}

bool SessionManager::isHolder(uint32_t ip, uint16_t port, uint32_t nowMs) const {
    return active_ && holder_.equals(ip, port) &&
           (nowMs - lastContactMs_) <= SESSION_TIMEOUT_MS;
}

bool SessionManager::isActive(uint32_t nowMs) const {
    return active_ && (nowMs - lastContactMs_) <= SESSION_TIMEOUT_MS;
}

bool SessionManager::checkTimeout(uint32_t nowMs) {
    if (active_ && (nowMs - lastContactMs_) > SESSION_TIMEOUT_MS) {
        active_ = false;
        return true;  // transition active → released
    }
    return false;
}

void SessionManager::setEmotionTarget(uint32_t seq, int16_t valenceMilli,
                                      uint16_t arousalMilli, uint16_t durationMs) {
    // Newest-wins: ignore stale/reordered frames for this channel.
    if (seq <= lastEmotionSeq_) {
        return;
    }
    lastEmotionSeq_ = seq;
    pending_.hasEmotion = true;
    pending_.valenceMilli = valenceMilli;
    pending_.arousalMilli = arousalMilli;
    pending_.emotionDurationMs = durationMs;
}

void SessionManager::setGazeTarget(uint32_t seq, int16_t h, int16_t v,
                                   uint16_t durationMs) {
    if (seq <= lastGazeSeq_) {
        return;
    }
    lastGazeSeq_ = seq;
    pending_.hasGaze = true;
    pending_.gazeH = h;
    pending_.gazeV = v;
    pending_.gazeDurationMs = durationMs;
}

void SessionManager::queueBlink(uint16_t durationMs) {
    pending_.actions |= ACTION_BLINK;
    pending_.blinkDurationMs = durationMs;
}

void SessionManager::queueWink(uint8_t eye, uint16_t durationMs) {
    pending_.actions |= ACTION_WINK;
    pending_.winkEye = eye;
    pending_.winkDurationMs = durationMs;
}

void SessionManager::queueIdle(uint8_t enable) {
    pending_.actions |= ACTION_IDLE;
    pending_.idleEnable = enable;
}

void SessionManager::queuePreset(uint8_t presetId, uint8_t intensityPct) {
    pending_.actions |= ACTION_PRESET;
    pending_.presetId = presetId;
    pending_.presetIntensityPct = intensityPct;
}

} // namespace net
} // namespace BotiEyes
