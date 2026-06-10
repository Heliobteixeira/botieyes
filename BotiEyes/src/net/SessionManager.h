#ifndef BOTIEYES_NET_SESSION_MANAGER_H
#define BOTIEYES_NET_SESSION_MANAGER_H

// SessionManager — pure, platform-agnostic single-controller lock + liveness +
// newest-wins coalescing state machine. No Arduino/Wi-Fi dependencies.
//
// Timing is injected via `nowMs` parameters so the logic is fully host-testable.
// See data-model.md and contracts/network-protocol.md.

#include <stdint.h>
#include "NetProtocol.h"

namespace BotiEyes {
namespace net {

// One-shot action flags (queued per render frame).
enum ActionFlag : uint8_t {
    ACTION_BLINK  = 0x01,
    ACTION_WINK   = 0x02,
    ACTION_IDLE   = 0x04,
    ACTION_PRESET = 0x08
};

// Newest-wins streaming targets + one-shot actions, applied once per frame.
struct PendingTargets {
    bool     hasEmotion;
    int16_t  valenceMilli;
    uint16_t arousalMilli;
    uint16_t emotionDurationMs;

    bool     hasGaze;
    int16_t  gazeH;
    int16_t  gazeV;
    uint16_t gazeDurationMs;

    uint8_t  actions;          // bitmask of ActionFlag
    uint16_t blinkDurationMs;
    uint8_t  winkEye;
    uint16_t winkDurationMs;
    uint8_t  idleEnable;
    uint8_t  presetId;
    uint8_t  presetIntensityPct;

    void clear() {
        hasEmotion = false;
        hasGaze = false;
        actions = 0;
    }
};

// Identity of a controller (IPv4 + UDP port).
struct ClientId {
    uint32_t ip;
    uint16_t port;
    bool equals(uint32_t otherIp, uint16_t otherPort) const {
        return ip == otherIp && port == otherPort;
    }
};

class SessionManager {
public:
    SessionManager();

    // Attempt to acquire the single-controller lock.
    // Returns REASON_OK if granted (free or already held by this client),
    // REASON_IN_USE if another client currently holds it.
    ReasonCode onAcquire(uint32_t ip, uint16_t port, uint32_t nowMs);

    // Release the lock if held by this client.
    void onRelease(uint32_t ip, uint16_t port);

    // Refresh liveness for the lock holder (any received frame / heartbeat).
    void onContact(uint32_t ip, uint16_t port, uint32_t nowMs);

    // True if `ip:port` currently holds the lock and the session is live.
    bool isHolder(uint32_t ip, uint16_t port, uint32_t nowMs) const;

    // True if any controller currently holds a live lock.
    bool isActive(uint32_t nowMs) const;

    // Releases the lock if the holder has been silent for > SESSION_TIMEOUT_MS.
    // Returns true exactly on the transition from active → released (so the
    // caller can trigger idle fallback once).
    bool checkTimeout(uint32_t nowMs);

    // --- newest-wins streaming targets (caller must have validated the cmd) ---
    void setEmotionTarget(uint32_t seq, int16_t valenceMilli,
                          uint16_t arousalMilli, uint16_t durationMs);
    void setGazeTarget(uint32_t seq, int16_t h, int16_t v, uint16_t durationMs);

    // --- one-shot actions ---
    void queueBlink(uint16_t durationMs);
    void queueWink(uint8_t eye, uint16_t durationMs);
    void queueIdle(uint8_t enable);
    void queuePreset(uint8_t presetId, uint8_t intensityPct);

    PendingTargets& pending() { return pending_; }
    const PendingTargets& pending() const { return pending_; }
    void clearPending() { pending_.clear(); }

private:
    bool      active_;
    ClientId  holder_;
    uint32_t  lastContactMs_;
    uint32_t  lastEmotionSeq_;
    uint32_t  lastGazeSeq_;
    PendingTargets pending_;
};

} // namespace net
} // namespace BotiEyes

#endif // BOTIEYES_NET_SESSION_MANAGER_H
