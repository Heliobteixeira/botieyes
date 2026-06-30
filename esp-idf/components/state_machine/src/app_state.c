/**
 * @file app_state.c
 * @brief Application State Machine Implementation
 */

#include "app_state.h"

#include <esp_log.h>
#include <esp_event.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <string.h>

static const char *TAG = "SVC:STATE";

static app_state_info_t s_state_info = {
    .current = APP_STATE_INIT,
    .previous = APP_STATE_INIT,
    .transition_time_ms = 0,
    .state_duration_ms = 0,
    .error_message = {0}
};
static SemaphoreHandle_t s_state_mutex = NULL;

ESP_EVENT_DEFINE_BASE(APP_STATE_EVENT);

esp_err_t app_state_init(void)
{
    ESP_LOGI(TAG, "Initializing application state machine");
    
    s_state_mutex = xSemaphoreCreateMutex();
    if (s_state_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create state mutex");
        return ESP_ERR_NO_MEM;
    }
    
    s_state_info.current = APP_STATE_INIT;
    s_state_info.previous = APP_STATE_INIT;
    s_state_info.transition_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    ESP_LOGI(TAG, "State machine initialized in INIT state");
    return ESP_OK;
}

/**
 * @brief Validate state transition (data-model.md Section 2)
 */
static bool is_valid_transition(app_state_t from, app_state_t to)
{
    // Valid transitions per data-model.md
    switch (from) {
        case APP_STATE_INIT:
            return (to == APP_STATE_CONNECTING || to == APP_STATE_ERROR);
        
        case APP_STATE_CONNECTING:
            return (to == APP_STATE_CONNECTED || to == APP_STATE_ERROR || 
                    to == APP_STATE_CONNECTING); // retry
        
        case APP_STATE_CONNECTED:
            return (to == APP_STATE_RUNNING || to == APP_STATE_CONNECTING || 
                    to == APP_STATE_ERROR);
        
        case APP_STATE_RUNNING:
            return (to == APP_STATE_CONNECTED || to == APP_STATE_CONNECTING || 
                    to == APP_STATE_ERROR);
        
        case APP_STATE_ERROR:
            return (to == APP_STATE_CONNECTING || to == APP_STATE_SAFE_MODE || 
                    to == APP_STATE_SHUTDOWN);
        
        case APP_STATE_SAFE_MODE:
            return (to == APP_STATE_INIT || to == APP_STATE_SHUTDOWN);
        
        case APP_STATE_SHUTDOWN:
            return false; // No transitions from SHUTDOWN
        
        default:
            return false;
    }
}

esp_err_t app_state_transition(app_state_t new_state, const char *reason)
{
    if (s_state_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire state mutex for transition");
        return ESP_ERR_TIMEOUT;
    }
    
    // Validate transition (FR-027, T062)
    if (!is_valid_transition(s_state_info.current, new_state)) {
        ESP_LOGE(TAG, "Invalid state transition: %d -> %d",
                 s_state_info.current, new_state);
        xSemaphoreGive(s_state_mutex);
        return ESP_ERR_INVALID_STATE;
    }
    
    s_state_info.previous = s_state_info.current;
    s_state_info.current = new_state;
    s_state_info.transition_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    if (reason) {
        strncpy(s_state_info.error_message, reason, sizeof(s_state_info.error_message) - 1);
        s_state_info.error_message[sizeof(s_state_info.error_message) - 1] = '\0';
    } else {
        s_state_info.error_message[0] = '\0';
    }
    
    ESP_LOGI(TAG, "State transition: %d -> %d (%s)", 
             s_state_info.previous, s_state_info.current, reason ? reason : "no reason");
    
    // Copy state info for event payload
    app_state_info_t state_info = s_state_info;
    
    xSemaphoreGive(s_state_mutex);
    
    // Post STATE_CHANGED event (FR-029, T064)
    esp_err_t ret = esp_event_post(APP_STATE_EVENT, STATE_CHANGED,
                                    &state_info, sizeof(app_state_info_t),
                                    pdMS_TO_TICKS(100));
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to post state change event: %s", esp_err_to_name(ret));
    }
    
    return ESP_OK;
}

app_state_t app_state_get_current(void)
{
    if (s_state_mutex == NULL) {
        return APP_STATE_INIT;
    }
    
    app_state_t current;
    if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        current = s_state_info.current;
        xSemaphoreGive(s_state_mutex);
    } else {
        current = APP_STATE_INIT;
    }
    
    return current;
}

app_state_info_t app_state_get_info(void)
{
    app_state_info_t info = s_state_info;
    
    if (s_state_mutex != NULL) {
        if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            info = s_state_info;
            xSemaphoreGive(s_state_mutex);
        }
    }
    
    return info;
}

esp_err_t app_state_set_error(const char *error_message)
{
    return app_state_transition(APP_STATE_ERROR, error_message);
}
