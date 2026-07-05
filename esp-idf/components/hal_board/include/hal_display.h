/**
 * @file hal_display.h
 * @brief Hardware Abstraction Layer - Display Interface
 *
 * Abstracts display hardware differences (I2C vs SPI).
 * Provides unified API for OLED display control.
 */

#pragma once

#include <esp_err.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Display handle (opaque)
     */
    typedef void *hal_display_handle_t;

    /**
     * @brief Initialize display
     *
     * Initializes I2C or SPI display based on Kconfig selection.
     * Creates display mutex for thread-safe access.
     *
     * @return Display handle on success, NULL on failure
     *
     * @note Must be called after hal_board_init()
     * @note Not thread-safe, call from main task only
     */
    hal_display_handle_t hal_display_init(void);

    /**
     * @brief Acquire display lock
     *
     * Must be called before any display operations.
     * Prevents concurrent access to display hardware.
     *
     * @param timeout_ms Maximum time to wait for lock (0 = no wait, portMAX_DELAY = wait forever)
     *
     * @return ESP_OK if lock acquired
     *         ESP_ERR_TIMEOUT if timeout exceeded
     *
     * @note Always pair with hal_display_unlock()
     * @note Thread-safe
     */
    esp_err_t hal_display_lock(uint32_t timeout_ms);

    /**
     * @brief Release display lock
     *
     * Must be called after display operations complete.
     *
     * @return ESP_OK on success
     *
     * @note Thread-safe
     */
    esp_err_t hal_display_unlock(void);

    /**
     * @brief Clear display
     *
     * @param handle Display handle from hal_display_init()
     *
     * @return ESP_OK on success
     *
     * @note Must hold display lock before calling
     * @note Thread-safe (if caller holds lock)
     */
    esp_err_t hal_display_clear(hal_display_handle_t handle);

    /**
     * @brief Flush display buffer to hardware
     *
     * @param handle Display handle from hal_display_init()
     *
     * @return ESP_OK on success
     *
     * @note Must hold display lock before calling
     * @note Thread-safe (if caller holds lock)
     */
    esp_err_t hal_display_flush(hal_display_handle_t handle);

    /**
     * @brief Set display brightness
     *
     * @param handle Display handle from hal_display_init()
     * @param brightness Brightness level 0-255
     *
     * @return ESP_OK on success
     *
     * @note Must hold display lock before calling
     * @note Thread-safe (if caller holds lock)
     */
    esp_err_t hal_display_set_brightness(hal_display_handle_t handle, uint8_t brightness);

#ifdef __cplusplus
}
#endif
