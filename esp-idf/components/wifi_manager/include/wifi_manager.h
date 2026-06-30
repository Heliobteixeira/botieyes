/**
 * @file wifi_manager.h
 * @brief WiFi Manager Component - Public API
 * 
 * Manages WiFi connectivity lifecycle: connection, reconnection, status monitoring.
 * Integrates with ESP event loop for system-wide event propagation.
 */

#pragma once

#include <esp_err.h>
#include <esp_event.h>
#include <esp_netif_types.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Public Types
// ============================================================================

/**
 * @brief WiFi connection status
 */
typedef enum {
    WIFI_MGR_IDLE,          ///< Not initialized
    WIFI_MGR_CONNECTING,    ///< Connection in progress
    WIFI_MGR_CONNECTED,     ///< Connected and have IP
    WIFI_MGR_DISCONNECTED,  ///< Lost connection, will retry
    WIFI_MGR_FAILED         ///< Failed after max retries
} wifi_manager_status_t;

/**
 * @brief WiFi manager state information
 */
typedef struct {
    wifi_manager_status_t status;
    esp_netif_ip_info_t ip_info;    ///< IP address, netmask, gateway
    uint8_t retry_count;             ///< Current retry attempt
    int8_t rssi;                     ///< Signal strength (dBm)
    uint32_t connected_time_sec;     ///< Duration of current connection
} wifi_manager_state_t;

// ============================================================================
// Custom Events
// ============================================================================

/**
 * @brief WiFi Manager event base
 * 
 * Register handlers: esp_event_handler_register(WIFI_MGR_EVENT, event_id, handler, NULL)
 */
ESP_EVENT_DECLARE_BASE(WIFI_MGR_EVENT);

/// WiFi Manager event IDs
enum {
    WIFI_MGR_EVENT_CONNECTED,     ///< Connection successful, IP acquired (payload: esp_netif_ip_info_t*)
    WIFI_MGR_EVENT_DISCONNECTED,  ///< Connection lost (payload: NULL)
    WIFI_MGR_EVENT_FAILED         ///< Max retries exceeded (payload: NULL)
};

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Initialize WiFi manager
 * 
 * Creates WiFi netif, initializes WiFi driver, registers event handlers,
 * loads credentials from NVS (via config_manager).
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if allocation fails
 *         ESP_FAIL if WiFi init fails
 * 
 * @note Must be called after nvs_flash_init() and esp_event_loop_create_default()
 * @note Not thread-safe, call from main task only
 */
esp_err_t wifi_manager_init(void);

/**
 * @brief Set WiFi credentials (SSID and password)
 * 
 * Stores credentials in NVS via config_manager. Does NOT initiate connection.
 * 
 * @param ssid WiFi SSID (max 32 chars, null-terminated)
 * @param password WiFi password (max 64 chars, null-terminated)
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if ssid/password NULL or too long
 *         ESP_ERR_NVS_* if NVS write fails
 * 
 * @note Thread-safe
 */
esp_err_t wifi_manager_set_credentials(const char *ssid, const char *password);

/**
 * @brief Get WiFi credentials from NVS
 * 
 * @param[out] ssid Buffer for SSID (min 33 bytes)
 * @param ssid_len Size of ssid buffer
 * @param[out] password Buffer for password (min 65 bytes)
 * @param password_len Size of password buffer
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if buffers NULL or too small
 *         ESP_ERR_NVS_NOT_FOUND if credentials not set
 * 
 * @note Thread-safe
 */
esp_err_t wifi_manager_get_credentials(char *ssid, size_t ssid_len,
                                        char *password, size_t password_len);

/**
 * @brief Start WiFi connection
 * 
 * Begins connection attempt using stored credentials. Returns immediately.
 * Connection progress reported via WIFI_MGR_EVENT events.
 * 
 * @return ESP_OK if connection attempt started
 *         ESP_ERR_INVALID_STATE if already connected or connecting
 *         ESP_ERR_NVS_NOT_FOUND if credentials not configured
 * 
 * @note Non-blocking, use events or wifi_manager_get_state() to monitor progress
 * @note Thread-safe
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Disconnect from WiFi
 * 
 * Stops any ongoing connection attempt and disconnects if connected.
 * Auto-reconnect is suspended until wifi_manager_connect() called again.
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_STATE if not connected
 * 
 * @note Blocking, waits for disconnection to complete (max 5 seconds)
 * @note Thread-safe
 */
esp_err_t wifi_manager_disconnect(void);

/**
 * @brief Get current WiFi manager state
 * 
 * Returns snapshot of current connection state, IP info, signal strength, etc.
 * 
 * @return Current state structure (by value)
 * 
 * @note Thread-safe (protected by internal mutex)
 * @note Non-blocking
 */
wifi_manager_state_t wifi_manager_get_state(void);

/**
 * @brief Get IP address as string
 * 
 * @return Pointer to static string "192.168.1.100" or NULL if not connected
 * 
 * @note Thread-safe
 * @note Returned pointer valid until next call from any thread
 */
const char* wifi_manager_get_ip_address(void);

#ifdef __cplusplus
}
#endif
