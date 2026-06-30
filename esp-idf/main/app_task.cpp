/**
 * @file app_task.cpp
 * @brief Application task implementation for BotiEyes ESP-IDF firmware
 * 
 * Implements the main application loop with queue-based command processing (FR-006, FR-013).
 */

#include "app_task.h"
#include "app_state.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>
#include <esp_task_wdt.h>

// Forward declarations for the command queue and type (defined in main.cpp)
extern QueueHandle_t g_network_cmd_queue;

// Network command types (must match main.cpp)
typedef enum {
    CMD_SET_EMOTION,
    CMD_SET_POSITION,
    CMD_SET_PRESET,
    CMD_IDLE_MODE,
    CMD_PING,
    CMD_RESET
} network_cmd_type_t;

typedef struct {
    network_cmd_type_t type;
    uint16_t seq;
    union {
        struct {
            int16_t valence_milli;
            int16_t arousal_milli;
            uint16_t duration_ms;
        } emotion;
        struct {
            int16_t h_percent;
            int16_t v_percent;
            uint16_t duration_ms;
        } position;
        struct {
            uint8_t preset_id;
            uint8_t intensity;
        } preset;
        struct {
            bool enabled;
        } idle;
        uint8_t raw[56];
    } payload;
} network_cmd_t;

static const char *TAG = "APP:TASK";

// Track whether we've transitioned to RUNNING state (T070)
static bool s_transitioned_to_running = false;

/**
 * @brief Process a single network command (FR-006)
 * 
 * @param cmd The command to process
 * @param eyes BotiEyes instance to apply command to
 */
static void process_command(const network_cmd_t* cmd, BotiEyes::BotiEyes* eyes)
{
    if (!eyes) {
        return;
    }

    switch (cmd->type) {
        case CMD_SET_EMOTION:
            {
                float valence = cmd->payload.emotion.valence_milli / 1000.0f;
                float arousal = cmd->payload.emotion.arousal_milli / 1000.0f;
                uint16_t duration = cmd->payload.emotion.duration_ms;
                ESP_LOGD(TAG, "CMD_SET_EMOTION: valence=%.3f, arousal=%.3f, duration=%u",
                         valence, arousal, duration);
                eyes->setEmotion(valence, arousal, duration);
            }
            break;

        case CMD_SET_POSITION:
            {
                int16_t h = cmd->payload.position.h_percent;
                int16_t v = cmd->payload.position.v_percent;
                uint16_t duration = cmd->payload.position.duration_ms;
                ESP_LOGD(TAG, "CMD_SET_POSITION: h=%d%%, v=%d%%, duration=%u",
                         h, v, duration);
                eyes->setEyePosition(h, v, duration);
            }
            break;

        case CMD_SET_PRESET:
            {
                uint8_t preset = cmd->payload.preset.preset_id;
                uint8_t intensity = cmd->payload.preset.intensity;
                float intensity_f = intensity / 100.0f;
                ESP_LOGD(TAG, "CMD_SET_PRESET: preset=%u, intensity=%u%%", preset, intensity);
                
                // Apply preset based on ID (matches BotiEyesServer preset list)
                switch (preset) {
                    case 0: eyes->happy(intensity_f); break;
                    case 1: eyes->sad(intensity_f); break;
                    case 2: eyes->angry(intensity_f); break;
                    case 3: eyes->calm(intensity_f); break;
                    case 4: eyes->excited(intensity_f); break;
                    case 5: eyes->tired(intensity_f); break;
                    case 6: eyes->surprised(intensity_f); break;
                    case 7: eyes->anxious(intensity_f); break;
                    case 8: eyes->content(intensity_f); break;
                    case 9: eyes->curious(intensity_f); break;
                    case 10: eyes->thinking(intensity_f); break;
                    case 11: eyes->confused(intensity_f); break;
                    case 12: eyes->neutral(intensity_f); break;
                    default:
                        ESP_LOGW(TAG, "Unknown preset ID: %u", preset);
                        break;
                }
            }
            break;

        case CMD_IDLE_MODE:
            {
                bool enabled = cmd->payload.idle.enabled;
                ESP_LOGD(TAG, "CMD_IDLE_MODE: enabled=%d", enabled);
                eyes->enableIdleBehavior(enabled);
            }
            break;

        case CMD_PING:
            // Heartbeat - no action needed, just for keepalive
            ESP_LOGD(TAG, "CMD_PING");
            break;

        case CMD_RESET:
            ESP_LOGI(TAG, "CMD_RESET: resetting to neutral state");
            eyes->setEmotion(0.0f, 0.5f, 500);  // Neutral
            eyes->setEyePosition(0, 0, 300);    // Center
            break;

        default:
            ESP_LOGW(TAG, "Unknown command type: %d", cmd->type);
            break;
    }
}

/**
 * @brief Main application task function (FR-033)
 * 
 * This task runs the main application loop with queue-based command processing:
 * 1. Wait for network commands from queue (with timeout)
 * 2. Process commands and update BotiEyes state
 * 3. Call BotiEyes update() for animation/rendering
 * 4. Feed watchdog
 * 
 * @param arg Task parameter - pointer to BotiEyes instance
 */
void app_task(void *arg)
{
    ESP_LOGI(TAG, "Application task starting on core %d", xPortGetCoreID());
    
    // Get BotiEyes instance from parameter
    BotiEyes::BotiEyes* eyes = static_cast<BotiEyes::BotiEyes*>(arg);
    if (!eyes) {
        ESP_LOGE(TAG, "No BotiEyes instance provided!");
        vTaskDelete(NULL);
        return;
    }

    uint32_t frame_count = 0;
    const uint32_t frame_interval_ms = CONFIG_BOTIEYES_FRAME_INTERVAL_MS;

    ESP_LOGI(TAG, "Application task running, frame interval: %u ms", frame_interval_ms);

    while (1) {
        // Process network commands from queue (FR-006, FR-016)
        // Use 100ms timeout to avoid blocking forever
        network_cmd_t cmd;
        bool received_command = false;
        while (xQueueReceive(g_network_cmd_queue, &cmd, pdMS_TO_TICKS(10)) == pdTRUE) {
            // T070: Transition to RUNNING state on first command (FR-026)
            if (!s_transitioned_to_running && cmd.type != CMD_PING) {
                app_state_t current_state = app_state_get_current();
                if (current_state == APP_STATE_CONNECTED) {
                    esp_err_t ret = app_state_transition(APP_STATE_RUNNING, "Network control active");
                    if (ret == ESP_OK) {
                        ESP_LOGI(TAG, "Transitioned to RUNNING state (network control active)");
                        s_transitioned_to_running = true;
                    } else {
                        ESP_LOGW(TAG, "Failed to transition to RUNNING: %s", esp_err_to_name(ret));
                    }
                }
            }
            
            process_command(&cmd, eyes);
            received_command = true;
        }

        // Update BotiEyes animation and rendering
        eyes->update();

        // Stack watermark monitoring (FR-034) - T048
        if (frame_count % 500 == 0) {  // Check every ~20 seconds (500 * 40ms)
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            if (watermark < 512) {
                ESP_LOGW(TAG, "App task stack low: %u bytes remaining", watermark);
            } else {
                ESP_LOGD(TAG, "App task stack watermark: %u bytes", watermark);
            }
        }

        // Periodic logging (every 5 seconds)
        if (frame_count % (5000 / frame_interval_ms) == 0) {
            ESP_LOGD(TAG, "Frame %lu, queue available: %d",
                     frame_count, uxQueueSpacesAvailable(g_network_cmd_queue));
        }

        // Feed watchdog (will be registered in Phase 10)
        // esp_task_wdt_reset();

        frame_count++;
        
        // Maintain frame timing
        vTaskDelay(pdMS_TO_TICKS(frame_interval_ms));
    }
}
