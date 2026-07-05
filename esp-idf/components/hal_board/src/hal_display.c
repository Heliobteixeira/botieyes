/**
 * @file hal_display.c
 * @brief HAL Display Abstraction Layer
 * 
 * Provides unified display API that selects I2C or SPI implementation
 * based on Kconfig at compile time.
 */

#include "hal_display.h"
#include "sdkconfig.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static const char *TAG = "HAL:DISPLAY";
static SemaphoreHandle_t g_display_mutex = NULL;
static hal_display_handle_t g_display_handle = NULL;

// Forward declarations for protocol-specific implementations
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
extern hal_display_handle_t hal_display_i2c_init(void);
#endif

#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
extern hal_display_handle_t hal_display_spi_init(void);
#endif

hal_display_handle_t hal_display_init(void)
{
    ESP_LOGI(TAG, "Initializing display abstraction layer");
    
    // Create display mutex (FR-014)
    g_display_mutex = xSemaphoreCreateMutex();
    if (g_display_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create display mutex (error=ESP_ERR_NO_MEM)");
        return NULL;
    }
    
    // Select protocol-specific initialization based on Kconfig (FR-050)
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
    ESP_LOGI(TAG, "Using I2C display protocol");
    g_display_handle = hal_display_i2c_init();
#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_SPI)
    ESP_LOGI(TAG, "Using SPI display protocol");
    g_display_handle = hal_display_spi_init();
#else
    #error "No display protocol selected in Kconfig"
#endif

    if (g_display_handle == NULL) {
        ESP_LOGE(TAG, "Display initialization failed (protocol=%s)",
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
                 "I2C"
#else
                 "SPI"
#endif
                 );
        vSemaphoreDelete(g_display_mutex);
        g_display_mutex = NULL;
        return NULL;
    }
    
    ESP_LOGI(TAG, "Display initialized successfully");
    return g_display_handle;
}

esp_err_t hal_display_lock(uint32_t timeout_ms)
{
    if (g_display_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    TickType_t timeout_ticks = (timeout_ms == 0) ? 0 : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(g_display_mutex, timeout_ticks) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }
    
    return ESP_OK;
}

esp_err_t hal_display_unlock(void)
{
    if (g_display_mutex == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    xSemaphoreGive(g_display_mutex);
    return ESP_OK;
}

esp_err_t hal_display_clear(hal_display_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // TODO: Call protocol-specific clear function
    ESP_LOGW(TAG, "Display clear stub - not yet implemented");
    return ESP_OK;
}

esp_err_t hal_display_flush(hal_display_handle_t handle)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // TODO: Call protocol-specific flush function
    ESP_LOGW(TAG, "Display flush stub - not yet implemented");
    return ESP_OK;
}

esp_err_t hal_display_set_brightness(hal_display_handle_t handle, uint8_t brightness)
{
    if (handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // TODO: Call protocol-specific brightness function
    ESP_LOGW(TAG, "Display brightness stub - not yet implemented");
    return ESP_OK;
}
