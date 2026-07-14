#ifndef ESP_ST7789_H
#define ESP_ST7789_H

#include "sdkconfig.h"

#ifdef CONFIG_DISPLAY_TYPE_ST7789_SPI

#include <Adafruit_GFX.h>
#include "esp_err.h"
#include "BotiEyes.h"

// Include C library with proper linkage
extern "C"
{
#include "st7789.h" // Include for TFT_t definition
}

namespace BotiEyes
{
    namespace display
    {

        /**
         * Adafruit_GFX adapter over the nopnop2002 ST7789 component.
         * Wraps the ST7789 color TFT driver to provide hardware abstraction
         * for the BotiEyes rendering layer via Adafruit_GFX interface.
         *
         * Supports both frame buffer (lcdDrawMultiPixels) and direct rendering
         * (lcdDrawPixel) modes based on Kconfig CONFIG_FRAME_BUFFER.
         */
        class ESP_ST7789 : public Adafruit_GFX, public ::BotiEyes::DisplayFlushable
        {
        public:
            /**
             * Constructor
             * @param w Display width in pixels
             * @param h Display height in pixels
             */
            ESP_ST7789(uint16_t w, uint16_t h);
            ~ESP_ST7789() override;

            /**
             * Initialize ST7789 via SPI
             * @param mosi_pin SPI MOSI GPIO pin
             * @param sclk_pin SPI clock GPIO pin
             * @param cs_pin SPI chip select GPIO pin (-1 to disable)
             * @param dc_pin Data/command GPIO pin
             * @param rst_pin Reset GPIO pin
             * @param bl_pin Backlight GPIO pin (-1 to disable)
             * @param offsetx Display X offset for GRAM alignment
             * @param offsety Display Y offset for GRAM alignment
             * @return true if initialization succeeded, false otherwise
             */
            bool beginSpi(int16_t mosi_pin, int16_t sclk_pin, int16_t cs_pin,
                          int16_t dc_pin, int16_t rst_pin, int16_t bl_pin,
                          uint16_t offsetx, uint16_t offsety);

            /**
             * Draw a single pixel (Adafruit_GFX interface)
             * Writes to frame buffer if enabled, otherwise direct to display.
             * @param x X coordinate
             * @param y Y coordinate
             * @param color RGB565 color (16-bit)
             */
            void drawPixel(int16_t x, int16_t y, uint16_t color) override;

            /**
             * Flush frame buffer to display (DisplayFlushable interface)
             * Only effective if frame buffer is enabled in Kconfig.
             */
            void flush() override;

            /**
             * Alias for flush() - maintains compatibility with display() pattern
             */
            void display();

            /**
             * Clear display to black
             */
            void clearDisplay();

            /**
             * Turn backlight on (if BL GPIO configured)
             */
            void backlightOn();

            /**
             * Turn backlight off (if BL GPIO configured)
             */
            void backlightOff();

        private:
            TFT_t *_dev;             ///< Pointer to native ST7789 driver context (opaque)
            uint16_t *_frame_buffer; ///< Optional RGB565 frame buffer
            uint32_t _buffer_size;   ///< Frame buffer size in pixels
            bool _initialized;       ///< Initialization state
            bool _use_frame_buffer;  ///< Frame buffer mode enabled
        };

    } // namespace display
} // namespace BotiEyes

#endif // CONFIG_DISPLAY_TYPE_ST7789_SPI

#endif // ESP_ST7789_H
