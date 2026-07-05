/**
 * @file esp32s3_spi.c
 * @brief ESP32-S3 SPI Board Configuration
 * 
 * ESP32-S3 with SSD1306 128x64 OLED (SPI) and WS2812 LED
 */

#include "sdkconfig.h"

#ifdef CONFIG_BOTIEYES_BOARD_ESP32S3_SPI

#include <driver/gpio.h>

/**
 * @brief ESP32-S3 SPI board pin configuration
 * 
 * Display (SPI):
 * - MOSI: GPIO11
 * - SCK: GPIO12
 * - CS: GPIO10
 * - DC: GPIO13
 * - RST: GPIO14
 * - Frequency: 10 MHz
 * 
 * LED (WS2812):
 * - Pin: GPIO38
 */
typedef struct {
    // Display configuration
    struct {
        int mosi;
        int sck;
        int cs;
        int dc;
        int rst;
        uint32_t freq_hz;
    } spi_display;
    
    // LED configuration
    struct {
        int ws2812_gpio;
    } led;
} esp32s3_spi_config_t;

/**
 * @brief Get ESP32-S3 SPI board configuration
 */
const esp32s3_spi_config_t* esp32s3_spi_get_config(void)
{
    static const esp32s3_spi_config_t config = {
        .spi_display = {
            .mosi = 11,
            .sck = 12,
            .cs = 10,
            .dc = 13,
            .rst = 14,
            .freq_hz = 10000000
        },
        .led = {
            .ws2812_gpio = 38
        }
    };
    
    return &config;
}

#endif // CONFIG_BOTIEYES_BOARD_ESP32S3_SPI
