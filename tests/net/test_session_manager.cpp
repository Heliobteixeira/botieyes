// Host-native tests for SessionManager (US1/US2; FR-008, FR-011, FR-012, FR-017).

#include "test_harness.h"
#include "SessionManager.h"
#include "NetProtocol.h"

using namespace BotiEyes::net;

static const uint32_t A_IP = 0xC0A80101;  // 192.168.1.1
static const uint16_t A_PORT = 5000;
static const uint32_t B_IP = 0xC0A80102;  // 192.168.1.2
static const uint16_t B_PORT = 6000;

// --- single-controller lock: acquire, reject second, holder check (FR-017) ---
static void test_lock_acquire_and_reject() {
    SessionManager s;
    CHECK_EQ(s.onAcquire(A_IP, A_PORT, 1000), REASON_OK);
    CHECK(s.isHolder(A_IP, A_PORT, 1000));
    CHECK(s.isActive(1000));
    // Second controller rejected while A holds the lock.
    CHECK_EQ(s.onAcquire(B_IP, B_PORT, 1100), REASON_IN_USE);
    CHECK(!s.isHolder(B_IP, B_PORT, 1100));
    // Re-acquire by same holder is idempotent OK.
    CHECK_EQ(s.onAcquire(A_IP, A_PORT, 1200), REASON_OK);
}

// --- explicit release frees the lock for takeover ---
static void test_release_and_takeover() {
    SessionManager s;
    CHECK_EQ(s.onAcquire(A_IP, A_PORT, 1000), REASON_OK);
    s.onRelease(A_IP, A_PORT);
    CHECK(!s.isActive(1000));
    // B can now take over.
    CHECK_EQ(s.onAcquire(B_IP, B_PORT, 1100), REASON_OK);
    CHECK(s.isHolder(B_IP, B_PORT, 1100));
}

// --- 5 s no-contact timeout releases lock; transition reported once (FR-011) ---
static void test_timeout_and_idle_fallback() {
    SessionManager s;
    CHECK_EQ(s.onAcquire(A_IP, A_PORT, 1000), REASON_OK);
    // Within timeout: still active, no transition.
    CHECK(!s.checkTimeout(1000 + SESSION_TIMEOUT_MS));        // exactly == not > 
    CHECK(s.isActive(1000 + SESSION_TIMEOUT_MS));
    // Past timeout: transition active → released, returns true once.
    CHECK(s.checkTimeout(1000 + SESSION_TIMEOUT_MS + 1));
    CHECK(!s.isActive(1000 + SESSION_TIMEOUT_MS + 1));
    // Subsequent calls return false (already released).
    CHECK(!s.checkTimeout(1000 + SESSION_TIMEOUT_MS + 2));
}

// --- heartbeat / any contact refreshes liveness (FR-011) ---
static void test_heartbeat_refreshes_liveness() {
    SessionManager s;
    CHECK_EQ(s.onAcquire(A_IP, A_PORT, 1000), REASON_OK);
    // Contact at 4000 keeps it alive well past the original 6000 deadline.
    s.onContact(A_IP, A_PORT, 4000);
    CHECK(!s.checkTimeout(4000 + SESSION_TIMEOUT_MS));   // refreshed
    CHECK(s.isActive(4000 + SESSION_TIMEOUT_MS));
    // Contact from a non-holder must NOT refresh.
    s.onContact(B_IP, B_PORT, 8000);
    CHECK(s.checkTimeout(4000 + SESSION_TIMEOUT_MS + 1));
}

// --- takeover after holder times out (FR-012) ---
static void test_takeover_after_timeout() {
    SessionManager s;
    CHECK_EQ(s.onAcquire(A_IP, A_PORT, 1000), REASON_OK);
    // B tries during the stale window after timeout → granted.
    CHECK_EQ(s.onAcquire(B_IP, B_PORT, 1000 + SESSION_TIMEOUT_MS + 1), REASON_OK);
    CHECK(s.isHolder(B_IP, B_PORT, 1000 + SESSION_TIMEOUT_MS + 1));
}

// --- newest-wins coalescing for streaming emotion (FR-008) ---
static void test_newest_wins_emotion() {
    SessionManager s;
    s.onAcquire(A_IP, A_PORT, 1000);
    s.setEmotionTarget(2, 100, 500, 400);
    s.setEmotionTarget(3, -300, 800, 400);   // newer supersedes
    s.setEmotionTarget(1, 0, 0, 400);         // stale, ignored
    CHECK(s.pending().hasEmotion);
    CHECK_EQ(s.pending().valenceMilli, -300);
    CHECK_EQ(s.pending().arousalMilli, 800);
    // Clear after applying.
    s.clearPending();
    CHECK(!s.pending().hasEmotion);
}

// --- newest-wins coalescing for streaming gaze (FR-008) ---
static void test_newest_wins_gaze() {
    SessionManager s;
    s.onAcquire(A_IP, A_PORT, 1000);
    s.setGazeTarget(5, -45, 0, 300);
    s.setGazeTarget(6, 45, 20, 300);
    s.setGazeTarget(4, 0, 0, 300);   // stale
    CHECK(s.pending().hasGaze);
    CHECK_EQ(s.pending().gazeH, 45);
    CHECK_EQ(s.pending().gazeV, 20);
}

// --- one-shot actions queue (US4) ---
static void test_actions_queue() {
    SessionManager s;
    s.onAcquire(A_IP, A_PORT, 1000);
    s.queueBlink(150);
    s.queueWink(1, 150);
    s.queueIdle(0);
    s.queuePreset(0, 100);
    CHECK(s.pending().actions & ACTION_BLINK);
    CHECK(s.pending().actions & ACTION_WINK);
    CHECK(s.pending().actions & ACTION_IDLE);
    CHECK(s.pending().actions & ACTION_PRESET);
    CHECK_EQ(s.pending().winkEye, 1);
    s.clearPending();
    CHECK_EQ(s.pending().actions, 0);
}

int main() {
    RUN(test_lock_acquire_and_reject);
    RUN(test_release_and_takeover);
    RUN(test_timeout_and_idle_fallback);
    RUN(test_heartbeat_refreshes_liveness);
    RUN(test_takeover_after_timeout);
    RUN(test_newest_wins_emotion);
    RUN(test_newest_wins_gaze);
    RUN(test_actions_queue);
    TEST_MAIN_END();
}
