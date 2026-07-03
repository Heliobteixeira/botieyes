#include "display_init.h"
#include "esp_idf_version.h"
#include "ssd1306.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "display_init";

namespace
{
    class Ssd1306Gfx : public Adafruit_GFX, public BotiEyes::DisplayFlushable
    {
    public:
        Ssd1306Gfx(uint8_t w, uint8_t h)
            : Adafruit_GFX(w, h), _buffer(nullptr), _buffer_size(0), _initialized(false)
        {
            _buffer_size = ((w * h) + 7) / 8;
            _buffer = new uint8_t[_buffer_size];
            memset(&_dev, 0, sizeof(_dev));
            clearDisplay();
        }

        ~Ssd1306Gfx() override
        {
            delete[] _buffer;
        }

        bool beginI2C(int sda_pin, int scl_pin, int rst_pin)
        {
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

        bool beginSpi(int mosi_pin, int sclk_pin, int cs_pin, int dc_pin, int rst_pin)
        {
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

        void drawPixel(int16_t x, int16_t y, uint16_t color) override
        {
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
            {
                return;
            }

            uint16_t byte_index = x + (y / 8) * WIDTH;
            uint8_t bit_mask = 1 << (y % 8);

            if (color)
            {
                _buffer[byte_index] |= bit_mask;
            }
            else
            {
                _buffer[byte_index] &= ~bit_mask;
            }
        }

        void flush() override
        {
            if (!_initialized)
            {
                return;
            }

            ssd1306_set_buffer(&_dev, _buffer);
            ssd1306_show_buffer(&_dev);
        }

        void display()
        {
            flush();
        }

        void clearDisplay()
        {
            if (_buffer)
            {
                memset(_buffer, 0, _buffer_size);
            }
        }

        SSD1306_t _dev;
        uint8_t *_buffer;
        uint16_t _buffer_size;
        bool _initialized;
    };
    Ssd1306Gfx *g_display = nullptr;
} // namespace

namespace BotiEyes
{
    namespace display
    {

        Adafruit_GFX *initializeDisplay()
        {
            if (g_display != nullptr)
            {
                return g_display;
            }

            Ssd1306Gfx *display = nullptr;

#if defined(CONFIG_SPI_INTERFACE)
            ESP_LOGI(TAG, "Initializing SSD1306 128x64 SPI display");
            display = new Ssd1306Gfx(128, 64);
            if (!display->beginSpi(CONFIG_MOSI_GPIO,
                                   CONFIG_SCLK_GPIO,
                                   CONFIG_CS_GPIO,
                                   CONFIG_DC_GPIO,
                                   CONFIG_RESET_GPIO))
            {
                ESP_LOGE(TAG, "SSD1306 SPI initialization failed");
                delete display;
                return nullptr;
            }
#elif defined(CONFIG_I2C_INTERFACE)
            ESP_LOGI(TAG, "Initializing SSD1306 128x64 I2C display");
            if (!display->beginI2C(CONFIG_SDA_GPIO,
                                   CONFIG_SCL_GPIO,
                                   CONFIG_RESET_GPIO))
            {
                ESP_LOGE(TAG, "SSD1306 I2C initialization failed");
                delete display;
                return nullptr;
            }
#else
#error "OLED protocol not configured. Run idf.py menuconfig."
#endif

            g_display = display;
            ESP_LOGI(TAG, "Display initialized successfully");
            display->clearDisplay();
            display->flush();
            return g_display;
        }

        ::BotiEyes::DisplayFlushable *initializeDisplayFlushable()
        {
            return g_display;
        }

        DisplayConfig createDisplayConfig()
        {
            DisplayConfig config = {};

            config.type = DISPLAY_SSD1306_128x64;
            config.width = 128;
            config.height = 64;

#if defined(CONFIG_SPI_INTERFACE)
            config.protocol = PROTOCOL_SPI_HW;
            config.i2cAddress = 0;
            config.cs_pin = CONFIG_CS_GPIO;
            config.dc_pin = CONFIG_DC_GPIO;
            config.reset_pin = CONFIG_RESET_GPIO;
#elif defined(CONFIG_I2C_INTERFACE)
            config.protocol = PROTOCOL_I2C;
            config.i2cAddress = 0x3C;
            config.cs_pin = -1;
            config.dc_pin = -1;
            config.reset_pin = CONFIG_RESET_GPIO;
#endif

            return config;
        }

    } // namespace display
} // namespace BotiEyes
