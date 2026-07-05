/**
 * @file hal_led.cpp
 * @brief HAL LED Abstraction Layer
 *
 * Provides unified LED API supporting GPIO and WS2812 types.
 * No-op when LED disabled in configuration.
 */

#include "hal_led.h"
#include "sdkconfig.h"

#include <esp_log.h>
#include <driver/gpio.h>
#include <led_strip.h>

static const char *TAG = "HAL:LED";
static led_strip_handle_t s_led_strip = nullptr;
static bool s_led_enabled = false;

esp_err_t hal_led_init(void)
{
#ifdef CONFIG_BOTIEYES_LED_TYPE_NONE
    ESP_LOGI(TAG, "LED disabled in configuration");
    return ESP_ERR_NOT_SUPPORTED;
#endif

#ifdef CONFIG_BOTIEYES_LED_TYPE_GPIO
    ESP_LOGI(TAG, "Initializing GPIO LED on pin %d", CONFIG_BOTIEYES_LED_GPIO_PIN);

    gpio_config_t io_conf = {};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_BOTIEYES_LED_GPIO_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "GPIO LED init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_set_level((gpio_num_t)CONFIG_BOTIEYES_LED_GPIO_PIN, 0);
    s_led_enabled = true;
    ESP_LOGI(TAG, "GPIO LED initialized");
    return ESP_OK;
#endif

#ifdef CONFIG_BOTIEYES_LED_TYPE_WS2812
    ESP_LOGI(TAG, "Initializing WS2812 LED on GPIO %d", CONFIG_BOTIEYES_LED_WS2812_GPIO);

    led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BOTIEYES_LED_WS2812_GPIO,
        .max_leds = 1,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
        .flags = {
            .invert_out = false}};

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10 MHz
        .mem_block_symbols = 0,
        .flags = {
            .with_dma = false}};

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_led_strip);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "WS2812 LED init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    led_strip_clear(s_led_strip);
    s_led_enabled = true;
    ESP_LOGI(TAG, "WS2812 LED initialized");
    return ESP_OK;
#endif

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t hal_led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led_enabled)
    {
        return ESP_ERR_NOT_SUPPORTED;
    }

#ifdef CONFIG_BOTIEYES_LED_TYPE_GPIO
    // For GPIO LED, turn on if any component is non-zero
    bool on = (r > 0 || g > 0 || b > 0);
    gpio_set_level((gpio_num_t)CONFIG_BOTIEYES_LED_GPIO_PIN, on ? 1 : 0);
    return ESP_OK;
#endif

#ifdef CONFIG_BOTIEYES_LED_TYPE_WS2812
    if (s_led_strip == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = led_strip_set_pixel(s_led_strip, 0, r, g, b);
    if (ret != ESP_OK)
    {
        return ret;
    }

    return led_strip_refresh(s_led_strip);
#endif

    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t hal_led_off(void)
{
    return hal_led_set_color(0, 0, 0);
}
