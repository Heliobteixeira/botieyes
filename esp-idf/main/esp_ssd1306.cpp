#include "esp_ssd1306.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "driver/gpio.h"
#include <cstring>

static const char *TAG = "esp_ssd1306";

// SSD1306 commands
#define SSD1306_SETCONTRAST 0x81
#define SSD1306_DISPLAYALLON_RESUME 0xA4
#define SSD1306_DISPLAYALLON 0xA5
#define SSD1306_NORMALDISPLAY 0xA6
#define SSD1306_INVERTDISPLAY 0xA7
#define SSD1306_DISPLAYOFF 0xAE
#define SSD1306_DISPLAYON 0xAF
#define SSD1306_SETDISPLAYOFFSET 0xD3
#define SSD1306_SETCOMPINS 0xDA
#define SSD1306_SETVCOMDETECT 0xDB
#define SSD1306_SETDISPLAYCLOCKDIV 0xD5
#define SSD1306_SETPRECHARGE 0xD9
#define SSD1306_SETMULTIPLEX 0xA8
#define SSD1306_SETLOWCOLUMN 0x00
#define SSD1306_SETHIGHCOLUMN 0x10
#define SSD1306_SETSTARTLINE 0x40
#define SSD1306_MEMORYMODE 0x20
#define SSD1306_COLUMNADDR 0x21
#define SSD1306_PAGEADDR 0x22
#define SSD1306_COMSCANINC 0xC0
#define SSD1306_COMSCANDEC 0xC8
#define SSD1306_SEGREMAP 0xA0
#define SSD1306_CHARGEPUMP 0x8D

namespace BotiEyes
{
    namespace display
    {

        ESP_SSD1306::ESP_SSD1306(uint8_t w, uint8_t h)
            : Adafruit_GFX(w, h), _i2c_port(I2C_NUM_0), _buffer(nullptr), _i2c_addr(0), _rst_pin(-1)
        {
            // Allocate framebuffer: 1 bit per pixel, row-major
            _buffer_size = ((w * h) + 7) / 8;
            _buffer = new uint8_t[_buffer_size];
            clearDisplay();
        }

        ESP_SSD1306::~ESP_SSD1306()
        {
            i2c_driver_delete(_i2c_port);
            delete[] _buffer;
        }

        bool ESP_SSD1306::begin(uint8_t i2c_addr, int sda_pin, int scl_pin, int rst_pin)
        {
            _i2c_addr = i2c_addr;
            _rst_pin = rst_pin;

            // Hardware reset if pin provided
            if (_rst_pin >= 0)
            {
                gpio_config_t io_conf = {};
                io_conf.intr_type = GPIO_INTR_DISABLE;
                io_conf.mode = GPIO_MODE_OUTPUT;
                io_conf.pin_bit_mask = (1ULL << _rst_pin);
                io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
                io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
                gpio_config(&io_conf);

                gpio_set_level((gpio_num_t)_rst_pin, 1);
                vTaskDelay(pdMS_TO_TICKS(1));
                gpio_set_level((gpio_num_t)_rst_pin, 0);
                vTaskDelay(pdMS_TO_TICKS(10));
                gpio_set_level((gpio_num_t)_rst_pin, 1);
                vTaskDelay(pdMS_TO_TICKS(10));
            }

            // Initialize I2C driver
            i2c_config_t conf = {};
            conf.mode = I2C_MODE_MASTER;
            conf.sda_io_num = (gpio_num_t)sda_pin;
            conf.scl_io_num = (gpio_num_t)scl_pin;
            conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
            conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
            conf.master.clk_speed = 400000; // 400 kHz
            conf.clk_flags = 0;

            esp_err_t err = i2c_param_config(_i2c_port, &conf);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "I2C param config failed: %s", esp_err_to_name(err));
                return false;
            }

            err = i2c_driver_install(_i2c_port, I2C_MODE_MASTER, 0, 0, 0);
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
                return false;
            }

            return initDisplay();
        }

        void ESP_SSD1306::sendCommand(uint8_t cmd)
        {
            i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
            i2c_master_start(i2c_cmd);
            i2c_master_write_byte(i2c_cmd, (_i2c_addr << 1) | I2C_MASTER_WRITE, true);
            i2c_master_write_byte(i2c_cmd, 0x00, true); // Command byte
            i2c_master_write_byte(i2c_cmd, cmd, true);
            i2c_master_stop(i2c_cmd);
            i2c_master_cmd_begin(_i2c_port, i2c_cmd, pdMS_TO_TICKS(100));
            i2c_cmd_link_delete(i2c_cmd);
        }

        void ESP_SSD1306::sendData(const uint8_t *data, size_t len)
        {
            // Send data in chunks
            const size_t chunk_size = 16;

            for (size_t i = 0; i < len; i += chunk_size)
            {
                size_t this_chunk = (i + chunk_size > len) ? (len - i) : chunk_size;

                i2c_cmd_handle_t i2c_cmd = i2c_cmd_link_create();
                i2c_master_start(i2c_cmd);
                i2c_master_write_byte(i2c_cmd, (_i2c_addr << 1) | I2C_MASTER_WRITE, true);
                i2c_master_write_byte(i2c_cmd, 0x40, true); // Data byte
                i2c_master_write(i2c_cmd, data + i, this_chunk, true);
                i2c_master_stop(i2c_cmd);
                i2c_master_cmd_begin(_i2c_port, i2c_cmd, pdMS_TO_TICKS(100));
                i2c_cmd_link_delete(i2c_cmd);
            }
        }

        bool ESP_SSD1306::initDisplay()
        {
            // Standard SSD1306 initialization sequence for 128x64
            sendCommand(SSD1306_DISPLAYOFF);
            sendCommand(SSD1306_SETDISPLAYCLOCKDIV);
            sendCommand(0x80); // Suggested ratio
            sendCommand(SSD1306_SETMULTIPLEX);
            sendCommand(HEIGHT - 1);
            sendCommand(SSD1306_SETDISPLAYOFFSET);
            sendCommand(0x00);
            sendCommand(SSD1306_SETSTARTLINE | 0x00);
            sendCommand(SSD1306_CHARGEPUMP);
            sendCommand(0x14); // Enable charge pump
            sendCommand(SSD1306_MEMORYMODE);
            sendCommand(0x00); // Horizontal addressing mode
            sendCommand(SSD1306_SEGREMAP | 0x01);
            sendCommand(SSD1306_COMSCANDEC);
            sendCommand(SSD1306_SETCOMPINS);
            sendCommand(0x12); // Alternative COM pin config
            sendCommand(SSD1306_SETCONTRAST);
            sendCommand(0xCF);
            sendCommand(SSD1306_SETPRECHARGE);
            sendCommand(0xF1);
            sendCommand(SSD1306_SETVCOMDETECT);
            sendCommand(0x40);
            sendCommand(SSD1306_DISPLAYALLON_RESUME);
            sendCommand(SSD1306_NORMALDISPLAY);
            sendCommand(SSD1306_DISPLAYON);

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
            // Set column address range
            sendCommand(SSD1306_COLUMNADDR);
            sendCommand(0);         // Start column
            sendCommand(WIDTH - 1); // End column

            // Set page address range
            sendCommand(SSD1306_PAGEADDR);
            sendCommand(0);                // Start page
            sendCommand((HEIGHT / 8) - 1); // End page

            // Send framebuffer
            sendData(_buffer, _buffer_size);
        }

        void ESP_SSD1306::clearDisplay()
        {
            memset(_buffer, 0, _buffer_size);
        }

    } // namespace display
} // namespace BotiEyes
