#ifndef DISPLAY_INIT_H
#define DISPLAY_INIT_H

#include "esp_idf_version.h" // Required before any ssd1306.h inclusion
#include <Adafruit_GFX.h>
#include "DisplayConfig.h"
#include "BotiEyes.h"

namespace BotiEyes
{
    namespace display
    {

        /**
         * Initialize OLED display based on Kconfig settings.
         * Returns an Adafruit_GFX-compatible object backed by the nopnop2002 SSD1306 component.
         */
        Adafruit_GFX *initializeDisplay();

        /**
         * Returns the flushable SSD1306 adapter backing the display object.
         */
        ::BotiEyes::DisplayFlushable *initializeDisplayFlushable();

        /**
         * Create DisplayConfig from Kconfig values.
         * Fills protocol, pins, and dimensions.
         */
        DisplayConfig createDisplayConfig();

    } // namespace display
} // namespace BotiEyes

#endif // DISPLAY_INIT_H
