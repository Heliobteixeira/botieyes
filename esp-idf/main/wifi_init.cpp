#include "wifi_init.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include <cstring>

static const char *TAG = "wifi_init";

// Event bits
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

namespace BotiEyes
{
    namespace wifi
    {

        // Module state
        static EventGroupHandle_t s_wifi_event_group = nullptr;
        static StatusCallback s_status_callback = nullptr;
        static char s_ip_address[16] = {0};
        static bool s_is_connected = false;
        static int s_retry_count = 0;
        static const int MAX_RETRY = 5;

        static void event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data)
        {
            if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
            {
                ESP_LOGI(TAG, "WiFi station started, connecting...");
                esp_wifi_connect();
            }
            else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
            {
                s_is_connected = false;
                memset(s_ip_address, 0, sizeof(s_ip_address));

                if (s_retry_count < MAX_RETRY)
                {
                    esp_wifi_connect();
                    s_retry_count++;
                    ESP_LOGI(TAG, "Retry to connect to AP (attempt %d/%d)", s_retry_count, MAX_RETRY);
                }
                else
                {
                    ESP_LOGE(TAG, "Failed to connect to AP after %d attempts", MAX_RETRY);
                    xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
                    if (s_status_callback)
                    {
                        s_status_callback(false, nullptr);
                    }
                }
            }
            else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
            {
                ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
                snprintf(s_ip_address, sizeof(s_ip_address), IPSTR, IP2STR(&event->ip_info.ip));

                ESP_LOGI(TAG, "Connected! IP address: %s", s_ip_address);
                s_is_connected = true;
                s_retry_count = 0;

                xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
                if (s_status_callback)
                {
                    s_status_callback(true, s_ip_address);
                }
            }
        }

        bool initialize(StatusCallback status_cb)
        {
            s_status_callback = status_cb;

            // Initialize NVS (required for WiFi)
            esp_err_t ret = nvs_flash_init();
            if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
            {
                ESP_ERROR_CHECK(nvs_flash_erase());
                ret = nvs_flash_init();
            }
            ESP_ERROR_CHECK(ret);

            // Create event group
            s_wifi_event_group = xEventGroupCreate();

            // Initialize network interface
            ESP_ERROR_CHECK(esp_netif_init());
            ESP_ERROR_CHECK(esp_event_loop_create_default());
            esp_netif_create_default_wifi_sta();

            // Initialize WiFi with default config
            wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            // Register event handlers
            esp_event_handler_instance_t instance_any_id;
            esp_event_handler_instance_t instance_got_ip;
            ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                                ESP_EVENT_ANY_ID,
                                                                &event_handler,
                                                                nullptr,
                                                                &instance_any_id));
            ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                                IP_EVENT_STA_GOT_IP,
                                                                &event_handler,
                                                                nullptr,
                                                                &instance_got_ip));

            // Configure WiFi station
            wifi_config_t wifi_config = {};
            strncpy((char *)wifi_config.sta.ssid, CONFIG_BOTIEYES_WIFI_SSID, sizeof(wifi_config.sta.ssid) - 1);
            strncpy((char *)wifi_config.sta.password, CONFIG_BOTIEYES_WIFI_PASSWORD, sizeof(wifi_config.sta.password) - 1);
            wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
            wifi_config.sta.pmf_cfg.capable = true;
            wifi_config.sta.pmf_cfg.required = false;

            ESP_LOGI(TAG, "Connecting to SSID: %s", CONFIG_BOTIEYES_WIFI_SSID);

            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
            ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
            ESP_ERROR_CHECK(esp_wifi_start());

            ESP_LOGI(TAG, "WiFi initialization complete, connection in progress...");

            return true;
        }

        bool isConnected()
        {
            return s_is_connected;
        }

        const char *getIPAddress()
        {
            return s_is_connected ? s_ip_address : nullptr;
        }

    } // namespace wifi
} // namespace BotiEyes
