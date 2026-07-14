#include "display_init.h"
#include "esp_idf_version.h"
#include <type_traits>

// Conditionally include drivers based on display type selection
#if defined(CONFIG_DISPLAY_TYPE_SSD1306_I2C) || defined(CONFIG_DISPLAY_TYPE_SSD1306_SPI)
#include "ssd1306.h"
#endif

#if defined(CONFIG_DISPLAY_TYPE_ST7789_SPI)
#include "esp_st7789.h"
#endif

#include "sdkconfig.h"
#include "esp_log.h"
#include <cstring>

static const char *TAG = "display_init";

// ============================================================================
// Compile-Time Interface Verification (Feature 005, Task T029)
// ============================================================================
// Verify that all display adapters inherit from required base classes.
// This ensures polymorphic compatibility at compile time.
//
// If any of these assertions fail, the driver implementation is incomplete
// and must be fixed before the build can succeed.

#if defined(CONFIG_DISPLAY_TYPE_ST7789_SPI)
// Verify ESP_ST7789 implements required interfaces
static_assert(std::is_base_of<Adafruit_GFX, BotiEyes::display::ESP_ST7789>::value,
              "ESP_ST7789 must inherit from Adafruit_GFX for hardware abstraction");
static_assert(std::is_base_of<::BotiEyes::DisplayFlushable, BotiEyes::display::ESP_ST7789>::value,
              "ESP_ST7789 must inherit from BotiEyes::DisplayFlushable for buffer management");
#endif

// Note: Ssd1306Gfx is verified at class definition (inline below) since it's in
// an anonymous namespace and not externally visible. The adapter inherits from
// both Adafruit_GFX and DisplayFlushable as required.

// ============================================================================

namespace
{
#if defined(CONFIG_DISPLAY_TYPE_SSD1306_I2C) || defined(CONFIG_DISPLAY_TYPE_SSD1306_SPI)
    // SSD1306 adapter class (only compiled if SSD1306 selected)
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
#endif

    // Global display pointer (polymorphic - can point to SSD1306 or ST7789)
    Adafruit_GFX *g_display = nullptr;
    ::BotiEyes::DisplayFlushable *g_flushable = nullptr;
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

            ESP_LOGI(TAG, "Initializing display based on Kconfig selection");

#if defined(CONFIG_DISPLAY_TYPE_ST7789_SPI)
            // ST7789 Color TFT (TTGO T-Display)
            ESP_LOGI(TAG, "Display type: ST7789 SPI");
            auto *st7789 = new ESP_ST7789(CONFIG_WIDTH, CONFIG_HEIGHT);

            if (!st7789->beginSpi(
                    CONFIG_MOSI_GPIO,
                    CONFIG_SCLK_GPIO,
                    CONFIG_CS_GPIO,
                    CONFIG_DC_GPIO,
                    CONFIG_RESET_GPIO,
                    CONFIG_BL_GPIO,
                    CONFIG_OFFSETX,
                    CONFIG_OFFSETY))
            {
                ESP_LOGE(TAG, "ST7789 SPI initialization failed");
                delete st7789;
                return nullptr;
            }

            g_display = st7789;
            g_flushable = st7789;
            st7789->clearDisplay();
            st7789->flush();

#elif defined(CONFIG_DISPLAY_TYPE_SSD1306_SPI)
            // SSD1306 Monochrome OLED (SPI)
            ESP_LOGI(TAG, "Display type: SSD1306 SPI");
            auto *ssd1306 = new Ssd1306Gfx(128, 64);

            if (!ssd1306->beginSpi(
                    CONFIG_BOTIEYES_OLED_SPI_MOSI,
                    CONFIG_BOTIEYES_OLED_SPI_SCK,
                    CONFIG_BOTIEYES_OLED_SPI_CS,
                    CONFIG_BOTIEYES_OLED_SPI_DC,
                    CONFIG_BOTIEYES_OLED_SPI_RST))
            {
                ESP_LOGE(TAG, "SSD1306 SPI initialization failed");
                delete ssd1306;
                return nullptr;
            }

            g_display = ssd1306;
            g_flushable = ssd1306;
            ssd1306->clearDisplay();
            ssd1306->flush();

#elif defined(CONFIG_DISPLAY_TYPE_SSD1306_I2C)
            // SSD1306 Monochrome OLED (I2C) - TTGO LoRa32 default
            ESP_LOGI(TAG, "Display type: SSD1306 I2C");
            auto *ssd1306 = new Ssd1306Gfx(128, 64);

            if (!ssd1306->beginI2C(
                    CONFIG_BOTIEYES_OLED_I2C_SDA,
                    CONFIG_BOTIEYES_OLED_I2C_SCL,
                    CONFIG_BOTIEYES_OLED_I2C_RST))
            {
                ESP_LOGE(TAG, "SSD1306 I2C initialization failed");
                delete ssd1306;
                return nullptr;
            }

            g_display = ssd1306;
            g_flushable = ssd1306;
            ssd1306->clearDisplay();
            ssd1306->flush();

#else
#error "No display type selected! Run idf.py menuconfig → Display Type Selection"
#endif

            ESP_LOGI(TAG, "Display initialized successfully");
            return g_display;
        }

        ::BotiEyes::DisplayFlushable *initializeDisplayFlushable()
        {
            return g_flushable;
        }

        DisplayConfig createDisplayConfig()
        {
            DisplayConfig config = {};

#if defined(CONFIG_DISPLAY_TYPE_ST7789_SPI)
            config.type = DISPLAY_ST7789_240x135; // TODO: Add to DisplayConfig.h enum
            config.width = CONFIG_WIDTH;
            config.height = CONFIG_HEIGHT;
            config.protocol = PROTOCOL_SPI_HW;
            config.i2cAddress = 0;
            config.cs_pin = CONFIG_CS_GPIO;
            config.dc_pin = CONFIG_DC_GPIO;
            config.reset_pin = CONFIG_RESET_GPIO;

#elif defined(CONFIG_DISPLAY_TYPE_SSD1306_SPI)
            config.type = DISPLAY_SSD1306_128x64;
            config.width = 128;
            config.height = 64;
            config.protocol = PROTOCOL_SPI_HW;
            config.i2cAddress = 0;
            config.cs_pin = CONFIG_BOTIEYES_OLED_SPI_CS;
            config.dc_pin = CONFIG_BOTIEYES_OLED_SPI_DC;
            config.reset_pin = CONFIG_BOTIEYES_OLED_SPI_RST;

#elif defined(CONFIG_DISPLAY_TYPE_SSD1306_I2C)
            config.type = DISPLAY_SSD1306_128x64;
            config.width = 128;
            config.height = 64;
            config.protocol = PROTOCOL_I2C;
            config.i2cAddress = CONFIG_BOTIEYES_OLED_I2C_ADDRESS;
            config.cs_pin = -1;
            config.dc_pin = -1;
            config.reset_pin = CONFIG_BOTIEYES_OLED_I2C_RST;
#endif

            return config;
        }

    } // namespace display
} // namespace BotiEyes
