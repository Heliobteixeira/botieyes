/**
 * @file app_state.h
 * @brief Application State Machine - Public API
 * 
 * Centralized management of application lifecycle states with validated
 * transitions and event notification.
 */

#pragma once

#include <esp_err.h>
#include <esp_event.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Public Types
// ============================================================================

/**
 * @brief Application lifecycle states
 */
typedef enum {
    APP_STATE_INIT,         ///< Initializing components
    APP_STATE_CONNECTING,   ///< Waiting for WiFi
    APP_STATE_CONNECTED,    ///< WiFi connected, idle
    APP_STATE_RUNNING,      ///< Network control active
    APP_STATE_ERROR,        ///< Recoverable error
    APP_STATE_SAFE_MODE,    ///< Boot loop detected
    APP_STATE_SHUTDOWN      ///< Graceful shutdown
} app_state_t;

/**
 * @brief Extended state information
 */
typedef struct {
    app_state_t current;
    app_state_t previous;
    uint32_t transition_time_ms;    ///< Timestamp of last transition
    uint32_t state_duration_ms;     ///< Time in current state
    char error_message[128];        ///< Error context (if ERROR state)
} app_state_info_t;

// ============================================================================
// Custom Events
// ============================================================================

/**
 * @brief Application State event base
 */
ESP_EVENT_DECLARE_BASE(APP_STATE_EVENT);

/// Application State event IDs
enum {
    STATE_CHANGED,  ///< State transition occurred (payload: app_state_info_t*)
};

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Initialize state machine
 * 
 * Creates mutex, initializes to INIT state, posts initial STATE_CHANGED event.
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if mutex creation fails
 * 
 * @note Must be called after esp_event_loop_create_default()
 * @note Not thread-safe, call from main task only
 */
esp_err_t app_state_init(void);

/**
 * @brief Transition to new state
 * 
 * Validates transition (per state machine rules), updates state, posts event.
 * Invalid transitions are rejected and logged.
 * 
 * Valid transitions:
 * - INIT → CONNECTING | ERROR
 * - CONNECTING → CONNECTED | ERROR | CONNECTING (retry)
 * - CONNECTED → RUNNING | CONNECTING (disconnect) | ERROR
 * - RUNNING → CONNECTED (stop) | CONNECTING | ERROR
 * - ERROR → CONNECTING (recovery) | SAFE_MODE | SHUTDOWN
 * - SAFE_MODE → INIT (reset) | SHUTDOWN
 * - SHUTDOWN → (reboot)
 * 
 * @param new_state Target state
 * @param reason Human-readable reason for transition (max 127 chars, can be NULL)
 * 
 * @return ESP_OK if transition allowed and completed
 *         ESP_ERR_INVALID_STATE if transition not allowed
 * 
 * @note Thread-safe (mutex-protected)
 * @note Blocks briefly to acquire mutex and post event
 */
esp_err_t app_state_transition(app_state_t new_state, const char *reason);

/**
 * @brief Get current state
 * 
 * @return Current state enum value
 * 
 * @note Thread-safe (mutex-protected)
 * @note Non-blocking (mutex timeout: 100ms)
 */
app_state_t app_state_get_current(void);

/**
 * @brief Get extended state information
 * 
 * @return State info structure (by value)
 * 
 * @note Thread-safe (mutex-protected)
 * @note Non-blocking (mutex timeout: 100ms)
 */
app_state_info_t app_state_get_info(void);

/**
 * @brief Set error state with message
 * 
 * Convenience function: transitions to ERROR state and stores error message.
 * 
 * @param error_message Error description (max 127 chars)
 * 
 * @return ESP_OK on success
 * 
 * @note Thread-safe
 */
esp_err_t app_state_set_error(const char *error_message);

#ifdef __cplusplus
}
#endif
