#ifndef DISPLAY_INIT_H
#define DISPLAY_INIT_H

#include <Adafruit_GFX.h>
#include "DisplayConfig.h"

namespace BotiEyes
{
    namespace display
    {

        // Forward declaration of ESP-IDF SSD1306 adapter
        class ESP_SSD1306;

        /**
         * Initialize OLED display based on Kconfig settings
         * Returns initialized display object or nullptr on failure
         */
        ESP_SSD1306 *initializeDisplay();

        /**
         * Create DisplayConfig from Kconfig values
         * Fills protocol, pins, and dimensions
         */
        DisplayConfig createDisplayConfig();

    } // namespace display
} // namespace BotiEyes

#endif // DISPLAY_INIT_H
