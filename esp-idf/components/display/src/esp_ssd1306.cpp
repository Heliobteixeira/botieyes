#include "esp_ssd1306.h"
#include "esp_idf_version.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "esp_ssd1306";

namespace BotiEyes
{
    namespace display
    {

        ESP_SSD1306::ESP_SSD1306(uint8_t w, uint8_t h)
            : Adafruit_GFX(w, h), _buffer(nullptr), _buffer_size(0), _initialized(false)
        {
            _buffer_size = ((w * h) + 7) / 8;
            _buffer = new uint8_t[_buffer_size];
            clearDisplay();
            memset(&_dev, 0, sizeof(_dev));
        }

        ESP_SSD1306::~ESP_SSD1306()
        {
            delete[] _buffer;
        }

        bool ESP_SSD1306::begin(uint8_t i2c_addr, int sda_pin, int scl_pin, int rst_pin)
        {
            (void)i2c_addr;
            ESP_LOGI(TAG, "Initializing SSD1306 via I2C (component-backed)");
            i2c_master_init(&_dev, sda_pin, scl_pin, rst_pin);
            esp_err_t err = ssd1306_init(&_dev, WIDTH, HEIGHT);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "ssd1306_init(I2C) failed: %s", esp_err_to_name(err));
                return false;
            }
            _initialized = true;
            return true;
        }

        bool ESP_SSD1306::beginSpi(int mosi_pin, int sclk_pin, int cs_pin, int dc_pin, int rst_pin)
        {
            ESP_LOGI(TAG, "Initializing SSD1306 via SPI (component-backed)");
            spi_clock_speed(CONFIG_BOTIEYES_OLED_SPI_FREQ);
            spi_master_init(&_dev, mosi_pin, sclk_pin, cs_pin, dc_pin, rst_pin);
            esp_err_t err = ssd1306_init(&_dev, WIDTH, HEIGHT);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "ssd1306_init(SPI) failed: %s", esp_err_to_name(err));
                return false;
            }
            _initialized = true;
            return true;
        }

        void ESP_SSD1306::drawPixel(int16_t x, int16_t y, uint16_t color)
        {
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
            {
                return;
            }

            // Calculate byte position in buffer (row-major, 1 bit per pixel)
            uint16_t byte_index = x + (y / 8) * WIDTH;
            uint8_t bit_mask = 1 << (y % 8);

            if (color)
            {
                _buffer[byte_index] |= bit_mask; // Set pixel
            }
            else
            {
                _buffer[byte_index] &= ~bit_mask; // Clear pixel
            }
        }

        void ESP_SSD1306::display()
        {
            if (!_initialized)
            {
                return;
            }

            ssd1306_set_buffer(&_dev, _buffer);
            ssd1306_show_buffer(&_dev);
        }

        void ESP_SSD1306::clearDisplay()
        {
            memset(_buffer, 0, _buffer_size);
        }

    } // namespace display
} // namespace BotiEyes
