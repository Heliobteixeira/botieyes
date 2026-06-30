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
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

// Component includes (Phase 7+)
#include "wifi_manager.h"
#include "app_state.h"
#include "config_manager.h"
#include "health_monitor.h"  // T091: Health monitor integration
#include "hal_board.h"       // T109: HAL board abstraction (LED only, display TBD)
#include "hal_led.h"         // Direct LED access

#include <Adafruit_GFX.h>
#include "display_init.h"    // Legacy display init (hal_display_spi.c incomplete)
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

// Event bases are defined in their owning component files:
// - WIFI_MGR_EVENT defined in components/wifi_manager/src/wifi_manager.c
// - APP_STATE_EVENT defined in components/state_machine/src/app_state.c
// Headers provide ESP_EVENT_DECLARE_BASE() for use in other files

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
static const char *TAG_INIT = "APP:INIT";  // T097: Initialization logging (FR-043)
static const char *TAG_NET = "NET:CMD";    // T104: Network task logging (FR-043)

// Network command queue (FR-006, FR-013) - used for inter-task communication
// Application task receives commands from network task via this queue
QueueHandle_t g_network_cmd_queue = nullptr;

static led_strip_handle_t s_status_led;
static bool s_wifi_connected = false;
static BotiEyes::BotiEyes *s_eyes = nullptr;
static BotiEyes::net::BotiEyesServer *s_server = nullptr;
static TaskHandle_t s_app_task_handle = nullptr;      // T093: For watchdog registration
static TaskHandle_t s_network_task_handle = nullptr;  // T093: For watchdog registration

static bool configure_status_led(void)
{
    if (CONFIG_BOTIEYES_STATUS_LED_PIN < 0)
    {
        ESP_LOGI(TAG, "Status LED disabled (CONFIG_BOTIEYES_STATUS_LED_PIN < 0)");
        return false;
    }

    // Configure WS2812 LED using updated API
    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BOTIEYES_STATUS_LED_PIN,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false
        }
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false
        }
    };

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
    ESP_LOGI(TAG_NET, "Network task started on core %d", xPortGetCoreID());
    
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
                ESP_LOGW(TAG_NET, "Network task stack low: %u bytes remaining", watermark);
            } else {
                ESP_LOGD(TAG_NET, "Network task stack watermark: %u bytes", watermark);
            }
        }
        
        // Feed watchdog (T095: registered in Phase 10)
        esp_task_wdt_reset();
        
        poll_count++;
        vTaskDelay(pdMS_TO_TICKS(poll_interval_ms));
    }
}

/* Legacy main_task removed (T114) - replaced by app_task() in Phase 6 */

extern "C" void app_main(void)
{
    // T108: Startup logging with ESP-IDF version, board type, and configuration (FR-046)
    ESP_LOGI(TAG_INIT, "=== BotiEyes ESP-IDF Firmware ===");
    #ifdef PROJECT_VER
    ESP_LOGI(TAG_INIT, "Version: %s", PROJECT_VER);
    #endif
    ESP_LOGI(TAG_INIT, "ESP-IDF: %s", esp_get_idf_version());
    
    // Log board type from Kconfig
    #ifdef CONFIG_BOTIEYES_BOARD_TTGO_LORA32
    ESP_LOGI(TAG_INIT, "Board: TTGO LoRa32 (ESP32 + I2C SSD1306)");
    #elif defined(CONFIG_BOTIEYES_BOARD_ESP32S3_SPI)
    ESP_LOGI(TAG_INIT, "Board: ESP32-S3 with SPI SSD1306");
    #else
    ESP_LOGI(TAG_INIT, "Board: Unknown/Custom");
    #endif
    
    // Log key sdkconfig selections
    #ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
    ESP_LOGI(TAG_INIT, "Display: I2C (SDA=%d, SCL=%d, ADDR=0x%02x)", 
             CONFIG_BOTIEYES_OLED_I2C_SDA, CONFIG_BOTIEYES_OLED_I2C_SCL, CONFIG_BOTIEYES_OLED_I2C_ADDRESS);
    #elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_SPI)
    ESP_LOGI(TAG_INIT, "Display: SPI (MOSI=%d, SCK=%d, CS=%d, DC=%d)", 
             CONFIG_BOTIEYES_OLED_SPI_MOSI, CONFIG_BOTIEYES_OLED_SPI_SCK, 
             CONFIG_BOTIEYES_OLED_SPI_CS, CONFIG_BOTIEYES_OLED_SPI_DC);
    #endif

    // ========================================================================
    // Phase 10: Health Monitor Boot Check (T091) - MUST BE FIRST
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "[0/6] Checking for boot loop");
    esp_err_t boot_check = health_monitor_on_boot();
    if (boot_check == ESP_ERR_INVALID_STATE) {
        // T087, T096: Safe mode detected - minimal services only (FR-042)
        ESP_LOGE(TAG_INIT, "SAFE MODE ACTIVE - Boot loop detected");
        ESP_LOGE(TAG_INIT, "Minimal services only. Factory reset required to exit safe mode.");
        
        // Initialize only display to show safe mode message
        esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK(nvs_flash_erase());
            ret = nvs_flash_init();
        }
        ESP_ERROR_CHECK(ret);
        
        // Show safe mode message (simplified - display API compatibility)
        ESP_LOGE(TAG, "===============================" );
        ESP_LOGE(TAG, "      SAFE MODE ACTIVE        ");
        ESP_LOGE(TAG, "  Boot loop detected (3+ crashes)");
        ESP_LOGE(TAG, "  Factory reset required      ");
        ESP_LOGE(TAG, "===============================" );
        
        // TODO: Display safe mode on OLED when display API is stable
        // (Currently requires BotiEyes display initialization which may be incompatible)
        
        // Enter infinite loop - no WiFi, no network
        // User must factory reset or power cycle with sufficient delay
        ESP_LOGE(TAG_INIT, "System halted in safe mode. Power cycle required.");
        while (1) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
    }
    
    ESP_LOGI(TAG_INIT, "Boot check passed, proceeding with normal initialization");

    // ========================================================================
    // Phase 1: ESP-IDF Core Initialization (FR-001)
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "[1/6] Initializing NVS flash");
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG_INIT, "NVS partition needs erasing, reinitializing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    ESP_LOGI(TAG_INIT, "[2/6] Initializing ESP event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    ESP_LOGI(TAG_INIT, "[3/6] Initializing network interface");
    ESP_ERROR_CHECK(esp_netif_init());
    
    ESP_LOGI(TAG_INIT, "[4/6] Creating network command queue");
    g_network_cmd_queue = xQueueCreate(10, sizeof(network_cmd_t)); // 10 commands max (FR-013)
    if (g_network_cmd_queue == nullptr) {
        ESP_LOGE(TAG_INIT, "Failed to create network command queue!");
        return;
    }
    
    // ========================================================================
    // Phase 2: Legacy Initialization (To be refactored in later phases)
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "[5/6] Initializing legacy components");

    // Step 1: Initialize status LED (blue = starting)
    ESP_LOGI(TAG_INIT, "Initializing status LED on GPIO %d", CONFIG_BOTIEYES_STATUS_LED_PIN);
    if (configure_status_led())
    {
        set_status_led(0, 0, 255); // Blue: starting
    }
    else
    {
        ESP_LOGW(TAG_INIT, "Status LED disabled or failed to initialize");
    }

    // Step 2: Initialize OLED display
    ESP_LOGI(TAG_INIT, "Initializing OLED display");
    auto *display = BotiEyes::display::initializeDisplay();
    auto *flushable = BotiEyes::display::initializeDisplayFlushable();
    if (!display || !flushable)
    {
        ESP_LOGE(TAG_INIT, "Display initialization failed!");
        set_status_led(255, 0, 0); // Red: error
        return;
    }

    // Step 3: Create DisplayConfig
    BotiEyes::DisplayConfig displayConfig = BotiEyes::display::createDisplayConfig();

    // Step 4: Initialize BotiEyes library
    ESP_LOGI(TAG_INIT, "Initializing BotiEyes library");
    s_eyes = new BotiEyes::BotiEyes();
    s_eyes->initialize(displayConfig);
    s_eyes->setDisplay(display, flushable);

    // Set neutral expression (valence=0.0, arousal=0.5)
    s_eyes->setEmotion(0.0f, 0.5f, 500);

    ESP_LOGI(TAG_INIT, "Display initialized, showing neutral eyes");

    // ========================================================================
    // Phase 3.5: Configuration Manager Integration - T078
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "Initializing configuration manager");
    ret = config_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_INIT, "Config manager initialization failed: %s", esp_err_to_name(ret));
        set_status_led(255, 0, 0); // Red: error
        // Continue anyway - will use defaults
    }

    // ========================================================================
    // Phase 3.6: Application State Machine Integration - T067, T069
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "Initializing application state machine");
    ret = app_state_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_INIT, "State machine initialization failed: %s", esp_err_to_name(ret));
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
        ESP_LOGW(TAG_INIT, "Failed to register state change handler: %s", esp_err_to_name(ret));
    }

    // ========================================================================
    // Phase 4: WiFi Manager Integration (FR-022 to FR-025) - T058, T059, T068
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "Initializing WiFi manager");
    set_status_led(0, 0, 255); // Blue: connecting
    
    // T058: Initialize WiFi manager component
    ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_INIT, "WiFi manager initialization failed: %s", esp_err_to_name(ret));
        set_status_led(255, 0, 0); // Red: error
        // Continue anyway - WiFi is not critical for basic operation
    }
    
    // T059: Register event handlers for WiFi manager events (FR-025)
    esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_EVENT_CONNECTED,
                               &wifi_connected_handler, NULL);
    esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_EVENT_FAILED,
                               &wifi_failed_handler, NULL);
    esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_EVENT_DISCONNECTED,
                               &wifi_disconnected_handler, NULL);
    
    // T068: Transition to CONNECTING state before WiFi connection (FR-026)
    ret = app_state_transition(APP_STATE_CONNECTING, "Starting WiFi connection");
    if (ret != ESP_OK) {
        ESP_LOGW(TAG_INIT, "State transition to CONNECTING failed: %s", esp_err_to_name(ret));
    }
    
    // Set credentials from Kconfig (T060 - temporarily using Kconfig until config_manager ready)
    #ifdef CONFIG_BOTIEYES_WIFI_SSID
    ret = wifi_manager_set_credentials(CONFIG_BOTIEYES_WIFI_SSID,
                                        CONFIG_BOTIEYES_WIFI_PASSWORD);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG_INIT, "WiFi credentials configured from Kconfig");
        
        // Start connection
        ret = wifi_manager_connect();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG_INIT, "WiFi connection start failed: %s", esp_err_to_name(ret));
            // T071: Transition to ERROR state on critical failure
            app_state_set_error("WiFi connection failed to start");
        }
    } else {
        ESP_LOGW(TAG_INIT, "Failed to set WiFi credentials: %s", esp_err_to_name(ret));
    }
    #else
    ESP_LOGW(TAG_INIT, "No WiFi credentials configured in Kconfig");
    #endif

    // Wait a moment for initial WiFi connection attempt
    vTaskDelay(pdMS_TO_TICKS(100));

    // ========================================================================
    // Phase 10: Health Monitor Initialization (T092) - After components, before tasks
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "[6/6] Initializing health monitor");
    ret = health_monitor_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_INIT, "Health monitor initialization failed: %s", esp_err_to_name(ret));
        // Continue anyway - system can run without watchdog, but less safe
    }

    // ========================================================================
    // Phase 3: Task Creation (FR-031, FR-032, FR-033) - T044, T045, T046
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "Creating FreeRTOS tasks");
    
    // T046: Create network task on Core 0 (network I/O) with HIGH priority
    BaseType_t net_result = xTaskCreatePinnedToCore(
        &network_task,
        "network",               // Task name
        TASK_STACK_MEDIUM,       // 4KB stack (FR-032)
        NULL,                    // Parameters
        TASK_PRIORITY_HIGH,      // High priority for network I/O (FR-031)
        &s_network_task_handle,  // T093: Store handle for watchdog registration
        0                        // Core 0 - network processing
    );
    
    if (net_result != pdPASS) {
        ESP_LOGE(TAG_INIT, "Failed to create network task!");
        set_status_led(255, 0, 0); // Red: error
        return;
    }
    ESP_LOGI(TAG_INIT, "Network task created on Core 0");
    
    // T045: Create application task on Core 1 (rendering) with NORMAL priority  
    BaseType_t app_result = xTaskCreatePinnedToCore(
        &app_task,
        "app",                   // Task name
        TASK_STACK_MEDIUM,       // 4KB stack (FR-032)
        s_eyes,                  // Pass BotiEyes instance as parameter (T044)
        TASK_PRIORITY_NORMAL,    // Normal priority for application logic (FR-031)
        &s_app_task_handle,      // T093: Store handle for watchdog registration
        1                        // Core 1 - rendering and compute
    );
    
    if (app_result != pdPASS) {
        ESP_LOGE(TAG_INIT, "Failed to create application task!");
        set_status_led(255, 0, 0); // Red: error
        return;
    }
    ESP_LOGI(TAG_INIT, "Application task created on Core 1");

    // ========================================================================
    // Phase 10: Register Tasks with Watchdog (T093)
    // ========================================================================
    
    ESP_LOGI(TAG_INIT, "Registering tasks with watchdog");
    
    // T093: Register network task with watchdog (FR-039, FR-040)
    ret = health_monitor_register_task(s_network_task_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG_INIT, "Failed to register network task with watchdog: %s", 
                 esp_err_to_name(ret));
    }
    
    // T093: Register application task with watchdog (FR-039, FR-040)
    ret = health_monitor_register_task(s_app_task_handle);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG_INIT, "Failed to register app task with watchdog: %s",
                 esp_err_to_name(ret));
    }

    ESP_LOGI(TAG_INIT, "Initialization complete, tasks running");
    ESP_LOGI(TAG_INIT, "Task architecture: network(Core 0, HIGH) + app(Core 1, NORMAL)");
    ESP_LOGI(TAG_INIT, "Component-based architecture: wifi_manager, state_machine, config_manager, health_monitor, hal_board");
}
