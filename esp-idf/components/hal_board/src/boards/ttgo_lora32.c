/**
 * @file ttgo_lora32.c
 * @brief TTGO LoRa32 Board Configuration
 * 
 * ESP32 with integrated SSD1306 128x64 OLED (I2C)
 * Hardware: ESP32-PICO-D4 + SX1276 LoRa + SSD1306 OLED
 */

#include "sdkconfig.h"

#ifdef CONFIG_BOTIEYES_BOARD_TTGO_LORA32

#include <driver/gpio.h>

/**
 * @brief TTGO LoRa32 pin configuration
 * 
 * Display (I2C):
 * - SDA: GPIO4
 * - SCL: GPIO15
 * - RST: GPIO16
 * - Address: 0x3C
 * - Frequency: 400 kHz
 * 
 * LED (GPIO):
 * - Pin: GPIO2 (onboard LED)
 */
typedef struct {
    // Display configuration
    struct {
        int sda;
        int scl;
        int rst;
        uint32_t freq_hz;
        uint8_t address;
    } i2c_display;
    
    // LED configuration
    struct {
        int gpio_pin;
    } led;
} ttgo_lora32_config_t;

/**
 * @brief Get TTGO LoRa32 board configuration
 */
const ttgo_lora32_config_t* ttgo_lora32_get_config(void)
{
    static const ttgo_lora32_config_t config = {
        .i2c_display = {
            .sda = 4,
            .scl = 15,
            .rst = 16,
            .freq_hz = 400000,
            .address = 0x3C
        },
        .led = {
            .gpio_pin = 2
        }
    };
    
    return &config;
}

#endif // CONFIG_BOTIEYES_BOARD_TTGO_LORA32
