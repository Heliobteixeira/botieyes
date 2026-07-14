# Display Component

## Overview

Provides polymorphic display driver abstraction via **Adafruit_GFX** base class. Enables hardware-agnostic rendering in the BotiEyes application layer, with driver selection at build time via menuconfig.

**Key Design**: All display drivers inherit from `Adafruit_GFX`, allowing BotiEyes to render to any supported display without code changes.

---

## Inheritance Hierarchy

### Class Diagram

```
                       Adafruit_GFX (Abstract Base)
                              |
                              | virtual drawPixel()
                              | virtual fillScreen()
                              | (+ graphics primitives)
                              |
              +---------------+---------------+
              |                               |
     ESP_SSD1306                        ESP_ST7789
   (Monochrome OLED)                 (Color TFT LCD)
              |                               |
              |                               |
    implements drawPixel()            implements drawPixel()
    implements display()              implements display()
    implements clearDisplay()         implements clearDisplay()
              |                               |
              +-------> DisplayFlushable <----+
                           (BotiEyes)
                             |
                        virtual flush()
```

### Polymorphic Usage

**Application Layer** (main.cpp):
```cpp
// Polymorphic pointer - driver selected at build time
Adafruit_GFX* g_display = initializeDisplay();

// Hardware-agnostic rendering
g_display->drawPixel(10, 20, WHITE);
g_display->fillCircle(64, 32, 15, BLACK);

// Flush to hardware (via DisplayFlushable)
auto* flushable = dynamic_cast<BotiEyes::DisplayFlushable*>(g_display);
if (flushable) {
    flushable->flush();
}
```

**Driver Selection** (display_init.cpp):
```cpp
Adafruit_GFX* initializeDisplay() {
#if CONFIG_DISPLAY_TYPE_ST7789_SPI
    return new ESP_ST7789(240, 135);  // Color TFT
#elif CONFIG_DISPLAY_TYPE_SSD1306_SPI
    return new Ssd1306Gfx(128, 64);   // OLED SPI
#else
    return new Ssd1306Gfx(128, 64);   // OLED I2C (default)
#endif
}
```

**Key Benefit**: Switching display hardware requires **zero application code changes**, only menuconfig selection.

---

## Supported Drivers

### 1. ESP_SSD1306 (Monochrome OLED)

**Base Driver**: [nopnop2002/esp-idf-ssd1306](https://github.com/nopnop2002/esp-idf-ssd1306)  
**Protocols**: I2C, SPI  
**Resolution**: 128×64 (typical), 128×32  
**Color Depth**: 1-bit (monochrome)  
**Frame Buffer**: 1KB (128×64÷8)  

**Adafruit_GFX Methods Implemented**:
- ✅ `drawPixel(x, y, color)` - Sets pixel in 1-bit buffer
- ✅ `display()` - Flushes buffer to OLED via I2C/SPI
- ✅ `clearDisplay()` - Fills buffer with black

**Hardware Examples**:
- TTGO LoRa32 (I2C, 128×64)
- Generic SSD1306 modules (I2C or SPI)

---

### 2. ESP_ST7789 (Color TFT LCD)

**Base Driver**: [nopnop2002/esp-idf-st7789](https://github.com/nopnop2002/esp-idf-st7789)  
**Protocol**: SPI only  
**Resolution**: 240×135 (TTGO T-Display), 240×240, 320×240  
**Color Depth**: 16-bit RGB565  
**Frame Buffer**: 64KB (240×135×2 bytes, optional)  

**Adafruit_GFX Methods Implemented**:
- ✅ `drawPixel(x, y, color)` - Writes RGB565 pixel (buffer or direct)
- ✅ `display()` - Flushes frame buffer via `lcdDrawMultiPixels()`
- ✅ `clearDisplay()` - Fills screen via `lcdFillScreen()`
- ✅ `backlightOn()` / `backlightOff()` - GPIO control (ST7789-specific)

**Hardware Examples**:
- TTGO T-Display (240×135, integrated ESP32)
- Generic ST7789 240×240 modules

---

## Required Interface Contract

All display drivers **MUST** implement:

### 1. Adafruit_GFX Pure Virtual Methods

```cpp
class YourDisplayDriver : public Adafruit_GFX {
public:
    // REQUIRED: Pure virtual override
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    
    // RECOMMENDED: Performance optimization
    void fillScreen(uint16_t color) override;
};
```

### 2. BotiEyes DisplayFlushable Interface

```cpp
class YourDisplayDriver : public BotiEyes::DisplayFlushable {
public:
    // REQUIRED: Transfer buffer to hardware
    void flush() override;
};
```

### 3. Initialization Methods

```cpp
// Pattern: begin<Protocol>() returns bool for success/failure
bool beginI2C(int sda, int scl, int rst);
bool beginSpi(int mosi, int sclk, int cs, int dc, int rst);
```

### 4. Display Control Methods

```cpp
void display();        // Alias for flush() - compatibility
void clearDisplay();   // Fill with background color
```

**See**: [contracts/display-driver-interface.md](../../../specs/005-st7789-display-support/contracts/display-driver-interface.md) for complete specification.

---

## Adding a New Display Driver

### Step 1: Create Adapter Class

```cpp
// components/display/include/esp_newdriver.h
#include <Adafruit_GFX.h>
#include "BotiEyes.h"
#include "newdriver.h"  // External component

class ESP_NewDriver : public Adafruit_GFX, public BotiEyes::DisplayFlushable {
public:
    ESP_NewDriver(uint16_t w, uint16_t h);
    bool beginSpi(...);
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void flush() override;
    void display() { flush(); }
    void clearDisplay();
private:
    // Driver-specific context
};
```

### Step 2: Add Kconfig Option

```kconfig
# components/hal_board/Kconfig.projbuild
choice DISPLAY_TYPE
    config DISPLAY_TYPE_NEWDRIVER_SPI
        bool "NewDriver SPI"
endchoice
```

### Step 3: Update display_init.cpp

```cpp
#if CONFIG_DISPLAY_TYPE_NEWDRIVER_SPI
#include "esp_newdriver.h"
#endif

Adafruit_GFX* initializeDisplay() {
#if CONFIG_DISPLAY_TYPE_NEWDRIVER_SPI
    auto* display = new ESP_NewDriver(width, height);
    display->beginSpi(...);
    return display;
#endif
}
```

### Step 4: Update CMakeLists.txt

```cmake
if(EXISTS "${sdkconfig_header}")
    file(STRINGS "${sdkconfig_header}" display_type_newdriver 
         REGEX "CONFIG_DISPLAY_TYPE_NEWDRIVER_SPI=y")
    if(display_type_newdriver)
        list(APPEND DISPLAY_SRCS "src/esp_newdriver.cpp")
        list(APPEND DISPLAY_REQUIRES newdriver_component)
    endif()
endif()
```

**Result**: New driver integrated without changing BotiEyes rendering code.

---

## Status: ⚠️ Temporary Bridge Component

This component provides display initialization and adapter classes as a **temporary bridge** to external display libraries while the proper HAL layer implementation is being completed.

## Architecture Position

**Current (Temporary):**
```
Application (main/) 
    ↓ depends on
Display Component (components/display/)
    ↓ wraps
External Libraries (nopnop2002/ssd1306, nopnop2002/st7789)
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

- **src/display_init.cpp**: Main display initialization with adapter factory function
- **src/esp_ssd1306.cpp**: SSD1306 adapter implementing Adafruit_GFX interface
- **src/esp_st7789.cpp**: ST7789 adapter implementing Adafruit_GFX interface
- **include/display_init.h**: Public API for display initialization
- **include/esp_ssd1306.h**: SSD1306 adapter class declaration
- **include/esp_st7789.h**: ST7789 adapter class declaration

## Dependencies

- `botieyes`: BotiEyes library (for DisplayFlushable interface)
- `adafruit_gfx`: Adafruit GFX library (managed component, base class)
- `ssd1306`: nopnop2002 SSD1306 driver component (local path)
- `nopnop2002__st7789`: nopnop2002 ST7789 driver (managed component)

## Future Work

This component should eventually be **removed** once `components/hal_board/src/hal_display_spi.c` and `hal_display_i2c.c` are fully implemented.

See `hal_board/src/hal_display_spi.c` (currently a 42-line stub with TODOs) for the intended permanent implementation.

## References

- Feature 004: Industrial Firmware Architecture Refactoring
- Feature 005: ST7789 Display Driver Support
- Original location: These files were in `main/` directory
- Related: [main.cpp line 35](../../main/main.cpp#L35) still references this as "Legacy display init"
- Contract: [specs/005-st7789-display-support/contracts/display-driver-interface.md](../../../specs/005-st7789-display-support/contracts/display-driver-interface.md)
