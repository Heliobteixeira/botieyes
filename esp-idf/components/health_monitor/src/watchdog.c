/**
 * @file watchdog.c
 * @brief Watchdog management implementation
 */

#include "health_monitor.h"

#include <esp_log.h>
#include <esp_task_wdt.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "SVC:HEALTH:WDT";
static bool watchdog_initialized = false;

// External functions from task_registry.c
extern esp_err_t task_registry_init(void);
extern esp_err_t task_registry_add(TaskHandle_t handle);

// External safe mode state from crash_log.c
extern bool g_safe_mode;

/**
 * @brief Initialize health monitor (T081)
 * 
 * Configures task watchdog with 30-second timeout (FR-039)
 * Handles ESP-IDF v6.0+ auto-initialization gracefully
 */
esp_err_t health_monitor_init(void)
{
    ESP_LOGI(TAG, "Initializing health monitor");
    
    // T081: Configure task watchdog with 30-second timeout (FR-039)
    esp_task_wdt_config_t wdt_config = {
        .timeout_ms = 30000,  // 30 seconds (FR-039)
        .idle_core_mask = 0,  // Don't watch idle tasks
        .trigger_panic = true // Trigger panic on watchdog timeout
    };
    
    esp_err_t ret = esp_task_wdt_init(&wdt_config);
    if (ret == ESP_ERR_INVALID_STATE) {
        // ESP-IDF v6.0+ auto-initializes TWDT - this is expected
        ESP_LOGI(TAG, "Task watchdog already initialized by ESP-IDF");
        
        // Reconfigure with our settings if needed
        ret = esp_task_wdt_reconfigure(&wdt_config);
        if (ret != ESP_OK && ret != ESP_ERR_NOT_SUPPORTED) {
            ESP_LOGW(TAG, "Could not reconfigure watchdog: %s (using existing config)", 
                     esp_err_to_name(ret));
            // Non-fatal - use existing watchdog configuration
        } else {
            ESP_LOGI(TAG, "Task watchdog reconfigured: timeout=30s");
        }
    } else if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize task watchdog: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    } else {
        ESP_LOGI(TAG, "Task watchdog configured: timeout=30s");
    }
    
    // T081: Create task registry for monitoring
    ret = task_registry_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize task registry: %s", esp_err_to_name(ret));
        return ret;
    }
    
    watchdog_initialized = true;
    ESP_LOGI(TAG, "Health monitor initialized successfully");
    return ESP_OK;
}

/**
 * @brief Register task with watchdog (T082)
 * 
 * Subscribes task to watchdog monitoring and adds to task registry (FR-039)
 */
esp_err_t health_monitor_register_task(TaskHandle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (!watchdog_initialized) {
        ESP_LOGW(TAG, "health_monitor not initialized, skipping task registration");
        return ESP_ERR_INVALID_STATE;
    }
    
    const char *task_name = pcTaskGetName(handle);
    ESP_LOGI(TAG, "Registering task with watchdog: %s", task_name);
    
    // T082: Subscribe task to watchdog (FR-039)
    esp_err_t ret = esp_task_wdt_add(handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add task to watchdog: %s (task=%s)", 
                 esp_err_to_name(ret), task_name);
        return ESP_FAIL;
    }
    
    // T082: Add task to registry for monitoring
    ret = task_registry_add(handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to add task to registry: %s (task=%s)", 
                 esp_err_to_name(ret), task_name);
        // Non-fatal - watchdog still active
    }
    
    ESP_LOGI(TAG, "Task registered with watchdog: %s", task_name);
    return ESP_OK;
}

/**
 * @brief Check if system is in safe mode
 */
bool health_monitor_is_safe_mode(void)
{
    return g_safe_mode;
}

/**
 * @brief Get system health status (T086)
 */
health_status_t health_monitor_get_status(void)
{
    extern uint32_t g_boot_count_rtc;  // From crash_log.c
    
    health_status_t status = {
        .boot_count = g_boot_count_rtc,
        .uptime_sec = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000,
        .safe_mode = g_safe_mode,
        .watchdog_active = watchdog_initialized
    };
    
    return status;
}
