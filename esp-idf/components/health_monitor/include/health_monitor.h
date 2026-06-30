/**
 * @file health_monitor.h
 * @brief Health Monitor - Public API
 * 
 * System health monitoring: watchdog management, crash recovery, boot loop detection.
 */

#pragma once

#include <esp_err.h>
#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Public Types
// ============================================================================

/**
 * @brief System health status
 */
typedef struct {
    uint32_t boot_count;        ///< Number of boots in last 60 seconds
    uint32_t uptime_sec;        ///< Current uptime in seconds
    bool safe_mode;             ///< True if in safe mode
    bool watchdog_active;       ///< True if watchdog initialized
} health_status_t;

// ============================================================================
// Public Functions
// ============================================================================

/**
 * @brief Check for boot loop on startup
 * 
 * Call BEFORE any initialization. Checks RTC memory for previous crash,
 * increments boot count, and enters safe mode if boot loop detected.
 * 
 * @return ESP_OK if normal boot
 *         ESP_ERR_INVALID_STATE if entering safe mode
 * 
 * @note Must be first call in app_main() before any component init
 * @note Not thread-safe
 */
esp_err_t health_monitor_on_boot(void);

/**
 * @brief Initialize health monitor
 * 
 * Configures task watchdog with 30-second timeout.
 * Creates task registry for monitoring.
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if allocation fails
 *         ESP_FAIL if watchdog init fails
 * 
 * @note Call after component initialization, before task creation
 * @note Not thread-safe, call from main task only
 */
esp_err_t health_monitor_init(void);

/**
 * @brief Register task with watchdog
 * 
 * Subscribes task to watchdog monitoring. Task MUST call esp_task_wdt_reset()
 * periodically (within 30 seconds) or system will reset.
 * 
 * @param handle Task handle (from xTaskCreate or xTaskGetCurrentTaskHandle())
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if handle is NULL
 *         ESP_FAIL if watchdog registration fails
 * 
 * @note Thread-safe
 * @note Call once per task after task creation
 */
esp_err_t health_monitor_register_task(TaskHandle_t handle);

/**
 * @brief Check if system is in safe mode
 * 
 * @return true if safe mode active, false otherwise
 * 
 * @note Thread-safe
 */
bool health_monitor_is_safe_mode(void);

/**
 * @brief Get system health status
 * 
 * @return Health status structure (by value)
 * 
 * @note Thread-safe
 */
health_status_t health_monitor_get_status(void);

#ifdef __cplusplus
}
#endif
