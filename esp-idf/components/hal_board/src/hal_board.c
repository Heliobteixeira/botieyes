/**
 * @file hal_board.c
 * @brief Hardware Abstraction Layer - Board Initialization
 */

#include "hal_board.h"
#include "hal_display.h"
#include "hal_led.h"
#include "sdkconfig.h"

#include <esp_log.h>

static const char *TAG = "HAL:BOARD";

esp_err_t hal_board_init(void)
{
    esp_err_t ret;
    
    // T109: Log detected board configuration (FR-046)
#ifdef CONFIG_BOTIEYES_BOARD_TTGO_LORA32
    ESP_LOGI(TAG, "Initializing TTGO LoRa32 board");
#elif defined(CONFIG_BOTIEYES_BOARD_ESP32S3_SPI)
    ESP_LOGI(TAG, "Initializing ESP32-S3 SPI board");
#else
    #error "No board selected in Kconfig"
#endif

    // Log display configuration
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
    ESP_LOGI(TAG, "Display: I2C SSD1306 (SDA=%d, SCL=%d, RST=%d, ADDR=0x%02x, FREQ=%d Hz)",
             CONFIG_BOTIEYES_OLED_I2C_SDA, CONFIG_BOTIEYES_OLED_I2C_SCL, CONFIG_BOTIEYES_OLED_I2C_RST,
             CONFIG_BOTIEYES_OLED_I2C_ADDRESS, CONFIG_BOTIEYES_OLED_I2C_FREQ);
#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_SPI)
    ESP_LOGI(TAG, "Display: SPI SSD1306 (MOSI=%d, SCK=%d, CS=%d, DC=%d, RST=%d, FREQ=%d Hz)",
             CONFIG_BOTIEYES_OLED_SPI_MOSI, CONFIG_BOTIEYES_OLED_SPI_SCK, CONFIG_BOTIEYES_OLED_SPI_CS,
             CONFIG_BOTIEYES_OLED_SPI_DC, CONFIG_BOTIEYES_OLED_SPI_RST, CONFIG_BOTIEYES_OLED_SPI_FREQ);
#endif

    // Log LED configuration
#ifdef CONFIG_BOTIEYES_LED_TYPE_NONE
    ESP_LOGI(TAG, "LED: None (disabled)");
#elif defined(CONFIG_BOTIEYES_LED_TYPE_GPIO)
    ESP_LOGI(TAG, "LED: GPIO (pin=%d)", CONFIG_BOTIEYES_LED_GPIO_PIN);
#elif defined(CONFIG_BOTIEYES_LED_TYPE_WS2812)
    ESP_LOGI(TAG, "LED: WS2812 (pin=%d)", CONFIG_BOTIEYES_LED_WS2812_PIN);
#endif

    // Initialize display
    ESP_LOGI(TAG, "Initializing display");
    if (hal_display_init() == NULL) {
        ESP_LOGE(TAG, "Display initialization failed (check display HAL logs for details)");
        return ESP_FAIL;
    }
    
    // Initialize LED
    ESP_LOGI(TAG, "Initializing LED");
    ret = hal_led_init();
    if (ret != ESP_OK && ret != ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGE(TAG, "LED initialization failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }
    
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGI(TAG, "LED disabled in configuration");
    }
    
    ESP_LOGI(TAG, "Board initialization complete");
    return ESP_OK;
}
