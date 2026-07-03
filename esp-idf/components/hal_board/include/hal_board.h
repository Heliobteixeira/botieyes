/**
 * @file hal_board.h
 * @brief Hardware Abstraction Layer - Board Initialization
 *
 * Board-level initialization and configuration.
 */

#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Initialize board hardware
     *
     * Selects board configuration via Kconfig, initializes display and LED
     * based on board type.
     *
     * @return ESP_OK on success
     *         ESP_FAIL if display or LED init fails
     *
     * @note Must be called before using hal_display_* or hal_led_* functions
     * @note Not thread-safe, call from main task only
     */
    esp_err_t hal_board_init(void);

#ifdef __cplusplus
}
#endif
