/**
 * @file wifi_manager.c
 * @brief WiFi Manager Implementation (FR-022 to FR-025)
 * 
 * Manages WiFi connectivity lifecycle with auto-reconnection and event propagation.
 */

#include "wifi_manager.h"
#include "config_manager.h"

#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <nvs.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char *TAG = "SVC:WIFI:MGR";

// WiFi state (protected by mutex)
static wifi_manager_state_t s_wifi_state = {
    .status = WIFI_MGR_IDLE,
    .retry_count = 0,
    .rssi = 0,
    .connected_time_sec = 0
};

static SemaphoreHandle_t s_state_mutex = NULL;
static esp_netif_t *s_sta_netif = NULL;
static bool s_initialized = false;
static char s_ip_address[16] = {0};

// WiFi configuration
typedef struct {
    char ssid[33];
    char password[65];
    uint8_t max_retry;
    uint32_t retry_delay_ms;
    bool auto_reconnect;
} wifi_config_internal_t;

static wifi_config_internal_t s_wifi_config = {
    .ssid = "",
    .password = "",
    .max_retry = 5,
    .retry_delay_ms = 1000,
    .auto_reconnect = true
};

// Event base definition (FR-005, FR-025)
ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT);

// Forward declarations
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data);
static void ip_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data);

/**
 * @brief Initialize WiFi manager (FR-022) - T050
 */
esp_err_t wifi_manager_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "WiFi manager already initialized");
        return ESP_OK;
    }
    
    ESP_LOGI(TAG, "Initializing WiFi manager");
    
    // Create state mutex (FR-024)
    s_state_mutex = xSemaphoreCreateMutex();
    if (s_state_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create state mutex (error=ESP_ERR_NO_MEM)");
        return ESP_ERR_NO_MEM;
    }
    
    // Create default WiFi station netif
    s_sta_netif = esp_netif_create_default_wifi_sta();
    if (s_sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create WiFi station netif (error=ESP_FAIL)");
        return ESP_FAIL;
    }
    
    // Initialize WiFi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_err_t ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Register event handlers (FR-022)
    ret = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID,
                                      &wifi_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WiFi event handler: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ret = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP,
                                      &ip_event_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register IP event handler: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set WiFi mode to station
    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi mode: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Start WiFi
    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WiFi: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Load credentials from config_manager (FR-019)
    config_wifi_t config;
    ret = config_get_wifi(&config);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Loaded WiFi config from NVS: SSID=%s", config.ssid);
        strncpy(s_wifi_config.ssid, config.ssid, sizeof(s_wifi_config.ssid) - 1);
        s_wifi_config.ssid[sizeof(s_wifi_config.ssid) - 1] = '\0';
        strncpy(s_wifi_config.password, config.password, sizeof(s_wifi_config.password) - 1);
        s_wifi_config.password[sizeof(s_wifi_config.password) - 1] = '\0';
        s_wifi_config.max_retry = config.max_retry;
        s_wifi_config.retry_delay_ms = config.retry_delay_ms;
        s_wifi_config.auto_reconnect = config.auto_reconnect;
    } else if (ret == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "No WiFi config found in NVS, using defaults");
    } else {
        ESP_LOGE(TAG, "Failed to load WiFi config: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "WiFi manager initialized successfully");
    s_initialized = true;
    
    return ESP_OK;
}

/**
 * @brief Set WiFi credentials (FR-019, FR-024) - T051
 */
esp_err_t wifi_manager_set_credentials(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (strlen(ssid) > 32 || strlen(password) > 64) {
        ESP_LOGE(TAG, "SSID or password too long (ssid_len=%zu, max=32; pass_len=%zu, max=64)",
                 strlen(ssid), strlen(password));
        return ESP_ERR_INVALID_ARG;
    }
    
    ESP_LOGI(TAG, "Setting WiFi credentials: SSID=%s", ssid);
    
    // Store in config_manager (FR-019)
    config_wifi_t config;
    strncpy(config.ssid, ssid, sizeof(config.ssid) - 1);
    config.ssid[sizeof(config.ssid) - 1] = '\0';
    
    strncpy(config.password, password, sizeof(config.password) - 1);
    config.password[sizeof(config.password) - 1] = '\0';
    
    config.max_retry = s_wifi_config.max_retry;
    config.retry_delay_ms = s_wifi_config.retry_delay_ms;
    config.auto_reconnect = s_wifi_config.auto_reconnect;
    
    esp_err_t ret = config_set_wifi(&config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to store WiFi config in NVS: %s (SSID=%s)",
                 esp_err_to_name(ret), ssid);
        return ret;
    }
    
    // Update internal config
    strncpy(s_wifi_config.ssid, ssid, sizeof(s_wifi_config.ssid) - 1);
    s_wifi_config.ssid[sizeof(s_wifi_config.ssid) - 1] = '\0';
    
    strncpy(s_wifi_config.password, password, sizeof(s_wifi_config.password) - 1);
    s_wifi_config.password[sizeof(s_wifi_config.password) - 1] = '\0';
    
    return ESP_OK;
}

/**
 * @brief Get WiFi credentials (FR-019, FR-024) - T051
 */
esp_err_t wifi_manager_get_credentials(char *ssid, size_t ssid_len,
                                        char *password, size_t password_len)
{
    if (ssid == NULL || password == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    if (ssid_len < 33 || password_len < 65) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Load from config_manager (FR-019)
    config_wifi_t config;
    esp_err_t ret = config_get_wifi(&config);
    if (ret != ESP_OK && ret != ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGE(TAG, "Failed to load WiFi config from NVS: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // If no stored config, check internal cache
    if (ret == ESP_ERR_NVS_NOT_FOUND && strlen(s_wifi_config.ssid) == 0) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    
    // Use stored config or internal cache
    const char *src_ssid = (ret == ESP_OK) ? config.ssid : s_wifi_config.ssid;
    const char *src_password = (ret == ESP_OK) ? config.password : s_wifi_config.password;
    
    strncpy(ssid, src_ssid, ssid_len - 1);
    ssid[ssid_len - 1] = '\0';
    
    strncpy(password, src_password, password_len - 1);
    password[password_len - 1] = '\0';
    
    return ESP_OK;
}

/**
 * @brief Start WiFi connection (FR-025) - T054
 */
esp_err_t wifi_manager_connect(void)
{
    if (!s_initialized) {
        ESP_LOGE(TAG, "WiFi manager not initialized (state=NOT_INIT)");
        return ESP_ERR_INVALID_STATE;
    }
    
    // Check if already connected or connecting
    if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        wifi_manager_status_t status = s_wifi_state.status;
        xSemaphoreGive(s_state_mutex);
        
        if ((int)status == (int)WIFI_MGR_CONNECTED || (int)status == (int)WIFI_MGR_CONNECTING) {
            ESP_LOGW(TAG, "WiFi already %s",
                     (int)status == (int)WIFI_MGR_CONNECTED ? "connected" : "connecting");
            return ESP_ERR_INVALID_STATE;
        }
    }
    
    // Check if credentials are set
    if (strlen(s_wifi_config.ssid) == 0) {
        ESP_LOGE(TAG, "WiFi credentials not configured (SSID=empty)");
        return ESP_ERR_NOT_FOUND;
    }
    
    ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", s_wifi_config.ssid);
    
    // Configure WiFi
    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, s_wifi_config.ssid,
            sizeof(wifi_config.sta.ssid) - 1);
    strncpy((char *)wifi_config.sta.password, s_wifi_config.password,
            sizeof(wifi_config.sta.password) - 1);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;
    
    esp_err_t ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set WiFi config: %s (SSID=%s)",
                 esp_err_to_name(ret), s_wifi_config.ssid);
        return ret;
    }
    
    // Update state to CONNECTING and post event (FR-025)
    if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_wifi_state.status = WIFI_MGR_CONNECTING;
        s_wifi_state.retry_count = 0;
        xSemaphoreGive(s_state_mutex);
    }
    
    // Start connection
    ret = esp_wifi_connect();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect to WiFi: %s (SSID=%s, retry_count=0)",
                 esp_err_to_name(ret), s_wifi_config.ssid);
        
        // Revert state on failure
        if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            s_wifi_state.status = WIFI_MGR_IDLE;
            xSemaphoreGive(s_state_mutex);
        }
        
        return ret;
    }
    
    return ESP_OK;
}

/**
 * @brief Disconnect from WiFi (FR-025) - T055
 */
esp_err_t wifi_manager_disconnect(void)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Disconnecting from WiFi");
    
    // Disable auto-reconnect temporarily
    s_wifi_config.auto_reconnect = false;
    
    esp_err_t ret = esp_wifi_disconnect();
    if (ret != ESP_OK && ret != ESP_ERR_WIFI_NOT_STARTED) {
        ESP_LOGE(TAG, "Failed to disconnect: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Update state
    if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        s_wifi_state.status = WIFI_MGR_DISCONNECTED;
        s_wifi_state.retry_count = 0;
        s_ip_address[0] = '\0';
        xSemaphoreGive(s_state_mutex);
    }
    
    return ESP_OK;
}

/**
 * @brief Get current WiFi state (FR-024) - T056
 */
wifi_manager_state_t wifi_manager_get_state(void)
{
    wifi_manager_state_t state = {0};
    
    if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        state = s_wifi_state;
        xSemaphoreGive(s_state_mutex);
    }
    
    return state;
}

/**
 * @brief Get IP address as string (FR-024) - T057
 */
const char* wifi_manager_get_ip_address(void)
{
    if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if ((int)s_wifi_state.status == (int)WIFI_MGR_CONNECTED && strlen(s_ip_address) > 0) {
            xSemaphoreGive(s_state_mutex);
            return s_ip_address;
        }
        xSemaphoreGive(s_state_mutex);
    }
    
    return NULL;
}

// ============================================================================
// Event Handlers (FR-022, FR-023) - T052, T053
// ============================================================================

/**
 * @brief WiFi event handler
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started");
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event =
            (wifi_event_sta_disconnected_t *)event_data;
        
        ESP_LOGW(TAG, "WiFi disconnected (reason: %d)", event->reason);
        
        // Update state
        bool should_retry = false;
        if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            s_wifi_state.status = WIFI_MGR_DISCONNECTED;
            s_wifi_state.retry_count++;
            s_ip_address[0] = '\0';
            
            // Exponential backoff retry (FR-023)
            should_retry = (s_wifi_state.retry_count < s_wifi_config.max_retry &&
                           s_wifi_config.auto_reconnect);
            
            if (!should_retry) {
                s_wifi_state.status = WIFI_MGR_FAILED;
            }
            
            xSemaphoreGive(s_state_mutex);
        }
        
        if (should_retry) {
            uint32_t delay_ms = s_wifi_config.retry_delay_ms *
                               (1 << (s_wifi_state.retry_count - 1)); // Exponential backoff
            if (delay_ms > 30000) delay_ms = 30000; // Cap at 30 seconds
            
            ESP_LOGI(TAG, "Retrying connection (%d/%d) in %lu ms",
                     s_wifi_state.retry_count, s_wifi_config.max_retry, delay_ms);
            
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
            esp_wifi_connect();
        } else {
            ESP_LOGE(TAG, "WiFi connection failed after %d retries (SSID=%s, retry_count=%d)",
                     s_wifi_config.max_retry, s_wifi_config.ssid, s_wifi_state.retry_count);
            
            // Post FAILED event (FR-025)
            esp_event_post(WIFI_MGR_EVENT, WIFI_MGR_FAILED, NULL, 0, portMAX_DELAY);
        }
        
        // Post DISCONNECTED event (FR-025)
        esp_event_post(WIFI_MGR_EVENT, WIFI_MGR_DISCONNECTED, NULL, 0, portMAX_DELAY);
    }
}

/**
 * @brief IP event handler
 */
static void ip_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
    if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        
        // Format IP address
        snprintf(s_ip_address, sizeof(s_ip_address), IPSTR,
                 IP2STR(&event->ip_info.ip));
        
        ESP_LOGI(TAG, "WiFi connected! IP: %s", s_ip_address);
        
        // Get RSSI
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            ESP_LOGI(TAG, "Signal strength: %d dBm", ap_info.rssi);
        }
        
        // Update state
        if (xSemaphoreTake(s_state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            s_wifi_state.status = WIFI_MGR_CONNECTED;
            s_wifi_state.ip_info = event->ip_info;
            s_wifi_state.retry_count = 0;
            s_wifi_state.rssi = (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) ?
                               ap_info.rssi : 0;
            s_wifi_state.connected_time_sec = 0; // TODO: Track uptime
            xSemaphoreGive(s_state_mutex);
        }
        
        // Re-enable auto-reconnect
        s_wifi_config.auto_reconnect = true;
        
        // Post CONNECTED event with IP info (FR-025)
        esp_event_post(WIFI_MGR_EVENT, WIFI_MGR_CONNECTED,
                      &event->ip_info, sizeof(esp_netif_ip_info_t),
                      portMAX_DELAY);
    }
}
