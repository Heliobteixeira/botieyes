/**
 * @file hal_led.h
 * @brief Hardware Abstraction Layer - LED Interface
 * 
 * Abstracts LED hardware differences (GPIO vs WS2812).
 * Provides unified API for status LED control.
 */

#pragma once

#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize status LED
 * 
 * Initializes GPIO or WS2812 LED based on Kconfig selection.
 * No-op if LED disabled in configuration.
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NOT_SUPPORTED if LED disabled
 *         ESP_FAIL if LED init fails
 * 
 * @note Must be called after hal_board_init()
 * @note Not thread-safe, call from main task only
 */
esp_err_t hal_led_init(void);

/**
 * @brief Set LED color
 * 
 * @param r Red component 0-255
 * @param g Green component 0-255
 * @param b Blue component 0-255
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NOT_SUPPORTED if LED disabled
 * 
 * @note For GPIO LEDs, any non-zero value turns LED on
 * @note Thread-safe
 */
esp_err_t hal_led_set_color(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Turn LED off
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NOT_SUPPORTED if LED disabled
 * 
 * @note Thread-safe
 */
esp_err_t hal_led_off(void);

#ifdef __cplusplus
}
#endif
