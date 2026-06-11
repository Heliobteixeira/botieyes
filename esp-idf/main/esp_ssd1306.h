#ifndef ESP_SSD1306_H
#define ESP_SSD1306_H

#include <Adafruit_GFX.h>
#include "driver/i2c.h"
#include "esp_err.h"

namespace BotiEyes
{
    namespace display
    {

        /**
         * Minimal SSD1306 OLED driver for ESP-IDF
         * Extends Adafruit_GFX to work with BotiEyes library
         * Uses ESP-IDF I2C driver (legacy API for ESP-IDF 6.0 compatibility)
         */
        class ESP_SSD1306 : public Adafruit_GFX
        {
        public:
            ESP_SSD1306(uint8_t w, uint8_t h);
            ~ESP_SSD1306();

            // Initialize with ESP-IDF I2C
            bool begin(uint8_t i2c_addr, int sda_pin, int scl_pin, int rst_pin);

            // Adafruit_GFX required methods
            void drawPixel(int16_t x, int16_t y, uint16_t color) override;
            void display();
            void clearDisplay();

        private:
            i2c_port_t _i2c_port;
            uint8_t *_buffer;
            uint16_t _buffer_size;
            uint8_t _i2c_addr;
            int _rst_pin;

            void sendCommand(uint8_t cmd);
            void sendData(const uint8_t *data, size_t len);
            bool initDisplay();
        };

    } // namespace display
} // namespace BotiEyes

#endif // ESP_SSD1306_H
