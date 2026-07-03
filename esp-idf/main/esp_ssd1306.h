#ifndef ESP_SSD1306_H
#define ESP_SSD1306_H

#include <Adafruit_GFX.h>
#include "esp_err.h"
#include "esp_idf_version.h" // Required before ssd1306.h for ESP_IDF_VERSION_VAL macro
#include "ssd1306.h"

namespace BotiEyes
{
    namespace display
    {

        /**
         * Small Adafruit_GFX wrapper over the nopnop2002 SSD1306 component.
         * It keeps the BotiEyes rendering layer unchanged while delegating
         * transport and DMA handling to the component-backed SSD1306_t device.
         */
        class ESP_SSD1306 : public Adafruit_GFX
        {
        public:
            ESP_SSD1306(uint8_t w, uint8_t h);
            ~ESP_SSD1306() override;

            bool begin(uint8_t i2c_addr, int sda_pin, int scl_pin, int rst_pin);
            bool beginSpi(int mosi_pin, int sclk_pin, int cs_pin, int dc_pin, int rst_pin);

            void drawPixel(int16_t x, int16_t y, uint16_t color) override;
            void display();
            void clearDisplay();

        private:
            SSD1306_t _dev;
            uint8_t *_buffer;
            uint16_t _buffer_size;
            bool _initialized;
        };

    } // namespace display
} // namespace BotiEyes

#endif // ESP_SSD1306_H
