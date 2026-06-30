# Display Component

## Status: ⚠️ Temporary Bridge Component

This component provides display initialization and SSD1306 adapter classes as a **temporary bridge** to the external [nopnop2002/ssd1306](https://github.com/nopnop2002/esp-idf-ssd1306) library.

## Purpose

Provides working display functionality while the proper HAL layer implementation is being completed.

## Architecture Position

**Current (Temporary):**
```
Application (main/) 
    ↓ depends on
Display Component (components/display/)
    ↓ wraps
External SSD1306 Library (/Users/helioteixeira/dev/esp-idf-ssd1306)
```

**Target (Future):**
```
Application (main/)
    ↓ depends on
HAL Board (components/hal_board/)
    ↓ contains
HAL Display SPI/I2C (hal_display_spi.c, hal_display_i2c.c)
    ↓ uses
Driver Layer (ESP-IDF SPI/I2C drivers)
```

## Files

- **src/display_init.cpp**: Main display initialization with Ssd1306Gfx wrapper class
- **src/esp_ssd1306.cpp**: ESP_SSD1306 adapter implementing Adafruit_GFX interface
- **include/display_init.h**: Public API for display initialization
- **include/esp_ssd1306.h**: SSD1306 adapter class declaration

## Dependencies

- `botieyes`: BotiEyes library (for DisplayFlushable interface)
- `adafruit_gfx`: Adafruit GFX library (managed component)
- `ssd1306`: External nopnop2002 SSD1306 driver component

## Future Work

This component should eventually be **removed** once `components/hal_board/src/hal_display_spi.c` and `hal_display_i2c.c` are fully implemented.

See `hal_board/src/hal_display_spi.c` (currently a 42-line stub with TODOs) for the intended permanent implementation.

## References

- Feature 004: Industrial Firmware Architecture Refactoring
- Original location: These files were in `main/` directory
- Related: [main.cpp line 35](../../main/main.cpp#L35) still references this as "Legacy display init"
