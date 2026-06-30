/**
 * @file config_manager.c
 * @brief Configuration Manager Implementation
 * 
 * NVS abstraction for runtime configuration management.
 */

#include "config_manager.h"
#include "sdkconfig.h"

#include <esp_log.h>
#include <nvs.h>
#include <string.h>

static const char *TAG = "SVC:CONFIG";

static nvs_handle_t wifi_nvs_handle = 0;
static nvs_handle_t app_nvs_handle = 0;
static nvs_handle_t sys_nvs_handle = 0;
static bool initialized = false;

esp_err_t config_manager_init(void)
{
    esp_err_t err;
    
    ESP_LOGI(TAG, "Initializing configuration manager");
    
    // Open WiFi namespace
    err = nvs_open(CONFIG_CONFIG_MGR_NVS_NAMESPACE_WIFI, NVS_READWRITE, &wifi_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open WiFi NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    // Open App namespace
    err = nvs_open(CONFIG_CONFIG_MGR_NVS_NAMESPACE_APP, NVS_READWRITE, &app_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open App NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    // Open System namespace
    err = nvs_open(CONFIG_CONFIG_MGR_NVS_NAMESPACE_SYS, NVS_READWRITE, &sys_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open System NVS namespace: %s", esp_err_to_name(err));
        return err;
    }
    
    initialized = true;
    ESP_LOGI(TAG, "Configuration manager initialized");
    return ESP_OK;
}

esp_err_t config_get_wifi(config_wifi_t *config)
{
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err;
    size_t len;
    
    // Read SSID
    len = sizeof(config->ssid);
    err = nvs_get_str(wifi_nvs_handle, "ssid", config->ssid, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // No stored SSID - return defaults from Kconfig
        strncpy(config->ssid, CONFIG_WIFI_SSID, sizeof(config->ssid) - 1);
        config->ssid[sizeof(config->ssid) - 1] = '\0';
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WiFi SSID: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read Password
    len = sizeof(config->password);
    err = nvs_get_str(wifi_nvs_handle, "password", config->password, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        // No stored password - use Kconfig default
        strncpy(config->password, CONFIG_WIFI_PASSWORD, sizeof(config->password) - 1);
        config->password[sizeof(config->password) - 1] = '\0';
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read WiFi password: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read max_retry (default 5)
    err = nvs_get_u8(wifi_nvs_handle, "max_retry", &config->max_retry);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->max_retry = 5;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read max_retry: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read retry_delay_ms (default 1000)
    err = nvs_get_u32(wifi_nvs_handle, "retry_delay", &config->retry_delay_ms);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->retry_delay_ms = 1000;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read retry_delay: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read auto_reconnect (default true)
    uint8_t auto_reconnect_u8;
    err = nvs_get_u8(wifi_nvs_handle, "auto_recon", &auto_reconnect_u8);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->auto_reconnect = true;
    } else if (err == ESP_OK) {
        config->auto_reconnect = (auto_reconnect_u8 != 0);
    } else {
        ESP_LOGE(TAG, "Failed to read auto_reconnect: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "WiFi config loaded: SSID=%s, max_retry=%d, retry_delay=%lu ms, auto_reconnect=%d",
             config->ssid, config->max_retry, config->retry_delay_ms, config->auto_reconnect);
    
    return ESP_OK;
}

esp_err_t config_set_wifi(const config_wifi_t *config)
{
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err;
    
    // Write SSID
    err = nvs_set_str(wifi_nvs_handle, "ssid", config->ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write WiFi SSID: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write Password
    err = nvs_set_str(wifi_nvs_handle, "password", config->password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write WiFi password: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write max_retry
    err = nvs_set_u8(wifi_nvs_handle, "max_retry", config->max_retry);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write max_retry: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write retry_delay_ms
    err = nvs_set_u32(wifi_nvs_handle, "retry_delay", config->retry_delay_ms);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write retry_delay: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write auto_reconnect
    err = nvs_set_u8(wifi_nvs_handle, "auto_recon", config->auto_reconnect ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write auto_reconnect: %s", esp_err_to_name(err));
        return err;
    }
    
    // Commit changes
    err = nvs_commit(wifi_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit WiFi config: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "WiFi config saved: SSID=%s", config->ssid);
    return ESP_OK;
}

esp_err_t config_get_app(config_app_t *config)
{
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err;
    
    // Read brightness (default 255)
    err = nvs_get_u8(app_nvs_handle, "brightness", &config->brightness);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->brightness = 255;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read brightness: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read idle_timeout_ms (default 30000 = 30 seconds)
    err = nvs_get_u32(app_nvs_handle, "idle_timeout", &config->idle_timeout_ms);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->idle_timeout_ms = 30000;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read idle_timeout: %s", esp_err_to_name(err));
        return err;
    }
    
    // Read network_enabled (default true)
    uint8_t network_enabled_u8;
    err = nvs_get_u8(app_nvs_handle, "net_enabled", &network_enabled_u8);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        config->network_enabled = true;
    } else if (err == ESP_OK) {
        config->network_enabled = (network_enabled_u8 != 0);
    } else {
        ESP_LOGE(TAG, "Failed to read network_enabled: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "App config loaded: brightness=%d, idle_timeout=%lu ms, network_enabled=%d",
             config->brightness, config->idle_timeout_ms, config->network_enabled);
    
    return ESP_OK;
}

esp_err_t config_set_app(const config_app_t *config)
{
    if (!initialized || config == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err;
    
    // Write brightness
    err = nvs_set_u8(app_nvs_handle, "brightness", config->brightness);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write brightness: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write idle_timeout_ms
    err = nvs_set_u32(app_nvs_handle, "idle_timeout", config->idle_timeout_ms);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write idle_timeout: %s", esp_err_to_name(err));
        return err;
    }
    
    // Write network_enabled
    err = nvs_set_u8(app_nvs_handle, "net_enabled", config->network_enabled ? 1 : 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write network_enabled: %s", esp_err_to_name(err));
        return err;
    }
    
    // Commit changes
    err = nvs_commit(app_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit app config: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "App config saved: brightness=%d", config->brightness);
    return ESP_OK;
}

esp_err_t config_get_crash_log(config_crash_log_t *log)
{
    if (!initialized || log == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err;
    size_t len = sizeof(config_crash_log_t);
    
    // Read crash log blob
    err = nvs_get_blob(sys_nvs_handle, "crash_log", log, &len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No crash log found");
        return ESP_ERR_NVS_NOT_FOUND;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read crash log: %s", esp_err_to_name(err));
        return err;
    }
    
    if (len != sizeof(config_crash_log_t)) {
        ESP_LOGW(TAG, "Crash log size mismatch: expected %zu, got %zu", 
                 sizeof(config_crash_log_t), len);
        return ESP_ERR_INVALID_SIZE;
    }
    
    ESP_LOGI(TAG, "Crash log loaded: timestamp=%lu, reset_reason=%d",
             log->timestamp, log->reset_reason);
    
    return ESP_OK;
}

esp_err_t config_set_crash_log(const config_crash_log_t *log)
{
    if (!initialized || log == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err;
    
    // Write crash log as blob
    err = nvs_set_blob(sys_nvs_handle, "crash_log", log, sizeof(config_crash_log_t));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write crash log: %s", esp_err_to_name(err));
        return err;
    }
    
    // Commit changes
    err = nvs_commit(sys_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit crash log: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "Crash log saved: timestamp=%lu, reset_reason=%d",
             log->timestamp, log->reset_reason);
    
    return ESP_OK;
}

esp_err_t config_factory_reset(void)
{
    if (!initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "Factory reset requested - erasing WiFi and App namespaces");
    
    esp_err_t err;
    
    // Erase WiFi namespace
    err = nvs_erase_all(wifi_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase WiFi namespace: %s", esp_err_to_name(err));
        return err;
    }
    err = nvs_commit(wifi_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit WiFi namespace erase: %s", esp_err_to_name(err));
        return err;
    }
    
    // Erase App namespace
    err = nvs_erase_all(app_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase App namespace: %s", esp_err_to_name(err));
        return err;
    }
    err = nvs_commit(app_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit App namespace erase: %s", esp_err_to_name(err));
        return err;
    }
    
    // Preserve System namespace (crash logs, boot count)
    ESP_LOGI(TAG, "Factory reset complete - System namespace preserved");
    
    return ESP_OK;
}
