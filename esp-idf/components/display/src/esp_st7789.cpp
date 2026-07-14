#include "esp_st7789.h"

#ifdef CONFIG_DISPLAY_TYPE_ST7789_SPI

#include "st7789.h" // Include actual TFT_t definition
#include "sdkconfig.h"
#include "esp_log.h"
#include <cstring>
#include <new> // For std::nothrow

static const char *TAG = "esp_st7789";

namespace BotiEyes
{
    namespace display
    {

        ESP_ST7789::ESP_ST7789(uint16_t w, uint16_t h)
            : Adafruit_GFX(w, h),
              _dev(new (std::nothrow) TFT_t()),
              _frame_buffer(nullptr),
              _buffer_size(0),
              _initialized(false),
              _use_frame_buffer(false)
        {
            if (_dev)
            {
                memset(_dev, 0, sizeof(TFT_t));
            }
            else
            {
                ESP_LOGE(TAG, "Failed to allocate TFT_t device structure");
            }

#ifdef CONFIG_FRAME_BUFFER
            // Calculate frame buffer size (RGB565 = 2 bytes per pixel)
            _buffer_size = (uint32_t)w * (uint32_t)h;
            uint32_t buffer_bytes = _buffer_size * sizeof(uint16_t);

            // Sanity check: ESP32 SRAM is limited (~300KB free after system overhead)
            // For 240x135 @ RGB565: 240*135*2 = 64,800 bytes (~63KB) - safe
            // For 320x240 @ RGB565: 320*240*2 = 153,600 bytes (~150KB) - borderline
            const uint32_t MAX_SAFE_BUFFER_SIZE = 160 * 1024; // 160KB limit
            if (buffer_bytes > MAX_SAFE_BUFFER_SIZE)
            {
                ESP_LOGW(TAG, "Frame buffer size (%lu bytes) exceeds safe limit (%lu bytes) - allocation may fail",
                         buffer_bytes, MAX_SAFE_BUFFER_SIZE);
            }

            // Allocate RGB565 frame buffer (using new instead of malloc for C++)
            _frame_buffer = new (std::nothrow) uint16_t[_buffer_size];
            if (_frame_buffer)
            {
                _use_frame_buffer = true;
                // Initialize buffer to black (0x0000 in RGB565)
                memset(_frame_buffer, 0, buffer_bytes);
                ESP_LOGI(TAG, "Frame buffer allocated: %lu bytes (%dx%d RGB565)",
                         buffer_bytes, w, h);
            }
            else
            {
                ESP_LOGE(TAG, "Frame buffer allocation failed! Falling back to direct rendering.");
                ESP_LOGE(TAG, "  Requested: %lu bytes (%dx%d RGB565)", buffer_bytes, w, h);
                ESP_LOGE(TAG, "  This will reduce rendering speed but functionality is preserved.");
                _use_frame_buffer = false;
                _buffer_size = 0; // Reset buffer size since allocation failed
            }
#else
            ESP_LOGI(TAG, "Frame buffer disabled (Kconfig), using direct rendering");
#endif
        }

        ESP_ST7789::~ESP_ST7789()
        {
            if (_frame_buffer)
            {
                delete[] _frame_buffer;
            }
            if (_dev)
            {
                delete _dev;
            }
        }

        bool ESP_ST7789::beginSpi(int16_t mosi_pin, int16_t sclk_pin, int16_t cs_pin,
                                  int16_t dc_pin, int16_t rst_pin, int16_t bl_pin,
                                  uint16_t offsetx, uint16_t offsety)
        {
            // Prevent re-initialization
            if (_initialized)
            {
                ESP_LOGW(TAG, "ST7789 already initialized, skipping");
                return true;
            }

            ESP_LOGI(TAG, "Initializing ST7789 via SPI (nopnop2002 component)");
            ESP_LOGI(TAG, "  GPIO: MOSI=%d SCLK=%d CS=%d DC=%d RST=%d BL=%d",
                     mosi_pin, sclk_pin, cs_pin, dc_pin, rst_pin, bl_pin);
            ESP_LOGI(TAG, "  Resolution: %dx%d, Offset: (%d, %d)",
                     WIDTH, HEIGHT, offsetx, offsety);

            // Validate GPIO pins (ESP32 GPIO range: 0-48, -1 for disabled)
            const int16_t GPIO_MAX = 48;
            if ((mosi_pin < -1 || mosi_pin > GPIO_MAX) ||
                (sclk_pin < -1 || sclk_pin > GPIO_MAX) ||
                (cs_pin < -1 || cs_pin > GPIO_MAX) ||
                (dc_pin < -1 || dc_pin > GPIO_MAX) ||
                (rst_pin < -1 || rst_pin > GPIO_MAX) ||
                (bl_pin < -1 || bl_pin > GPIO_MAX))
            {
                ESP_LOGE(TAG, "Invalid GPIO pin(s): must be -1 or 0-%d", GPIO_MAX);
                return false;
            }

            // Validate required pins (MOSI, SCLK, DC are mandatory for SPI displays)
            if (mosi_pin < 0 || sclk_pin < 0 || dc_pin < 0)
            {
                ESP_LOGE(TAG, "Required GPIO pins missing: MOSI=%d SCLK=%d DC=%d (must be >= 0)",
                         mosi_pin, sclk_pin, dc_pin);
                return false;
            }

            // Set SPI clock speed from Kconfig
#ifdef CONFIG_ST7789_SPI_CLOCK_MHZ
            int clock_speed = CONFIG_ST7789_SPI_CLOCK_MHZ * 1000000;
            spi_clock_speed(clock_speed);
            ESP_LOGI(TAG, "  SPI Clock: %d MHz", CONFIG_ST7789_SPI_CLOCK_MHZ);
#endif

            // Initialize SPI master and ST7789 controller
            // Note: nopnop2002 component doesn't return error codes, but logs internally
            spi_master_init(_dev, mosi_pin, sclk_pin, cs_pin, dc_pin, rst_pin, bl_pin);

            // Verify SPI handle was initialized (basic sanity check)
            if (_dev->_SPIHandle == NULL)
            {
                ESP_LOGE(TAG, "SPI master initialization failed: _SPIHandle is NULL");
                return false;
            }

            lcdInit(_dev, WIDTH, HEIGHT, offsetx, offsety);

            // Configure MADCTL (Memory Access Control) for 240×135 landscape
            // MADCTL=0x70: MX=1, MV=1, ML=1 (common for 240×135 landscape)
            // Try different values if display still shows incorrectly:
            //   0x00 = default (portrait)
            //   0x60 = MX=1, MV=1 (rotate 90°)
            //   0x70 = MX=1, MV=1, ML=1 (rotate 90° + vertical flip)
            //   0xA0 = MY=1, MV=1 (rotate 270°)
            spi_master_write_command(_dev, 0x36);   // MADCTL
            spi_master_write_data_byte(_dev, 0x70); // Try 0x70 for landscape
            ESP_LOGI(TAG, "MADCTL set to 0x70 (240×135 landscape mode)");

            // Verify display dimensions were set correctly
            if (_dev->_width != WIDTH || _dev->_height != HEIGHT)
            {
                ESP_LOGE(TAG, "LCD initialization failed: dimensions mismatch (expected %dx%d, got %dx%d)",
                         WIDTH, HEIGHT, _dev->_width, _dev->_height);
                return false;
            }

            // Enable frame buffer mode in native driver if configured
            if (_use_frame_buffer && _frame_buffer)
            {
                lcdEnableFrameBuffer(_dev);
                _dev->_frame_buffer = _frame_buffer;
                ESP_LOGI(TAG, "Frame buffer mode enabled in native driver");
            }

            _initialized = true;
            ESP_LOGI(TAG, "ST7789 initialization complete");

            // Turn on backlight if configured
            if (bl_pin >= 0)
            {
                backlightOn();
            }

            return true;
        }

        void ESP_ST7789::drawPixel(int16_t x, int16_t y, uint16_t color)
        {
            // Bounds checking
            if (x < 0 || x >= WIDTH || y < 0 || y >= HEIGHT)
            {
                return;
            }

            if (_use_frame_buffer && _frame_buffer)
            {
                // Write to frame buffer (RGB565 format)
                uint32_t index = (uint32_t)y * WIDTH + x;
                _frame_buffer[index] = color;
            }
            else
            {
                // Direct rendering (slower but lower memory)
                lcdDrawPixel(_dev, (uint16_t)x, (uint16_t)y, color);
            }
        }

        void ESP_ST7789::flush()
        {
            if (!_initialized)
            {
                return;
            }

            if (_use_frame_buffer && _frame_buffer)
            {
                // Flush entire frame buffer using native driver's optimized transfer
                lcdDrawFinish(_dev);
            }
            // Direct rendering mode has no flush (pixels already sent)
        }

        void ESP_ST7789::display()
        {
            flush();
        }

        void ESP_ST7789::clearDisplay()
        {
            if (_use_frame_buffer && _frame_buffer)
            {
                // Clear frame buffer (black = 0x0000 in RGB565)
                memset(_frame_buffer, 0, _buffer_size * sizeof(uint16_t));
            }

            if (_initialized)
            {
                // Clear hardware display
                lcdFillScreen(_dev, BLACK);
            }
        }

        void ESP_ST7789::backlightOn()
        {
            if (_initialized && _dev->_bl >= 0)
            {
                lcdBacklightOn(_dev);
            }
        }

        void ESP_ST7789::backlightOff()
        {
            if (_initialized && _dev->_bl >= 0)
            {
                lcdBacklightOff(_dev);
            }
        }

    } // namespace display
} // namespace BotiEyes

#endif // CONFIG_DISPLAY_TYPE_ST7789_SPI
