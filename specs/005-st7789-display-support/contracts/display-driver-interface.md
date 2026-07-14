# Display Driver Interface Contract

**Feature**: 005-st7789-display-support | **Version**: 1.0 | **Date**: 2026-07-06

## Overview

This document defines the interface contract for display drivers in the botieyes firmware. All display driver implementations (SSD1306, ST7789, future drivers) MUST implement this interface to ensure compatibility with the BotiEyes rendering engine.

## Interface Definition

### Base Class: Adafruit_GFX

All display drivers extend `Adafruit_GFX`, which provides:

**Graphics Primitives** (inherited, implemented by base class):
- `drawLine(x0, y0, x1, y1, color)`
- `drawRect(x, y, w, h, color)`
- `fillRect(x, y, w, h, color)`
- `drawCircle(x, y, r, color)`
- `fillCircle(x, y, r, color)`
- `drawTriangle(x0, y0, x1, y1, x2, y2, color)`
- `fillTriangle(x0, y0, x1, y1, x2, y2, color)`
- `drawChar(x, y, c, color, bg, size)`
- `print(string)`, `println(string)`

**Required Overrides** (driver-specific):
- `drawPixel(x, y, color)` - MUST be implemented
- `fillScreen(color)` - SHOULD be overridden for performance (optional)

### Extended Interface: DisplayFlushable

BotiEyes-specific interface for display buffer management:

```cpp
namespace BotiEyes {
    class DisplayFlushable {
    public:
        virtual void flush() = 0;  // Transfer buffer to hardware
        virtual ~DisplayFlushable() = default;
    };
}
```

**Requirement**: Display adapters MUST implement `DisplayFlushable::flush()` or provide equivalent `display()` method.

---

## Contract Specification

### 1. Initialization Contract

**Method Signature**:
```cpp
// SSD1306
bool begin(uint8_t i2c_addr, int sda_pin, int scl_pin, int rst_pin);
bool beginSpi(int mosi_pin, int sclk_pin, int cs_pin, int dc_pin, int rst_pin);

// ST7789
bool beginSpi(int mosi_pin, int sclk_pin, int cs_pin, int dc_pin, int rst_pin, int bl_pin);
```

**Preconditions**:
- GPIO pins must be valid for target ESP32 variant (0-48, excluding input-only pins)
- Pins must not conflict (same GPIO used for multiple functions)
- Bus (I2C/SPI) must not already be initialized by another driver

**Postconditions**:
- **On success**: Display hardware initialized, ready for drawing operations, returns `true` or `ESP_OK`
- **On failure**: Error logged, display not usable, returns `false` or `ESP_ERR_*`

**Behavior**:
1. Configure GPIO pins (reset, data/command, backlight)
2. Initialize communication bus (I2C or SPI)
3. Send display-specific initialization sequence
4. Allocate frame buffer if configured (optional, may fail gracefully)
5. Return success/failure status

**Example**:
```cpp
ESP_ST7789* display = new ESP_ST7789(240, 135);  // TTGO T-Display dimensions
if (!display->beginSpi(GPIO_MOSI, GPIO_SCLK, GPIO_CS, GPIO_DC, GPIO_RST, GPIO_BL)) {
    ESP_LOGE(TAG, "Display initialization failed");
    return ESP_FAIL;
}
```

---

### 2. Pixel Drawing Contract

**Method Signature**:
```cpp
void drawPixel(int16_t x, int16_t y, uint16_t color);
```

**Preconditions**:
- Display must be initialized (`begin*()` called successfully)
- Coordinates may be outside bounds (implementation must handle clipping)

**Postconditions**:
- **Within bounds**: Pixel at (x, y) updated to specified color
- **Out of bounds**: No-op or clipped (no error, undefined behavior allowed)

**Behavior**:
- **Frame buffer mode**: Write color to `_buffer[y * width + x]` (deferred hardware update)
- **Direct mode**: Immediate SPI/I2C transfer to display hardware
- Color format: RGB565 (16-bit) for ST7789, non-zero=on for SSD1306

**Performance Requirement**:
- MUST be efficient (called thousands of times per frame)
- Frame buffer mode strongly preferred for animated graphics

**Example**:
```cpp
display->drawPixel(120, 120, WHITE);  // Center pixel
display->drawPixel(-1, 0, RED);       // Out of bounds - no-op
```

---

### 3. Buffer Flush Contract

**Method Signature**:
```cpp
void flush();        // DisplayFlushable interface
// OR
void display();      // Adafruit_SSD1306 convention
```

**Preconditions**:
- Display must be initialized
- May be called multiple times without intervening draws (idempotent)

**Postconditions**:
- Frame buffer contents transferred to physical display hardware
- Display shows updated image
- Buffer remains allocated (not cleared)

**Behavior**:
- **Frame buffer mode**: Bulk SPI/I2C transfer of entire buffer to display
- **Direct mode**: No-op (pixels already on screen)

**Performance Requirement**:
- **Frame buffer transfer**: <30ms for 240x135 RGB565 at 40 MHz SPI
- **Direct mode**: Negligible overhead (<1ms)

**Example**:
```cpp
// Draw multiple pixels
for (int i = 0; i < 100; i++) {
    display->drawPixel(i, i, WHITE);
}
// Transfer all pixels to screen
display->flush();  // or display->display()
```

---

### 4. Screen Clear Contract

**Method Signature**:
```cpp
void clearDisplay();      // Adafruit_SSD1306 convention
// OR  
void fillScreen(color);   // Adafruit_GFX base class
```

**Preconditions**:
- Display must be initialized

**Postconditions**:
- All pixels set to background color (BLACK for clear, specified color for fillScreen)
- Changes visible after flush() in frame buffer mode

**Behavior**:
- **Frame buffer mode**: Memset buffer to color value
- **Direct mode**: Send fill command to display hardware

**Performance Requirement**:
- **Frame buffer**: O(n) where n = width * height (fast memset)
- **Direct mode**: Single SPI command sequence (<10ms)

**Example**:
```cpp
display->clearDisplay();       // Clear to black
display->fillScreen(BLACK);    // Equivalent
display->flush();              // Make visible (frame buffer mode)
```

---

### 5. Brightness Control Contract (Optional)

**Method Signature**:
```cpp
void setBrightness(uint8_t level);  // 0-255 or driver-specific range
void backlightOn();
void backlightOff();
```

**Preconditions**:
- Display must be initialized
- Brightness control supported by hardware (backlight GPIO configured)

**Postconditions**:
- Display brightness adjusted
- On unsupported hardware: No-op or error logged

**Behavior**:
- **PWM backlight**: Adjust duty cycle (0-100%)
- **GPIO backlight**: On/off only (binary control)
- **I2C contrast**: Send contrast command to display controller

**Example**:
```cpp
display->setBrightness(128);   // 50% brightness
display->backlightOff();        // Power saving mode
```

---

## Color Format Contract

### RGB565 Encoding

**Standard Colors** (platform-independent):
```cpp
#define BLACK   0x0000  // All pixels off
#define WHITE   0xFFFF  // All pixels on
#define RED     0xF800  // rgb565(255, 0, 0)
#define GREEN   0x07E0  // rgb565(0, 255, 0)
#define BLUE    0x001F  // rgb565(0, 0, 255)
```

**Conversion Macro**:
```cpp
#define rgb565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
```

### Monochrome Adaptation (SSD1306)

**Rule**: Non-zero color values → pixel ON, zero → pixel OFF

**Example**:
```cpp
drawPixel(x, y, WHITE);   // Pixel on (0xFFFF != 0)
drawPixel(x, y, RED);     // Pixel on (0xF800 != 0)
drawPixel(x, y, BLACK);   // Pixel off (0x0000 == 0)
```

---

## Error Handling Contract

### Initialization Errors

**MUST return error code or false on**:
- GPIO pin conflicts (same pin for multiple functions)
- Bus initialization failure (spi_bus_initialize, i2c_param_config)
- Display not responding (timeout on initialization commands)

**MAY log warning and continue on**:
- Frame buffer allocation failure (fall back to direct rendering)
- Backlight GPIO not available (display works without backlight)

**Example Error Codes** (ESP-IDF style):
```cpp
ESP_OK            // Success
ESP_ERR_NO_MEM    // Frame buffer allocation failed
ESP_ERR_INVALID_ARG  // Invalid GPIO pin number
ESP_FAIL          // Generic initialization failure
```

### Runtime Errors

**MUST handle gracefully**:
- Out-of-bounds pixel coordinates (clip or ignore)
- Multiple flush() calls (idempotent)
- Operations on uninitialized display (undefined, may crash - caller error)

**Example**:
```cpp
void ESP_ST7789::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Bounds check (required for frame buffer mode)
    if (x < 0 || x >= _width || y < 0 || y >= _height) {
        return;  // Silently clip
    }
    // ... draw logic
}
```

---

## Thread Safety Contract

### Current Requirement (Single-Threaded)

**Assumption**: Display accessed from single task (main rendering task)

**Not Required**:
- Mutex protection
- Reentrant operations
- Concurrent drawing

### Future Extension (Multi-Threaded)

**IF multiple tasks draw concurrently, driver MUST**:
- Protect SPI/I2C transactions with mutex
- Protect frame buffer with semaphore
- Document thread-safety guarantees

---

## Performance Guarantees

### Initialization Time

**MUST initialize within**:
- **SSD1306 I2C**: <500ms (100 kHz I2C, 128x64 display)
- **SSD1306 SPI**: <200ms (10 MHz SPI, 128x64 display)
- **ST7789 SPI**: <500ms (40 MHz SPI, 240x135 display, includes delays)

### Frame Rate (240x135 ST7789 - TTGO T-Display)

**MUST achieve minimum 10 FPS**:
- **40 MHz SPI + frame buffer**: ~13ms transfer + drawing time = <30ms/frame ✓ (33 FPS)
- **20 MHz SPI + frame buffer**: ~26ms transfer + drawing time = <50ms/frame ✓ (20 FPS)
- **Direct rendering**: Depends on pixel count, <100ms for typical eye animation

### Memory Overhead

**MUST document memory usage**:
- **Frame buffer**: width * height * bytes_per_pixel
- **Driver overhead**: <5 KB (structs, buffers, handles)
- **SPI queue**: ~1 KB (transaction buffers)

**Example** (ST7789 240x135 RGB565 - TTGO T-Display):
- Frame buffer: 240 * 135 * 2 = 64,800 bytes (~65 KB)
- Driver overhead: ~2 KB
- Total: ~67 KB (12.5% of ESP32 SRAM)

---

## Compatibility Matrix

### Required Support

| Display Type | Interface | Resolution | Color | Status |
|-------------|-----------|-----------|-------|--------|
| SSD1306     | I2C       | 128x64    | Mono  | ✅ Supported |
| SSD1306     | SPI       | 128x64    | Mono  | ✅ Supported |
| ST7789      | SPI       | 240x135 (default) | RGB565| ⚠️ This Feature |
| ST7789      | SPI       | 240x240, others   | RGB565| ⚠️ Configurable |

### Future Compatibility

**New drivers MUST**:
- Extend Adafruit_GFX
- Implement drawPixel()
- Implement flush() or display()
- Document color format
- Provide initialization method

**Recommended** (for consistency):
- Follow naming conventions (ESP_<DriverName>)
- Support frame buffer via Kconfig
- Return ESP-IDF error codes
- Use ESP-IDF SPI/I2C drivers

---

## Example Implementation Template

```cpp
// esp_st7789.h
#ifndef ESP_ST7789_H
#define ESP_ST7789_H

#include <Adafruit_GFX.h>
#include "st7789.h"

namespace BotiEyes {
    namespace display {
        
        class ESP_ST7789 : public Adafruit_GFX {
        public:
            // Constructor
            ESP_ST7789(uint16_t w, uint16_t h);
            ~ESP_ST7789() override;
            
            // Initialization (REQUIRED)
            bool beginSpi(int mosi, int sclk, int cs, int dc, int rst, int bl);
            
            // Core drawing (REQUIRED)
            void drawPixel(int16_t x, int16_t y, uint16_t color) override;
            
            // Buffer management (REQUIRED for frame buffer mode)
            void display();  // or flush()
            
            // Utility methods (OPTIONAL)
            void clearDisplay();
            void backlightOn();
            void backlightOff();
            
        private:
            TFT_t _dev;                // Native driver context
            uint16_t *_buffer;         // Frame buffer (optional)
            bool _initialized;
        };
        
    } // namespace display
} // namespace BotiEyes

#endif
```

```cpp
// esp_st7789.cpp (excerpt)
bool ESP_ST7789::beginSpi(int mosi, int sclk, int cs, int dc, int rst, int bl) {
    // 1. Initialize SPI bus
    spi_master_init(&_dev, mosi, sclk, cs, dc, rst, bl);
    
    // 2. Initialize ST7789 controller
    lcdInit(&_dev, _width, _height, 0, 0);
    
    // 3. Allocate frame buffer (optional)
    if (CONFIG_FRAME_BUFFER) {
        _buffer = (uint16_t*)malloc(_width * _height * sizeof(uint16_t));
        if (!_buffer) {
            ESP_LOGW(TAG, "Frame buffer allocation failed, using direct rendering");
        }
    }
    
    _initialized = true;
    return true;
}

void ESP_ST7789::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Bounds check
    if (x < 0 || x >= _width || y < 0 || y >= _height) return;
    
    // Frame buffer or direct rendering
    if (_buffer) {
        _buffer[y * _width + x] = color;
    } else {
        lcdDrawPixel(&_dev, x, y, color);
    }
}

void ESP_ST7789::display() {
    if (_buffer) {
        lcdDrawMultiPixels(&_dev, 0, 0, _width * _height, _buffer);
    }
    // Direct mode: no-op (already on screen)
}
```

---

## Validation Checklist

**Before submitting a new display driver, verify**:

- [ ] Extends `Adafruit_GFX` base class
- [ ] Implements `drawPixel(x, y, color)` override
- [ ] Provides initialization method (`begin*()`)
- [ ] Implements buffer flush (`flush()` or `display()`)
- [ ] Handles out-of-bounds coordinates gracefully
- [ ] Returns error codes from initialization
- [ ] Documents frame buffer memory requirements
- [ ] Supports Kconfig for GPIO pin configuration
- [ ] Logs errors with appropriate ESP_LOG* macros
- [ ] Tested on target hardware (initialization, drawing, flush)
- [ ] Achieves required frame rate (≥10 FPS for animation)
- [ ] Integrated with `display_init.cpp` factory pattern
- [ ] Updated `DisplayType` enum in Kconfig
- [ ] Added to `hal_board.c` initialization switch statement

---

## Adding New Display Drivers

**Target Audience**: Firmware developers integrating new display hardware into BotiEyes.

**Goal**: Add support for a new display type without modifying the BotiEyes rendering engine.

### Prerequisites

Before starting, ensure you have:
- ✅ Display hardware datasheet (pinout, initialization sequence, color format)
- ✅ Working low-level driver component (nopnop2002-style or vendor SDK)
- ✅ ESP-IDF development environment (v5.0+)
- ✅ Test hardware (display module + ESP32 development board)

---

### Step 1: Create Display Adapter Class

**Location**: `esp-idf/components/display/include/esp_<driver>.h`

**Template**:
```cpp
#ifndef ESP_<DRIVER>_H
#define ESP_<DRIVER>_H

#include <Adafruit_GFX.h>
#include "BotiEyes.h"
#include "<driver>.h"  // External component header

namespace BotiEyes {
    namespace display {
        
        /**
         * Adafruit_GFX adapter for <Driver> display driver.
         * Wraps the <vendor>/<driver> component to provide hardware abstraction.
         */
        class ESP_<Driver> : public Adafruit_GFX, public ::BotiEyes::DisplayFlushable {
        public:
            // Constructor: width and height in pixels
            ESP_<Driver>(uint16_t w, uint16_t h);
            ~ESP_<Driver>() override;
            
            // Initialization: return true on success, false on failure
            bool beginSpi(/* GPIO pins, config params */);
            // OR
            bool beginI2C(/* GPIO pins, I2C address */);
            
            // REQUIRED: Adafruit_GFX pure virtual override
            void drawPixel(int16_t x, int16_t y, uint16_t color) override;
            
            // REQUIRED: DisplayFlushable interface
            void flush() override;
            
            // OPTIONAL: Convenience method (alias for flush)
            void display() { flush(); }
            
            // OPTIONAL: Clear screen
            void clearDisplay();
            
        private:
            <DriverContext_t> _dev;    // Native driver context
            uint16_t *_frame_buffer;   // Optional frame buffer
            bool _initialized;
            bool _use_frame_buffer;
        };
        
    } // namespace display
} // namespace BotiEyes

#endif
```

**Implementation** (`src/esp_<driver>.cpp`):
```cpp
#include "esp_<driver>.h"
#include "esp_log.h"

static const char *TAG = "ESP_<Driver>";

namespace BotiEyes {
    namespace display {
        
        ESP_<Driver>::ESP_<Driver>(uint16_t w, uint16_t h)
            : Adafruit_GFX(w, h), _frame_buffer(nullptr), _initialized(false) {
            memset(&_dev, 0, sizeof(_dev));
            
            // Allocate frame buffer if configured via Kconfig
            #ifdef CONFIG_<DRIVER>_FRAME_BUFFER
            _buffer_size = w * h * sizeof(uint16_t);  // RGB565
            _frame_buffer = (uint16_t*)malloc(_buffer_size);
            if (!_frame_buffer) {
                ESP_LOGW(TAG, "Frame buffer allocation failed, using direct rendering");
                _use_frame_buffer = false;
            } else {
                _use_frame_buffer = true;
            }
            #endif
        }
        
        ESP_<Driver>::~ESP_<Driver>() {
            if (_frame_buffer) {
                free(_frame_buffer);
            }
        }
        
        bool ESP_<Driver>::beginSpi(/* params */) {
            // 1. Initialize SPI bus via external driver component
            spi_master_init(&_dev, mosi, sclk, cs, dc, rst, bl);
            
            // 2. Initialize display controller
            <driver>_init(&_dev, _width, _height, offsetx, offsety);
            
            // 3. Clear display to known state
            clearDisplay();
            flush();
            
            _initialized = true;
            ESP_LOGI(TAG, "Display initialized: %dx%d", _width, _height);
            return true;
        }
        
        void ESP_<Driver>::drawPixel(int16_t x, int16_t y, uint16_t color) {
            // Bounds check
            if (x < 0 || x >= _width || y < 0 || y >= _height) {
                return;  // Silently clip
            }
            
            // Frame buffer or direct rendering
            if (_use_frame_buffer && _frame_buffer) {
                _frame_buffer[y * _width + x] = color;
            } else {
                <driver>_draw_pixel(&_dev, x, y, color);
            }
        }
        
        void ESP_<Driver>::flush() {
            if (!_initialized) return;
            
            if (_use_frame_buffer && _frame_buffer) {
                // Bulk transfer frame buffer to display
                <driver>_draw_pixels(&_dev, 0, 0, _width * _height, _frame_buffer);
            }
            // Direct mode: no-op (pixels already on screen)
        }
        
        void ESP_<Driver>::clearDisplay() {
            if (_use_frame_buffer && _frame_buffer) {
                memset(_frame_buffer, 0, _width * _height * sizeof(uint16_t));
            } else {
                <driver>_fill_screen(&_dev, 0x0000);  // Black
            }
        }
        
    } // namespace display
} // namespace BotiEyes
```

---

### Step 2: Add Kconfig Configuration

**Location**: `esp-idf/components/hal_board/Kconfig.projbuild`

**Add to DISPLAY_TYPE choice**:
```kconfig
choice DISPLAY_TYPE
    prompt "Display controller type"
    default DISPLAY_TYPE_SSD1306_I2C
    
    config DISPLAY_TYPE_<DRIVER>_SPI
        bool "<Driver> SPI"
        help
            Use <Driver> color TFT display via SPI.
            Example hardware: <Board name>
            
    # ... existing options
endchoice
```

**Create driver-specific config** (`components/<driver>_config/Kconfig.projbuild`):
```kconfig
menu "<Driver> Display Configuration"
    depends on DISPLAY_TYPE_<DRIVER>_SPI
    
    comment "GPIO Pin Configuration (<Board> defaults)"
    
    config <DRIVER>_MOSI_GPIO
        int "MOSI GPIO pin"
        default 19
        range -1 48
        help
            SPI MOSI pin for <Driver> display.
    
    config <DRIVER>_SCLK_GPIO
        int "SCLK GPIO pin"
        default 18
        # ... similar for all GPIO pins
    
    config <DRIVER>_WIDTH
        int "Display width (pixels)"
        default 240
        
    config <DRIVER>_HEIGHT
        int "Display height (pixels)"
        default 135
        
    config <DRIVER>_FRAME_BUFFER
        bool "Enable frame buffer"
        default n
        help
            Allocate full frame buffer in RAM for faster rendering.
            Memory usage: width * height * 2 bytes (RGB565)
            
endmenu
```

---

### Step 3: Add Component Dependency

**Method A: Managed Component** (recommended for public components)

**Location**: `esp-idf/main/idf_component.yml`

```yaml
dependencies:
  <vendor>/<driver>:
    git: https://github.com/<vendor>/esp-idf-<driver>.git
    path: components/<driver>
```

**Method B: Local Component**

Place component in `esp-idf/components/<driver>/` or reference external path in CMakeLists.txt.

---

### Step 4: Update display_init.cpp

**Location**: `esp-idf/components/display/src/display_init.cpp`

**Add conditional include**:
```cpp
#if defined(CONFIG_DISPLAY_TYPE_<DRIVER>_SPI)
#include "esp_<driver>.h"
#endif
```

**Add to initializeDisplay()**:
```cpp
Adafruit_GFX* initializeDisplay() {
    if (g_display != nullptr) {
        return g_display;
    }
    
    ESP_LOGI(TAG, "Initializing display based on Kconfig selection");
    
#if defined(CONFIG_DISPLAY_TYPE_<DRIVER>_SPI)
    // <Driver> Display
    ESP_LOGI(TAG, "Display type: <Driver> SPI");
    auto *display = new ESP_<Driver>(CONFIG_<DRIVER>_WIDTH, CONFIG_<DRIVER>_HEIGHT);
    
    if (!display->beginSpi(
            CONFIG_<DRIVER>_MOSI_GPIO,
            CONFIG_<DRIVER>_SCLK_GPIO,
            CONFIG_<DRIVER>_CS_GPIO,
            CONFIG_<DRIVER>_DC_GPIO,
            CONFIG_<DRIVER>_RST_GPIO,
            CONFIG_<DRIVER>_BL_GPIO))
    {
        ESP_LOGE(TAG, "<Driver> SPI initialization failed");
        delete display;
        return nullptr;
    }
    
    g_display = display;
    g_flushable = display;
    display->clearDisplay();
    display->flush();
    
#elif ...
```

**Add compile-time assertion** (after includes):
```cpp
#if defined(CONFIG_DISPLAY_TYPE_<DRIVER>_SPI)
// Verify ESP_<Driver> implements required interfaces
static_assert(std::is_base_of<Adafruit_GFX, BotiEyes::display::ESP_<Driver>>::value,
              "ESP_<Driver> must inherit from Adafruit_GFX");
static_assert(std::is_base_of<::BotiEyes::DisplayFlushable, BotiEyes::display::ESP_<Driver>>::value,
              "ESP_<Driver> must inherit from DisplayFlushable");
#endif
```

---

### Step 5: Update display/CMakeLists.txt

**Location**: `esp-idf/components/display/CMakeLists.txt`

**Add conditional compilation**:
```cmake
# Conditionally add <Driver> driver when selected in menuconfig
idf_build_get_property(sdkconfig_header SDKCONFIG_HEADER)
if(EXISTS "${sdkconfig_header}")
    file(STRINGS "${sdkconfig_header}" display_type_<driver>
         REGEX "CONFIG_DISPLAY_TYPE_<DRIVER>_SPI=y")
    if(display_type_<driver>)
        list(APPEND DISPLAY_SRCS "src/esp_<driver>.cpp")
        list(APPEND DISPLAY_REQUIRES <vendor>__<driver>)  # Managed component name
    endif()
endif()
```

---

### Step 6: Build and Test

**Build**:
```bash
cd esp-idf
source ~/.espressif/tools/activate_idf_v6.0.1.sh
idf.py menuconfig
# Navigate to "HAL Board Configuration" → "Display controller type"
# Select "<Driver> SPI"
# Configure GPIO pins in "<Driver> Display Configuration"
idf.py build
```

**Flash**:
```bash
idf.py flash monitor
```

**Verify**:
1. ✅ Build completes without errors
2. ✅ Initialization log shows correct display type
3. ✅ Display backlight turns on
4. ✅ BotiEyes eyes appear on screen
5. ✅ Eye animations render smoothly (≥10 FPS)
6. ✅ No visual artifacts or flickering

---

### Step 7: Backward Compatibility Verification

**Critical**: Ensure existing display configurations still build.

**Test SSD1306 I2C** (default):
```bash
idf.py menuconfig
# Select "Display controller type" → "SSD1306 I2C"
idf.py build
# Verify: No <driver> files compiled, no <driver> component linked
```

**Test SSD1306 SPI**:
```bash
idf.py menuconfig
# Select "Display controller type" → "SSD1306 SPI"
idf.py build
# Verify: Only SSD1306 compiled
```

**Expected Result**: All three configurations (SSD1306 I2C, SSD1306 SPI, <Driver> SPI) build successfully without code changes in BotiEyes.

---

### Troubleshooting

**Build errors**:
- ❌ "undefined reference to `<driver>_init`": Component not linked (check CMakeLists.txt)
- ❌ "CONFIG_<DRIVER>_WIDTH undeclared": Kconfig not loaded (check Kconfig.projbuild location)
- ❌ Static assertion failed: Class doesn't inherit from required base (check class declaration)

**Runtime errors**:
- ❌ Display blank: Check GPIO pin assignments, power supply, initialization sequence
- ❌ Initialization failed: Check SPI/I2C bus conflicts, GPIO conflicts, hardware connections
- ❌ Artifacts/corruption: Check SPI clock speed (try lowering to 20 MHz), frame buffer alignment

**Performance issues**:
- ❌ <10 FPS: Enable frame buffer mode, increase SPI clock speed
- ❌ High memory usage: Disable frame buffer, use direct rendering

---

### Best Practices

1. ✅ **Test on actual hardware** before submitting (emulator insufficient for SPI/I2C validation)
2. ✅ **Document validated GPIO pins** in Kconfig help text (reference board pinout)
3. ✅ **Provide sensible defaults** (tested configuration that works out-of-box)
4. ✅ **Match nopnop2002 conventions** (SPI host, clock speed, offset handling)
5. ✅ **Log initialization steps** (GPIO pins, resolution, SPI speed) for debugging
6. ✅ **Handle errors gracefully** (return false on init failure, clip out-of-bounds pixels)
7. ✅ **Measure frame rate** (ESP_LOGI timestamps before/after flush())
8. ✅ **Document memory usage** (frame buffer size, driver overhead)

---

### Example: ST7789 Integration

**Reference Implementation**: See Feature 005 completed tasks (T001-T026) for a working example.

**Files**:
- `components/display/include/esp_st7789.h` - Adapter class declaration
- `components/display/src/esp_st7789.cpp` - Implementation
- `components/st7789_config/Kconfig.projbuild` - GPIO and SPI configuration
- `main/idf_component.yml` - Managed component dependency
- `components/display/src/display_init.cpp` - Factory integration

**Result**: TTGO T-Display (240×135 ST7789) supported without BotiEyes code changes.

---

## References

- **Adafruit_GFX Library**: https://github.com/adafruit/Adafruit-GFX-Library
- **ESP-IDF SPI Master**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html
- **ESP-IDF I2C**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2c.html
- **Feature 004 Architecture**: `specs/004-industrial-firmware-architecture/`
- **ST7789 Datasheet**: [Sitronix ST7789V Datasheet](https://www.displayfuture.com/Display/datasheet/controller/ST7789.pdf)
- **SSD1306 Datasheet**: [Solomon Systech SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf)

---

## Version History

| Version | Date       | Changes |
|---------|------------|---------|
| 1.0     | 2026-07-06 | Initial contract definition for ST7789 integration |

---

## Contract Compliance

**This contract is BINDING for all display drivers in botieyes firmware.**

Violations of this contract may result in:
- Build failures (missing required methods)
- Runtime crashes (unhandled errors)
- Poor user experience (slow rendering, artifacts)
- Incompatibility with BotiEyes core library

When in doubt, refer to existing implementations (`ESP_SSD1306`, `ESP_ST7789`) as reference examples.
