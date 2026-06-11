#include "display_init.h"
#include "esp_ssd1306.h"
#include "sdkconfig.h"
#include "esp_log.h"

static const char *TAG = "display_init";

namespace BotiEyes
{
    namespace display
    {

        ESP_SSD1306 *initializeDisplay()
        {
            ESP_SSD1306 *display = nullptr;

#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
            // I2C mode
            ESP_LOGI(TAG, "Initializing SSD1306 128x64 I2C display");
            ESP_LOGI(TAG, "  SDA=%d, SCL=%d, RST=%d, Address=0x%02X",
                     CONFIG_BOTIEYES_OLED_SDA_PIN,
                     CONFIG_BOTIEYES_OLED_SCL_PIN,
                     CONFIG_BOTIEYES_OLED_RST_PIN,
                     CONFIG_BOTIEYES_OLED_I2C_ADDRESS);

            // Create display object (128x64)
            display = new ESP_SSD1306(128, 64);

            // Initialize with ESP-IDF I2C
            if (!display->begin(CONFIG_BOTIEYES_OLED_I2C_ADDRESS,
                                CONFIG_BOTIEYES_OLED_SDA_PIN,
                                CONFIG_BOTIEYES_OLED_SCL_PIN,
                                CONFIG_BOTIEYES_OLED_RST_PIN))
            {
                ESP_LOGE(TAG, "SSD1306 I2C initialization failed");
                delete display;
                return nullptr;
            }

#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_SPI)
            ESP_LOGE(TAG, "SPI mode not yet implemented");
            return nullptr;
#else
#error "OLED protocol not configured. Run idf.py menuconfig."
#endif

            ESP_LOGI(TAG, "Display initialized successfully");

            // Clear display
            display->clearDisplay();
            display->display();

            return display;
        }

        DisplayConfig createDisplayConfig()
        {
            DisplayConfig config = {};

            config.type = DISPLAY_SSD1306_128x64;
            config.width = 128;
            config.height = 64;

#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
            config.protocol = PROTOCOL_I2C;
            config.i2cAddress = CONFIG_BOTIEYES_OLED_I2C_ADDRESS;
            config.cs_pin = -1;
            config.dc_pin = -1;
            config.reset_pin = CONFIG_BOTIEYES_OLED_RST_PIN;

#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_SPI)
            config.protocol = PROTOCOL_SPI_HW;
            config.i2cAddress = 0;
            config.cs_pin = CONFIG_BOTIEYES_OLED_CS_PIN;
            config.dc_pin = CONFIG_BOTIEYES_OLED_DC_PIN;
            config.reset_pin = CONFIG_BOTIEYES_OLED_RST_PIN;
#endif

            return config;
        }

    } // namespace display
} // namespace BotiEyes
