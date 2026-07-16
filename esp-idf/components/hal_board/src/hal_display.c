/**
 * @file hal_display.c
 * @brief HAL Display Abstraction Layer
 * 
 * Provides unified display API that selects I2C or SPI implementation
 * based on Kconfig at compile time.
 * 
 * NOTE: This HAL layer is currently incomplete. The display component
 * (components/display) provides the working implementation via
 * display_init.cpp which uses CONFIG_DISPLAY_TYPE_* Kconfig settings.
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
// TODO: Implement these functions when HAL layer is completed
#if defined(CONFIG_DISPLAY_TYPE_SSD1306_I2C)
extern hal_display_handle_t hal_display_i2c_init(void);
#endif

#if defined(CONFIG_DISPLAY_TYPE_SSD1306_SPI) || defined(CONFIG_DISPLAY_TYPE_ST7789_SPI)
extern hal_display_handle_t hal_display_spi_init(void);
#endif

hal_display_handle_t hal_display_init(void)
{
    ESP_LOGI(TAG, "Initializing display abstraction layer (HAL stub)");
    
    // Create display mutex (FR-014)
    g_display_mutex = xSemaphoreCreateMutex();
    if (g_display_mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create display mutex (error=ESP_ERR_NO_MEM)");
        return NULL;
    }
    
    // Select protocol-specific initialization based on new CONFIG_DISPLAY_TYPE_* settings
#if defined(CONFIG_DISPLAY_TYPE_ST7789_SPI)
    ESP_LOGI(TAG, "Display type: ST7789 SPI (HAL layer incomplete, use display component)");
    // g_display_handle = hal_display_spi_init();  // TODO: Implement
    g_display_handle = (hal_display_handle_t)1;  // Stub handle
    
#elif defined(CONFIG_DISPLAY_TYPE_SSD1306_SPI)
    ESP_LOGI(TAG, "Display type: SSD1306 SPI (HAL layer incomplete, use display component)");
    // g_display_handle = hal_display_spi_init();  // TODO: Implement
    g_display_handle = (hal_display_handle_t)1;  // Stub handle
    
#elif defined(CONFIG_DISPLAY_TYPE_SSD1306_I2C)
    ESP_LOGI(TAG, "Display type: SSD1306 I2C (HAL layer incomplete, use display component)");
    // g_display_handle = hal_display_i2c_init();  // TODO: Implement
    g_display_handle = (hal_display_handle_t)1;  // Stub handle
    
#else
    #error "No display type selected in Kconfig. Run 'idf.py menuconfig' → Display Type Selection"
#endif

    if (g_display_handle == NULL) {
        ESP_LOGE(TAG, "Display initialization failed");
        vSemaphoreDelete(g_display_mutex);
        g_display_mutex = NULL;
        return NULL;
    }
    
    ESP_LOGI(TAG, "Display HAL initialized (stub mode - actual display via components/display)");
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
