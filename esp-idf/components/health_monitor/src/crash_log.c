/**
 * @file crash_log.c
 * @brief Crash log management implementation
 */

#include "health_monitor.h"
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <string.h>
#include "config_manager.h"

static const char *TAG = "SVC:HEALTH";

// ============================================================================
// RTC Memory for Boot Loop Detection (T084, T085)
// ============================================================================

// RTC memory survives warm reboot but not power cycle
// Used for boot loop detection (FR-042)
RTC_DATA_ATTR uint32_t g_boot_count_rtc = 0;
RTC_DATA_ATTR uint64_t g_first_boot_time_us = 0;

// Safe mode flag (global, accessible from watchdog.c)
bool g_safe_mode = false;

// Boot loop thresholds (FR-042)
#define BOOT_LOOP_COUNT_THRESHOLD 3
#define BOOT_LOOP_TIME_WINDOW_US  (60ULL * 1000000ULL)  // 60 seconds

/**
 * @brief Check for boot loop on startup (T085)
 * 
 * Tracks boot count in RTC memory and enters safe mode if
 * 3+ boots within 60 seconds (FR-042)
 */
esp_err_t health_monitor_on_boot(void)
{
    ESP_LOGI(TAG, "Checking for boot loop (boot_count=%lu)", g_boot_count_rtc);
    
    // T083: Read crash info from reset reason
    esp_reset_reason_t reset_reason = esp_reset_reason();
    const char *reason_str = "UNKNOWN";
    
    switch (reset_reason) {
        case ESP_RST_POWERON:   reason_str = "POWERON"; break;
        case ESP_RST_SW:        reason_str = "SOFTWARE"; break;
        case ESP_RST_PANIC:     reason_str = "PANIC"; break;
        case ESP_RST_INT_WDT:   reason_str = "INT_WDT"; break;
        case ESP_RST_TASK_WDT:  reason_str = "TASK_WDT"; break;
        case ESP_RST_WDT:       reason_str = "WDT"; break;
        case ESP_RST_DEEPSLEEP: reason_str = "DEEPSLEEP"; break;
        case ESP_RST_BROWNOUT:  reason_str = "BROWNOUT"; break;
        case ESP_RST_SDIO:      reason_str = "SDIO"; break;
        default: break;
    }
    
    ESP_LOGI(TAG, "Reset reason: %s (%d)", reason_str, reset_reason);
    
    // T083: Store crash log to NVS if this was a crash (FR-041)
    if (reset_reason == ESP_RST_PANIC || 
        reset_reason == ESP_RST_INT_WDT || 
        reset_reason == ESP_RST_TASK_WDT ||
        reset_reason == ESP_RST_WDT) {
        
        ESP_LOGW(TAG, "Previous boot was a crash: %s", reason_str);
        
        // Store crash log via config_manager
        crash_log_t log = {
            .reason = reset_reason,
            .task_name = {0},  // Not available from reset reason
            .pc = 0,           // Would need coredump analysis
            .timestamp = (uint32_t)(esp_timer_get_time() / 1000000ULL),
            .description = {0}
        };
        snprintf(log.description, sizeof(log.description), 
                 "Reset: %s", reason_str);
        
        esp_err_t ret = config_set_crash_log(&log);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "Failed to store crash log: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Crash log stored to NVS");
        }
    }
    
    // T084: Boot loop detection
    uint64_t current_time_us = esp_timer_get_time();
    
    // Check if this is a power-on reset (clear boot count)
    if (reset_reason == ESP_RST_POWERON) {
        g_boot_count_rtc = 1;
        g_first_boot_time_us = current_time_us;
        ESP_LOGI(TAG, "Power-on reset detected, boot count reset to 1");
        return ESP_OK;
    }
    
    // T085: Increment boot count
    g_boot_count_rtc++;
    
    // If this is the first boot in the window, record time
    if (g_boot_count_rtc == 1) {
        g_first_boot_time_us = current_time_us;
    }
    
    // T084: Check if we're in a boot loop
    uint64_t time_since_first_boot_us = current_time_us - g_first_boot_time_us;
    
    ESP_LOGI(TAG, "Boot analysis: count=%lu, time_since_first=%.2fs",
             g_boot_count_rtc,
             time_since_first_boot_us / 1000000.0);
    
    // T084, T087: Enter safe mode if 3+ boots within 60 seconds (FR-042)
    if (g_boot_count_rtc >= BOOT_LOOP_COUNT_THRESHOLD &&
        time_since_first_boot_us < BOOT_LOOP_TIME_WINDOW_US) {
        
        ESP_LOGE(TAG, "BOOT LOOP DETECTED: %lu crashes in %.1f seconds",
                 g_boot_count_rtc,
                 time_since_first_boot_us / 1000000.0);
        ESP_LOGE(TAG, "ENTERING SAFE MODE (minimal services, factory reset available)");
        
        g_safe_mode = true;
        return ESP_ERR_INVALID_STATE;  // Signal safe mode to caller
    }
    
    // Reset boot count if we've passed the time window
    if (time_since_first_boot_us >= BOOT_LOOP_TIME_WINDOW_US) {
        ESP_LOGI(TAG, "Boot window expired, resetting boot count");
        g_boot_count_rtc = 1;
        g_first_boot_time_us = current_time_us;
    }
    
    return ESP_OK;
}
