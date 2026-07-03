/**
 * @file config_manager.h
 * @brief Configuration Manager - Public API
 *
 * NVS abstraction for runtime configuration management.
 * Provides typed access to WiFi credentials, application settings, and system data.
 */

#pragma once

#include <esp_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <esp_system.h> // For esp_reset_reason_t

#ifdef __cplusplus
extern "C"
{
#endif

    // ============================================================================
    // Public Types
    // ============================================================================

    /**
     * @brief WiFi configuration
     */
    typedef struct
    {
        char ssid[33];           ///< WiFi SSID (32 chars + null)
        char password[65];       ///< WPA2 password (64 chars + null)
        uint8_t max_retry;       ///< Max connection retries (default: 5)
        uint32_t retry_delay_ms; ///< Initial retry delay (default: 1000)
        bool auto_reconnect;     ///< Enable auto-reconnection (default: true)
    } config_wifi_t;

    /**
     * @brief Application configuration
     */
    typedef struct
    {
        uint8_t brightness;       ///< Display brightness 0-255 (default: 255)
        uint32_t idle_timeout_ms; ///< Idle animation timeout (default: 30000)
        bool network_enabled;     ///< Enable network control (default: true)
    } config_app_t;

    /**
     * @brief Crash log entry (matching health_monitor contract)
     */
    typedef struct
    {
        esp_reset_reason_t reason; ///< Reset reason
        char task_name[16];        ///< Task name (if available)
        uint32_t pc;               ///< Program counter at crash
        uint32_t timestamp;        ///< Unix timestamp
        char description[64];      ///< Crash description
    } crash_log_t;

    // Alias for backward compatibility
    typedef crash_log_t config_crash_log_t;

    // ============================================================================
    // Public Functions
    // ============================================================================

    /**
     * @brief Initialize configuration manager
     *
     * Opens NVS namespaces (botieyes_wifi, botieyes_app, botieyes_sys).
     *
     * @return ESP_OK on success
     *         ESP_ERR_NVS_NOT_INITIALIZED if nvs_flash_init() not called
     *         ESP_FAIL if namespace open fails
     *
     * @note Must be called after nvs_flash_init()
     * @note Not thread-safe, call from main task only
     */
    esp_err_t config_manager_init(void);

    /**
     * @brief Get WiFi configuration from NVS
     *
     * @param[out] config WiFi configuration structure to fill
     *
     * @return ESP_OK on success (returns defaults if keys not found)
     *         ESP_ERR_INVALID_ARG if config is NULL
     *
     * @note Thread-safe
     */
    esp_err_t config_get_wifi(config_wifi_t *config);

    /**
     * @brief Set WiFi configuration in NVS
     *
     * @param config WiFi configuration to store
     *
     * @return ESP_OK on success
     *         ESP_ERR_INVALID_ARG if config is NULL
     *         ESP_ERR_NVS_* if write fails
     *
     * @note Thread-safe
     * @note Changes take effect on next wifi_manager_connect()
     */
    esp_err_t config_set_wifi(const config_wifi_t *config);

    /**
     * @brief Get application configuration from NVS
     *
     * @param[out] config Application configuration structure to fill
     *
     * @return ESP_OK on success (returns defaults if keys not found)
     *         ESP_ERR_INVALID_ARG if config is NULL
     *
     * @note Thread-safe
     */
    esp_err_t config_get_app(config_app_t *config);

    /**
     * @brief Set application configuration in NVS
     *
     * @param config Application configuration to store
     *
     * @return ESP_OK on success
     *         ESP_ERR_INVALID_ARG if config is NULL
     *         ESP_ERR_NVS_* if write fails
     *
     * @note Thread-safe
     */
    esp_err_t config_set_app(const config_app_t *config);

    /**
     * @brief Get crash log from NVS
     *
     * @param[out] log Crash log structure to fill
     *
     * @return ESP_OK if crash log found
     *         ESP_ERR_NVS_NOT_FOUND if no crash log stored
     *         ESP_ERR_INVALID_ARG if log is NULL
     *
     * @note Thread-safe
     */
    esp_err_t config_get_crash_log(config_crash_log_t *log);

    /**
     * @brief Set crash log in NVS
     *
     * @param log Crash log to store
     *
     * @return ESP_OK on success
     *         ESP_ERR_INVALID_ARG if log is NULL
     *         ESP_ERR_NVS_* if write fails
     *
     * @note Thread-safe
     */
    esp_err_t config_set_crash_log(const config_crash_log_t *log);

    /**
     * @brief Factory reset: erase user configuration
     *
     * Erases botieyes_wifi and botieyes_app namespaces.
     * Preserves botieyes_sys (crash logs, boot count).
     *
     * @return ESP_OK on success
     *         ESP_FAIL if erase fails
     *
     * @note Thread-safe
     * @note Changes take effect immediately
     */
    esp_err_t config_factory_reset(void);

#ifdef __cplusplus
}
#endif
