# Data Model: ST7789 Display Driver Integration

**Feature**: 005-st7789-display-support | **Phase**: 1 | **Date**: 2026-07-06

## Entity Definitions

### 1. DisplayType

**Description**: Enumeration of supported display controller types for build-time selection.

**Attributes**:
- `SSD1306_I2C`: SSD1306 controller with I2C interface
- `SSD1306_SPI`: SSD1306 controller with SPI interface  
- `ST7789_SPI`: ST7789 controller with SPI interface

**Relationships**:
- Selected value determines which `DisplayConfig` variant is active
- Maps to corresponding `DisplayDriver` implementation

**Validation Rules**:
- Exactly one type selected at build time via Kconfig
- Default: `SSD1306_I2C` for backward compatibility

**State Transitions**: N/A (compile-time constant)

---

### 2. DisplayConfig

**Description**: Configuration structure containing display-specific parameters selected via menuconfig.

**Attributes (Common)**:
- `display_type`: DisplayType enum value
- `width`: Screen width in pixels (range: 0-999)
- `height`: Screen height in pixels (range: 0-999)
- `offsetx`: X-axis GRAM offset (range: 0-99)
- `offsety`: Y-axis GRAM offset (range: 0-99)

**Attributes (SSD1306_I2C)**:
- `sda_pin`: I2C data GPIO (range: 0-48)
- `scl_pin`: I2C clock GPIO (range: 0-48)
- `rst_pin`: Reset GPIO (range: -1 to 48, -1 disables)
- `i2c_addr`: I2C device address (typically 0x3C or 0x3D)

**Attributes (SSD1306_SPI)**:
- `mosi_pin`: SPI MOSI GPIO
- `sclk_pin`: SPI clock GPIO
- `cs_pin`: Chip select GPIO (range: -1 to 48, -1 disables)
- `dc_pin`: Data/command GPIO
- `rst_pin`: Reset GPIO (range: -1 to 48, -1 disables)

**Attributes (ST7789_SPI)**:
- `mosi_pin`: SPI MOSI GPIO (default GPIO 19 for TTGO T-Display)
- `sclk_pin`: SPI clock GPIO (default GPIO 18 for TTGO T-Display)
- `cs_pin`: Chip select GPIO (default GPIO 5 for TTGO T-Display, -1 for hardware control)
- `dc_pin`: Data/command GPIO (default GPIO 16 for TTGO T-Display)
- `rst_pin`: Reset GPIO (default GPIO 23 for TTGO T-Display)
- `bl_pin`: Backlight GPIO (default GPIO 4 for TTGO T-Display, -1 disables)
- `spi_host`: SPI peripheral (SPI2_HOST or SPI3_HOST)
- `clock_speed_hz`: SPI clock frequency (20-80 MHz, default 40 MHz)
- `enable_inversion`: Display color inversion (bool, default false)
- `enable_frame_buffer`: RGB565 frame buffer (bool, default false)

**Relationships**:
- Created by `display_init::createDisplayConfig()` from Kconfig values
- Consumed by driver initialization functions
- Lifetime: Created once during `hal_board_init()`, immutable afterward

**Validation Rules**:
- GPIO pins must not conflict (same pin used for multiple functions)
- GPIO numbers within valid range for target ESP32 variant
- Input-only pins (34-39 on ESP32) cannot be outputs
- Strapping pins (0, 2, 12, 15) avoided in defaults but override allowed
- Clock speed ≤80 MHz (hardware limit)

**State Transitions**: N/A (immutable after creation)

---

### 3. DisplayDriver (Interface)

**Description**: Abstract interface implemented by all display driver adapters. Provides hardware abstraction for BotiEyes rendering engine.

**Attributes**:
- `width`: Display width in pixels (read-only)
- `height`: Display height in pixels (read-only)
- `display_buffer`: Optional frame buffer pointer (NULL if direct rendering)

**Methods** (pure virtual, implemented by adapters):
- `init(DisplayConfig) -> esp_err_t`: Initialize hardware and communication
- `drawPixel(x, y, color) -> void`: Set single pixel color
- `flush() -> void`: Transfer buffer to display hardware
- `clear() -> void`: Clear all pixels to background color
- `setBrightness(level) -> void`: Adjust display brightness (if supported)

**Relationships**:
- Base class for `ESP_SSD1306` and `ESP_ST7789` adapter classes
- Extends `Adafruit_GFX` interface (provides drawLine, drawRect, etc.)
- Used by BotiEyes rendering logic (display-agnostic)

**Validation Rules**: N/A (interface definition)

**State Transitions**:
1. **Uninitialized** → `init()` → **Ready**
2. **Ready** → `clear()`/`drawPixel()` → **Dirty** (pending flush)
3. **Dirty** → `flush()` → **Ready** (hardware updated)

---

### 4. ESP_ST7789 (Adapter Implementation)

**Description**: Adafruit_GFX adapter that wraps ST7789 native driver, translating high-level graphics operations to ST7789 hardware commands.

**Attributes**:
- `_dev`: TFT_t structure (native ST7789 driver handle)
- `_width`, `_height`: Screen dimensions (inherited from Adafruit_GFX)
- `_buffer`: Optional RGB565 frame buffer (uint16_t*, 2 bytes per pixel)
- `_buffer_size`: Frame buffer size in bytes (width * height * 2)
- `_initialized`: Initialization state flag

**Methods** (override from Adafruit_GFX):
- `drawPixel(x, y, color)`: Write to frame buffer or direct SPI transfer
- `display()`: Flush frame buffer to ST7789 via SPI (implements `flush()`)
- `clearDisplay()`: Fill buffer/screen with background color (implements `clear()`)

**Methods** (ST7789-specific):
- `beginSpi(mosi, sclk, cs, dc, rst, bl) -> bool`: Initialize SPI and ST7789
- `setRotation(angle)`: Configure display orientation (0°, 90°, 180°, 270°)
- `backlightOn()`, `backlightOff()`: Control backlight GPIO

**Relationships**:
- Implements `DisplayDriver` interface via Adafruit_GFX inheritance
- Owns `TFT_t _dev` (native driver state)
- Used by `initializeDisplay()` when `CONFIG_DISPLAY_TYPE_ST7789_SPI` selected
- Delegates to `st7789.c` functions: `lcdInit()`, `lcdDrawPixel()`, `lcdFillScreen()`, etc.

**Validation Rules**:
- `beginSpi()` must be called before drawing operations
- Pixel coordinates must be within (0, 0) to (width-1, height-1)
- Frame buffer allocation may fail on low-memory devices (checked and logged)

**State Transitions**:
1. **Constructed** → `beginSpi()` → **Initialized**
2. **Initialized** → `drawPixel()`/`clearDisplay()` → **Buffer Modified**
3. **Buffer Modified** → `display()` → **Hardware Synced**

---

### 5. TFT_t (Native Driver Context)

**Description**: ST7789 driver state structure (from reference implementation). Encapsulates hardware handles and display parameters.

**Attributes**:
- `_SPIHandle`: spi_device_handle_t (ESP-IDF SPI device handle)
- `_width`, `_height`: Screen dimensions
- `_offsetx`, `_offsety`: GRAM offset for display variants
- `_dc`, `_bl`: GPIO numbers for data/command and backlight
- `_use_frame_buffer`: Frame buffer enabled flag
- `_frame_buffer`: Optional RGB565 buffer (uint16_t*)
- `_font_direction`, `_font_fill`, `_font_underline`: Font rendering state

**Relationships**:
- Owned by `ESP_ST7789` adapter (one-to-one)
- Passed to all `st7789.c` functions as first parameter
- Contains `spi_device_handle_t` allocated by `spi_bus_add_device()`

**Validation Rules**: 
- `_SPIHandle` must be valid (non-NULL) after `spi_master_init()`
- `_frame_buffer` may be NULL (direct rendering mode)

**State Transitions**: Managed by ST7789 driver functions

---

### 6. ColorFormat

**Description**: Pixel color representation for cross-driver compatibility.

**Formats**:
- **Monochrome (SSD1306)**: 1 bit per pixel (0=off, 1=on), packed in bytes
- **RGB565 (ST7789)**: 16 bits per pixel (5R:6G:5B), little-endian

**Conversion**:
- `Adafruit_GFX` uses uint16_t color values
- `ESP_SSD1306`: Non-zero → 1 (pixel on), zero → 0 (pixel off)
- `ESP_ST7789`: Pass RGB565 directly to `lcdDrawPixel()`

**Standard Colors** (Adafruit_GFX constants):
- `WHITE`: 0xFFFF (all pixels on, works on both)
- `BLACK`: 0x0000 (all pixels off, works on both)
- RGB macros: `rgb565(r, g, b)` from st7789.h

**Relationships**:
- Application code uses Adafruit_GFX color constants
- Adapters translate to native format
- Future: Color themes could map emotions to RGB565 values

**Validation Rules**: 
- RGB565 values are 0x0000-0xFFFF
- Monochrome displays ignore color information beyond on/off

---

## Entity Relationship Diagram

```text
┌─────────────────┐
│   DisplayType   │ (enum: SSD1306_I2C, SSD1306_SPI, ST7789_SPI)
└────────┬────────┘
         │ selects
         ▼
┌─────────────────┐         ┌──────────────────┐
│  DisplayConfig  │────────▶│ GPIO Validation  │
└────────┬────────┘         └──────────────────┘
         │ configures
         ▼
┌─────────────────────────────────────┐
│       DisplayDriver (Interface)     │
│  - init(), drawPixel(), flush()     │
└───────────┬─────────────────────────┘
            │ implemented by
      ┌─────┴──────┐
      ▼            ▼
┌────────────┐  ┌────────────┐
│ESP_SSD1306 │  │ESP_ST7789  │
│  (adapter) │  │  (adapter) │
└─────┬──────┘  └─────┬──────┘
      │               │ owns
      │               ▼
      │         ┌──────────┐
      │         │  TFT_t   │────────▶ spi_device_handle_t
      │         │ (native) │          (ESP-IDF SPI)
      │         └──────────┘
      │
      └──────────────────┬──────────────────┐
                         ▼                  ▼
                  ┌────────────┐    ┌────────────────┐
                  │ BotiEyes   │    │  Adafruit_GFX  │
                  │ Rendering  │    │   (graphics)   │
                  └────────────┘    └────────────────┘
                         │                  │
                         └──────┬───────────┘
                                ▼
                         ┌────────────┐
                         │ColorFormat │ (RGB565/Monochrome)
                         └────────────┘
```

---

## Data Flow: Initialization

```text
1. hal_board_init() [hal_board.c]
   │
   ├─▶ Read CONFIG_DISPLAY_TYPE from Kconfig
   │
   ├─▶ createDisplayConfig() [display_init.cpp]
   │   └─▶ Returns DisplayConfig with GPIO pins, dimensions
   │
   ├─▶ initializeDisplay() [display_init.cpp]
   │   │
   │   ├─▶ if (ST7789_SPI):
   │   │   ├─▶ Create ESP_ST7789(width, height)
   │   │   ├─▶ beginSpi(mosi, sclk, cs, dc, rst, bl)
   │   │   │   ├─▶ spi_master_init() [st7789.c]
   │   │   │   │   ├─▶ gpio_reset_pin(), gpio_set_direction()
   │   │   │   │   ├─▶ spi_bus_initialize()
   │   │   │   │   └─▶ spi_bus_add_device() → _SPIHandle
   │   │   │   └─▶ lcdInit() [st7789.c]
   │   │   │       ├─▶ Send initialization commands (0x01, 0x11, 0x3A, ...)
   │   │   │       └─▶ Allocate frame buffer (if CONFIG_FRAME_BUFFER)
   │   │   └─▶ Return Adafruit_GFX* pointer
   │   │
   │   └─▶ else if (SSD1306_I2C/SPI):
   │       └─▶ Create ESP_SSD1306 (existing code path)
   │
   └─▶ Return ESP_OK or ESP_FAIL
```

---

## Data Flow: Rendering

```text
BotiEyes::render() [BotiEyes.cpp]
   │
   ├─▶ gfx->fillScreen(BLACK)  [Adafruit_GFX method]
   │   └─▶ ESP_ST7789::drawPixel() x many times
   │       └─▶ if (frame_buffer): _buffer[y*width + x] = color
   │           else: lcdDrawPixel(&_dev, x, y, color) [st7789.c]
   │
   ├─▶ gfx->fillCircle(x, y, r, WHITE)  [Draw eye pupil]
   │   └─▶ ESP_ST7789::drawPixel() x many times
   │
   └─▶ flushable->flush()  [DisplayFlushable interface]
       └─▶ ESP_ST7789::display()
           └─▶ if (frame_buffer): 
                   lcdDrawMultiPixels() / lcdDrawColors() [bulk SPI transfer]
               else: 
                   No-op (already written directly)
```

---

## Storage & Lifetime

### Compile-Time (Kconfig)
- `CONFIG_DISPLAY_TYPE`: Stored in `sdkconfig`, persists across builds
- `CONFIG_ST7789_*_GPIO`: Pin assignments, stored in `sdkconfig`
- `CONFIG_FRAME_BUFFER`: Boolean, stored in `sdkconfig`

### Runtime (RAM)
- `DisplayConfig`: Stack-allocated in `hal_board_init()`, short-lived
- `ESP_ST7789`: Heap-allocated by `initializeDisplay()`, lifetime = application
- `TFT_t`: Embedded in ESP_ST7789, lifetime = adapter lifetime
- `_frame_buffer`: Heap-allocated (MALLOC_CAP_DEFAULT), ~115KB for 240x240, optional

### Persistent (NVS)
- None (display configuration is compile-time only)

---

## Performance Considerations

### Memory Usage (ST7789 240x135 default)
- **Frame buffer enabled**: 64,800 bytes (240 * 135 * 2) = ~65 KB
- **Frame buffer disabled**: <1 KB (TFT_t struct + adapter overhead)
- **Trade-off**: Speed vs SRAM availability (65KB is 12.5% of ESP32 SRAM)

### SPI Transfer Rates (240x135 resolution)
- **20 MHz clock**: 2.5 MB/s theoretical, ~65KB frame in 26ms
- **40 MHz clock**: 5 MB/s theoretical, ~65KB frame in 13ms  
- **80 MHz clock**: 10 MB/s theoretical, ~65KB frame in 7ms
- **Target**: 10 FPS = 100ms/frame (easily achievable at 20-40 MHz with frame buffer)

### Pixel Operations
- **Frame buffer mode**: Write to RAM, bulk flush at end (fast)
- **Direct mode**: SPI transaction per pixel/region (slower but lower memory)

---

## Validation & Error Handling

### Initialization Errors
- **GPIO conflict**: Logged as error, returns ESP_FAIL
- **SPI bus init failure**: assert() in reference implementation (fail-fast)
- **Frame buffer alloc failure**: Logged, falls back to direct rendering
- **Display not responding**: Initialization commands may timeout (no hardware detection)

### Runtime Errors  
- **Out-of-bounds pixel**: Clipped or no-op (Adafruit_GFX handles)
- **Uninitialized display**: Undefined behavior (init is prerequisite)

### Recovery Strategy
- Display initialization errors are fatal (HAL layer returns ESP_FAIL)
- Application should check `hal_board_init()` return code
- No runtime recovery (device restart required)

---

## Constraints & Assumptions

### Hardware Constraints
- ST7789 requires SPI interface (no I2C variant)
- Minimum 5 GPIOs required (MOSI, SCLK, DC, RESET; CS and BL optional)
- ESP32 family required (uses ESP-IDF drivers)

### Software Constraints
- ESP-IDF v5.0+ required (spi_bus_initialize API)
- Adafruit_GFX library required (rendering abstraction)
- FreeRTOS required (vTaskDelay for initialization delays)

### Assumptions
- Single display per device (no multi-display support)
- Display dimensions known at compile time (no dynamic detection)
- SPI bus not shared with other devices (dedicated bus configuration)
- Display panel matches ST7789 initialization sequence (most variants compatible)

---

## Future Extensions

### Potential Entity Additions
- **DisplayTheme**: Maps emotions to RGB565 colors (e.g., red for anger)
- **DisplayOrientation**: Runtime rotation support (0°/90°/180°/270°)
- **DisplayBrightnessControl**: PWM backlight dimming
- **MultiDisplayManager**: Support for multiple displays

### Backward Compatibility
- All future entities must implement `DisplayDriver` interface
- Kconfig extensibility: New display types added to `DisplayType` enum
- No breaking changes to BotiEyes core API
