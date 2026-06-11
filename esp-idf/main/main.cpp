#include "sdkconfig.h"

#include "esp_log.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_ssd1306.h"
#include "display_init.h"
#include "wifi_init.h"
#include "BotiEyes.h"
#include "net/BotiEyesServer.h"

static const char *TAG = "botieyes_idf";
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
    ESP_LOGI(TAG, "=== BotiEyes ESP-IDF Network Control ===");

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
    BotiEyes::display::ESP_SSD1306 *display = BotiEyes::display::initializeDisplay();
    if (!display)
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
    s_eyes->setDisplay(static_cast<Adafruit_GFX *>(display));

    // Set neutral expression (valence=0.0, arousal=0.5)
    s_eyes->setEmotion(0.0f, 0.5f, 500);

    ESP_LOGI(TAG, "Display initialized, showing neutral eyes");

    // Step 5: Initialize WiFi with status callback
    ESP_LOGI(TAG, "Initializing WiFi...");
    set_status_led(0, 0, 255); // Blue: connecting

    auto wifi_status_cb = [](bool connected, const char *ip)
    {
        s_wifi_connected = connected;
        if (connected)
        {
            ESP_LOGI(TAG, "WiFi connected! IP: %s", ip);
            set_status_led(0, 255, 0); // Green: connected

            // Start network control server
            if (s_eyes && !s_server)
            {
                ESP_LOGI(TAG, "Starting BotiEyes network server on port %d", CONFIG_BOTIEYES_UDP_PORT);
                s_server = new BotiEyes::net::BotiEyesServer(*s_eyes);
                s_server->begin(CONFIG_BOTIEYES_UDP_PORT);
                ESP_LOGI(TAG, "Network server started, listening for commands");
            }
        }
        else
        {
            ESP_LOGE(TAG, "WiFi connection failed");
            set_status_led(255, 0, 0); // Red: error
        }
    };

    if (!BotiEyes::wifi::initialize(wifi_status_cb))
    {
        ESP_LOGE(TAG, "WiFi initialization failed!");
        set_status_led(255, 0, 0); // Red: error
    }

    // Wait a moment for WiFi connection attempt
    vTaskDelay(pdMS_TO_TICKS(100));

    // Step 6: Create main rendering task
    ESP_LOGI(TAG, "Creating main task");
    BaseType_t result = xTaskCreate(
        &main_task,
        "main_task",
        8192, // Stack size: 8KB for network + display + animation
        NULL,
        5, // Priority: medium
        NULL);

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create main task!");
        set_status_led(255, 0, 0); // Red: error
        return;
    }

    ESP_LOGI(TAG, "Initialization complete, main task running");
}
