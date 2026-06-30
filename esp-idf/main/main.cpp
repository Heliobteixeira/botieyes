/**
 * @file main.cpp
 * @brief BotiEyes ESP-IDF Firmware - Main Entry Point
 * 
 * Boot Sequence (data-model.md Section 10):
 * 1. ESP-IDF Core Initialization (NVS, event loop, network interface)
 * 2. Component Initialization (WiFi Manager, State Machine, Config Manager, Health Monitor, HAL)
 * 3. Task Creation (Application task, Network task) with proper priorities and core pinning
 * 4. BotiEyes Instance Creation (emotion engine, display rendering)
 * 5. Main Loop: Event-driven architecture with queue-based command processing
 */

#include "sdkconfig.h"
#include "app_task.h"

#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Component includes (Phase 7+)
#include "wifi_manager.h"
#include "app_state.h"
#include "config_manager.h"

#include <Adafruit_GFX.h>
#include "display_init.h"
#include "BotiEyes.h"
#include "net/BotiEyesServer.h"

// ============================================================================
// Task Management Constants (FR-031, FR-032)
// ============================================================================

// Task Priorities (FR-031)
#define TASK_PRIORITY_CRITICAL  (configMAX_PRIORITIES - 1)  // Time-critical tasks
#define TASK_PRIORITY_HIGH      (configMAX_PRIORITIES - 2)  // Network I/O
#define TASK_PRIORITY_NORMAL    (configMAX_PRIORITIES / 2)  // Application logic
#define TASK_PRIORITY_LOW       (tskIDLE_PRIORITY + 1)      // Background tasks

// Task Stack Sizes (FR-032)
#define TASK_STACK_LARGE   (8192)  // 8KB - Complex operations (rendering + network)
#define TASK_STACK_MEDIUM  (4096)  // 4KB - Normal operations
#define TASK_STACK_SMALL   (2048)  // 2KB - Simple monitoring tasks

// ============================================================================
// Event Infrastructure (FR-005, FR-012, FR-025, FR-029) - T038
// ============================================================================

// Custom event bases for application-level events
ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT);    // WiFi manager events (connected, disconnected, failed)
ESP_EVENT_DEFINE_BASE(APP_STATE_EVENT);   // Application state machine events (state transitions)

// ============================================================================
// Network Command Queue (FR-006, FR-013) - T039, T040
// ============================================================================

// Network command types for inter-task communication
typedef enum {
    CMD_SET_EMOTION,     // Set emotion (valence, arousal)
    CMD_SET_POSITION,    // Set eye position (horizontal, vertical)
    CMD_SET_PRESET,      // Apply emotion preset
    CMD_IDLE_MODE,       // Enable/disable idle behavior
    CMD_PING,            // Ping/heartbeat (no action)
    CMD_RESET            // Reset to neutral state
} network_cmd_type_t;

// Network command structure (data-model.md Section 8)
// Maximum payload size: 64 bytes for flexibility
typedef struct {
    network_cmd_type_t type;
    uint16_t seq;               // Sequence number for ack/nak
    union {
        struct {
            int16_t valence_milli;   // Valence * 1000 (-1000 to 1000)
            int16_t arousal_milli;   // Arousal * 1000 (0 to 1000)
            uint16_t duration_ms;    // Transition duration
        } emotion;
        struct {
            int16_t h_percent;       // Horizontal % * 100 (-100 to 100)
            int16_t v_percent;       // Vertical % * 100 (-100 to 100)
            uint16_t duration_ms;    // Transition duration
        } position;
        struct {
            uint8_t preset_id;       // Preset index (0-12)
            uint8_t intensity;       // Intensity 0-100
        } preset;
        struct {
            bool enabled;            // Enable/disable idle behavior
        } idle;
        uint8_t raw[56];            // Raw payload (56 bytes + 8 bytes header = 64 total)
    } payload;
} network_cmd_t;

// ============================================================================
// Global State (FR-006, FR-013)
// ============================================================================

static const char *TAG = "APP:MAIN";

// Network command queue (FR-006, FR-013) - used for inter-task communication
// Application task receives commands from network task via this queue
QueueHandle_t g_network_cmd_queue = nullptr;

static led_strip_handle_t s_status_led;
static bool s_wifi_connected = false;
static BotiEyes::BotiEyes *s_eyes = nullptr;
static BotiEyes::net::BotiEyesServer *s_server = nullptr;

static bool configure_status_led(void)
{
    if (CONFIG_BOTIEYES_STATUS_LED_PIN < 0)
    {
        ESP_LOGI(TAG, "Status LED disabled (CONFIG_BOTIEYES_STATUS_LED_PIN < 0)");
        return false;
    }

    // Let driver auto-detect LED type (official blink example style)
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = CONFIG_BOTIEYES_STATUS_LED_PIN;
    strip_config.max_leds = 1;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000;

    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_status_led));
    led_strip_clear(s_status_led);
    return true;
}

static void set_status_led(uint8_t r, uint8_t g, uint8_t b)
{
    if (s_status_led == nullptr)
    {
        return;
    }

    ESP_ERROR_CHECK(led_strip_set_pixel(s_status_led, 0, r, g, b));
    ESP_ERROR_CHECK(led_strip_refresh(s_status_led));
}

// ============================================================================
// WiFi Manager Event Handlers (FR-025) - T059
// ============================================================================

static void wifi_connected_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "WiFi manager event: CONNECTED");
    s_wifi_connected = true;
    
    const char *ip = wifi_manager_get_ip_address();
    if (ip) {
        ESP_LOGI(TAG, "WiFi connected! IP: %s", ip);
        set_status_led(0, 255, 0); // Green: connected
        
        // T068: Transition to CONNECTED state (FR-026)
        esp_err_t ret = app_state_transition(APP_STATE_CONNECTED, "WiFi connected");
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "State transition to CONNECTED failed: %s", esp_err_to_name(ret));
        }
        
        // Start network control server
        if (s_eyes && !s_server) {
            ESP_LOGI(TAG, "Starting BotiEyes network server on port %d",
                     CONFIG_BOTIEYES_UDP_PORT);
            s_server = new BotiEyes::net::BotiEyesServer(*s_eyes);
            s_server->begin(CONFIG_BOTIEYES_UDP_PORT);
            ESP_LOGI(TAG, "Network server started, listening for commands");
        }
    }
}

static void wifi_failed_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
    ESP_LOGE(TAG, "WiFi manager event: FAILED - connection failed after retries");
    s_wifi_connected = false;
    set_status_led(255, 0, 0); // Red: error
    
    // T071: Transition to ERROR state (FR-026)
    esp_err_t ret = app_state_set_error("WiFi connection failed after max retries");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "State transition to ERROR failed: %s", esp_err_to_name(ret));
    }
}

static void wifi_disconnected_handler(void *arg, esp_event_base_t event_base,
                                       int32_t event_id, void *event_data)
{
    ESP_LOGW(TAG, "WiFi manager event: DISCONNECTED");
    s_wifi_connected = false;
    set_status_led(255, 165, 0); // Orange: disconnected, will retry
    
    // T068: Transition back to CONNECTING (FR-026)
    esp_err_t ret = app_state_transition(APP_STATE_CONNECTING, "WiFi disconnected, retrying");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "State transition to CONNECTING failed: %s", esp_err_to_name(ret));
    }
}

// ============================================================================
// Network Task (FR-033, FR-040) - T047
// ============================================================================

/**
 * @brief Network task function
 * 
 * Runs on Core 0 with HIGH priority (FR-031, FR-033).
 * Responsible for:
 * - Polling UDP socket for incoming commands
 * - Decoding and validating commands
 * - Sending commands to g_network_cmd_queue for processing
 * - Feeding watchdog
 * 
 * @param pvParameters Task parameters (unused)
 */
static void network_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Network task started on core %d", xPortGetCoreID());
    
    uint32_t poll_count = 0;
    const uint32_t poll_interval_ms = 10;  // Poll every 10ms for low latency (FR-010)
    
    while (true) {
        // Poll network server for incoming datagrams (non-blocking)
        if (s_server) {
            s_server->poll();           // Drain UDP datagrams, decode, and queue via handleDatagram
            s_server->applyPending();   // Convert pending targets to queue commands
        }
        
        // Stack watermark monitoring (FR-034) - T049
        if (poll_count % 1000 == 0) {  // Check every 10 seconds
            UBaseType_t watermark = uxTaskGetStackHighWaterMark(NULL);
            if (watermark < 512) {
                ESP_LOGW(TAG, "Network task stack low: %u bytes remaining", watermark);
            } else {
                ESP_LOGD(TAG, "Network task stack watermark: %u bytes", watermark);
            }
        }
        
        // Feed watchdog (will be registered in Phase 10)
        // esp_task_wdt_reset();
        
        poll_count++;
        vTaskDelay(pdMS_TO_TICKS(poll_interval_ms));
    }
}

// Legacy main_task (to be removed after Phase 6) - keeping temporarily for reference
static void main_task(void *pvParameters)
{
    ESP_LOGI(TAG, "Main task started");
    uint32_t frame = 0;
    const uint32_t interval_ms = CONFIG_BOTIEYES_FRAME_INTERVAL_MS;

    while (true)
    {
        // Network control: drain incoming commands and apply to eyes
        if (s_server)
        {
            s_server->poll();         // Non-blocking: process all pending UDP datagrams
            s_server->applyPending(); // Apply coalesced targets + one-shot actions
        }

        // Update eye animation (interpolation + rendering)
        s_eyes->update();

        // Status LED: White when WiFi connected and network server running
        if (s_wifi_connected && s_server && frame % 100 == 0)
        {
            set_status_led(255, 255, 255); // White: running normally
        }

        // Heartbeat log every 5 seconds
        if (frame % (5000 / interval_ms) == 0)
        {
            const char *ip = BotiEyes::wifi::getIPAddress();
            if (ip)
            {
                const char *mode = (s_server && s_server->isIdle()) ? "idle" : "controlled";
                ESP_LOGI(TAG, "Running (frame %lu, WiFi: %s, mode: %s)", frame, ip, mode);
            }
            else
            {
                ESP_LOGI(TAG, "Running (frame %lu, WiFi: disconnected)", frame);
            }
        }

        frame++;
        vTaskDelay(pdMS_TO_TICKS(interval_ms));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== BotiEyes ESP-IDF Firmware ===");
    ESP_LOGI(TAG, "ESP-IDF version: %s", esp_get_idf_version());

    // ========================================================================
    // Phase 1: ESP-IDF Core Initialization (FR-001)
    // ========================================================================
    
    ESP_LOGI(TAG, "[1/5] Initializing NVS flash");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition needs erasing, reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG, "[2/5] Initializing ESP event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_LOGI(TAG, "[3/5] Initializing network interface");
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_LOGI(TAG, "[4/5] Creating network command queue");
    g_network_cmd_queue = xQueueCreate(10, sizeof(network_cmd_t)); // 10 commands max (FR-013)
    if (g_network_cmd_queue == nullptr) {
        ESP_LOGE(TAG, "Failed to create network command queue!");
        return;
    }
    
    // ========================================================================
    // Phase 2: Legacy Initialization (To be refactored in later phases)
    // ========================================================================
    
    ESP_LOGI(TAG, "[5/5] Initializing legacy components");

    // Step 1: Initialize status LED (blue = starting)
    ESP_LOGI(TAG, "Initializing status LED on GPIO %d", CONFIG_BOTIEYES_STATUS_LED_PIN);
    if (configure_status_led())
    {
        set_status_led(0, 0, 255); // Blue: starting
    }
    else
    {
        ESP_LOGW(TAG, "Status LED disabled or failed to initialize");
    }

    // Step 2: Initialize OLED display
    ESP_LOGI(TAG, "Initializing OLED display");
    auto *display = BotiEyes::display::initializeDisplay();
    auto *flushable = BotiEyes::display::initializeDisplayFlushable();
    if (!display || !flushable)
    {
        ESP_LOGE(TAG, "Display initialization failed!");
        set_status_led(255, 0, 0); // Red: error
        return;
    }

    // Step 3: Create DisplayConfig
    BotiEyes::DisplayConfig displayConfig = BotiEyes::display::createDisplayConfig();

    // Step 4: Initialize BotiEyes library
    ESP_LOGI(TAG, "Initializing BotiEyes library");
    s_eyes = new BotiEyes::BotiEyes();
    s_eyes->initialize(displayConfig);
    s_eyes->setDisplay(display, flushable);

    // Set neutral expression (valence=0.0, arousal=0.5)
    s_eyes->setEmotion(0.0f, 0.5f, 500);

    ESP_LOGI(TAG, "Display initialized, showing neutral eyes");

    // ========================================================================
    // Phase 3.5: Configuration Manager Integration - T078
    // ========================================================================
    
    ESP_LOGI(TAG, "Initializing configuration manager");
    ret = config_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Config manager initialization failed: %s", esp_err_to_name(ret));
        set_status_led(255, 0, 0); // Red: error
        // Continue anyway - will use defaults
    }

    // ========================================================================
    // Phase 3.6: Application State Machine Integration - T067, T069
    // ========================================================================
    
    ESP_LOGI(TAG, "Initializing application state machine");
    ret = app_state_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "State machine initialization failed: %s", esp_err_to_name(ret));
        set_status_led(255, 0, 0); // Red: error
        return;
    }
    
    // T069: Register state change event handler for logging (FR-046)
    auto state_change_handler = [](void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data) {
        app_state_info_t *info = (app_state_info_t *)event_data;
        ESP_LOGI(TAG, "State change: %d -> %d at %lu ms (reason: %s)",
                 info->previous, info->current, info->transition_time_ms,
                 info->error_message[0] ? info->error_message : "none");
    };
    
    ret = esp_event_handler_register(APP_STATE_EVENT, STATE_CHANGED,
                                      state_change_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register state change handler: %s", esp_err_to_name(ret));
    }

    // ========================================================================
    // Phase 4: WiFi Manager Integration (FR-022 to FR-025) - T058, T059, T068
    // ========================================================================
    
    ESP_LOGI(TAG, "Initializing WiFi manager");
    set_status_led(0, 0, 255); // Blue: connecting
    
    // T058: Initialize WiFi manager component
    ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi manager initialization failed: %s", esp_err_to_name(ret));
        set_status_led(255, 0, 0); // Red: error
        // Continue anyway - WiFi is not critical for basic operation
    }
    
    // T059: Register event handlers for WiFi manager events (FR-025)
    esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_CONNECTED,
                               &wifi_connected_handler, NULL);
    esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_FAILED,
                               &wifi_failed_handler, NULL);
    esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_DISCONNECTED,
                               &wifi_disconnected_handler, NULL);
    
    // T068: Transition to CONNECTING state before WiFi connection (FR-026)
    ret = app_state_transition(APP_STATE_CONNECTING, "Starting WiFi connection");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "State transition to CONNECTING failed: %s", esp_err_to_name(ret));
    }
    
    // Set credentials from Kconfig (T060 - temporarily using Kconfig until config_manager ready)
    #ifdef CONFIG_BOTIEYES_WIFI_SSID
    ret = wifi_manager_set_credentials(CONFIG_BOTIEYES_WIFI_SSID,
                                        CONFIG_BOTIEYES_WIFI_PASSWORD);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "WiFi credentials configured from Kconfig");
        
        // Start connection
        ret = wifi_manager_connect();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "WiFi connection start failed: %s", esp_err_to_name(ret));
            // T071: Transition to ERROR state on critical failure
            app_state_set_error("WiFi connection failed to start");
        }
    } else {
        ESP_LOGW(TAG, "Failed to set WiFi credentials: %s", esp_err_to_name(ret));
    }
    #else
    ESP_LOGW(TAG, "No WiFi credentials configured in Kconfig");
    #endif

    // Wait a moment for initial WiFi connection attempt
    vTaskDelay(pdMS_TO_TICKS(100));

    // ========================================================================
    // Phase 3: Task Creation (FR-031, FR-032, FR-033) - T044, T045, T046
    // ========================================================================
    
    ESP_LOGI(TAG, "Creating FreeRTOS tasks");
    
    // T046: Create network task on Core 0 (network I/O) with HIGH priority
    BaseType_t net_result = xTaskCreatePinnedToCore(
        &network_task,
        "network",               // Task name
        TASK_STACK_MEDIUM,       // 4KB stack (FR-032)
        NULL,                    // Parameters
        TASK_PRIORITY_HIGH,      // High priority for network I/O (FR-031)
        NULL,                    // Task handle (not needed)
        0                        // Core 0 - network processing
    );
    
    if (net_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create network task!");
        set_status_led(255, 0, 0); // Red: error
        return;
    }
    ESP_LOGI(TAG, "Network task created on Core 0");
    
    // T045: Create application task on Core 1 (rendering) with NORMAL priority  
    BaseType_t app_result = xTaskCreatePinnedToCore(
        &app_task,
        "app",                   // Task name
        TASK_STACK_MEDIUM,       // 4KB stack (FR-032)
        s_eyes,                  // Pass BotiEyes instance as parameter (T044)
        TASK_PRIORITY_NORMAL,    // Normal priority for application logic (FR-031)
        NULL,                    // Task handle (not needed)
        1                        // Core 1 - rendering and compute
    );
    
    if (app_result != pdPASS) {
        ESP_LOGE(TAG, "Failed to create application task!");
        set_status_led(255, 0, 0); // Red: error
        return;
    }
    ESP_LOGI(TAG, "Application task created on Core 1");

    ESP_LOGI(TAG, "Initialization complete, tasks running");
    ESP_LOGI(TAG, "Task architecture: network(Core 0, HIGH) + app(Core 1, NORMAL)");
    ESP_LOGI(TAG, "NOTE: Component-based architecture will be integrated in subsequent phases");
}
