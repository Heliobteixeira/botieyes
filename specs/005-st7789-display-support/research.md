# Research: ST7789 Display Driver Integration

**Feature**: 005-st7789-display-support | **Phase**: 0 | **Date**: 2026-07-06

## Research Questions

### 1. ST7789 Component Structure & Integration Approach

**Question**: How should the ST7789 component from the reference implementation be integrated into the botieyes project structure?

**Decision**: Create a dedicated `esp-idf/components/st7789` component that mirrors the reference implementation structure.

**Rationale**:
- Reference implementation at `/Users/helioteixeira/dev/esp-idf-st7789/components/st7789/` is a complete, tested ESP-IDF component
- Component structure includes: `st7789.{c,h}`, `fontx.{c,h}`, `CMakeLists.txt`, `Kconfig.projbuild`
- Direct integration preserves proven GPIO configurations and SPI initialization sequences
- Maintains compatibility with ESP-IDF component dependency system
- Allows future updates from upstream if needed

**Alternatives considered**:
- **Merge into existing display component**: Rejected. ST7789 is ~2000 LOC with font rendering. Mixing driver implementations violates single responsibility principle.
- **Rewrite from scratch**: Rejected. Reference implementation is tested on target hardware. Rewriting introduces regression risk without benefit.

---

### 2. Display Type Selection Mechanism

**Question**: How should users select between SSD1306 and ST7789 displays at build time?

**Decision**: Add `CONFIG_DISPLAY_TYPE` enum to `hal_board/Kconfig.projbuild` with three options: SSD1306_I2C, SSD1306_SPI, ST7789_SPI.

**Rationale**:
- Follows ESP-IDF standard pattern for hardware variant selection
- Compile-time selection eliminates runtime overhead and unused code
- Centralizes display selection in HAL layer (matches Feature 004 architecture)
- Existing `main/Kconfig.projbuild` handles application config; HAL handles hardware config
- Allows future display types to be added as additional enum values

**Implementation pattern** (from ESP-IDF best practices):
```c
// hal_board/Kconfig.projbuild
choice DISPLAY_TYPE
    prompt "Display controller type"
    default DISPLAY_TYPE_SSD1306_I2C
    config DISPLAY_TYPE_SSD1306_I2C
        bool "SSD1306 I2C"
    config DISPLAY_TYPE_SSD1306_SPI
        bool "SSD1306 SPI"
    config DISPLAY_TYPE_ST7789_SPI
        bool "ST7789 SPI"
endchoice
```

**Alternatives considered**:
- **Runtime selection via NVS**: Rejected. Adds complexity, requires all drivers linked, wastes flash space.
- **Separate firmware builds**: Rejected. User-hostile (manual build script modification).
- **Component-level Kconfig**: Rejected. Display selection is board-level concern, belongs in HAL.

---

### 3. GPIO Pin Configuration Strategy

**Question**: How should GPIO pin assignments for ST7789 be configured and what defaults should be used?

**Decision**: Use reference implementation's Kconfig pattern with validated pin defaults from `/Users/helioteixeira/dev/esp-idf-st7789/components/st7789/Kconfig.projbuild`.

**Validated GPIO defaults** (targeting TTGO T-Display ESP32 with 240x135 ST7789):
- **MOSI**: GPIO 19 (TTGO T-Display), GPIO 23 (generic ESP32)
- **SCLK**: GPIO 18 (TTGO T-Display), GPIO 18 (generic ESP32)
- **CS**: GPIO 5 (TTGO T-Display), GPIO -1 (disabled for generic)
- **DC**: GPIO 16 (TTGO T-Display), GPIO 27 (generic ESP32)
- **RESET**: GPIO 23 (TTGO T-Display), GPIO 33 (generic ESP32)
- **BL (Backlight)**: GPIO 4 (TTGO T-Display), GPIO 32 (generic ESP32)

**Rationale**:
- These pins successfully tested on reference hardware
- Avoid GPIO 0, 2, 12, 15 conflicts (strapping pins)
- Per-chip defaults match silicon capabilities (input-only pins on ESP32: 34-39)
- Menuconfig allows override for custom hardware

**Alternatives considered**:
- **Hardcoded pins**: Rejected. Inflexible for custom boards.
- **Runtime configuration**: Rejected. Pins set at boot, no need for runtime changes.
- **Different defaults**: Rejected. Using untested pins increases risk of hardware conflicts.

---

### 4. Adafruit_GFX Abstraction for ST7789

**Question**: How should ST7789 integrate with the existing Adafruit_GFX-based rendering layer used by BotiEyes?

**Decision**: Create `ESP_ST7789` class parallel to existing `ESP_SSD1306` class, both implementing Adafruit_GFX interface.

**Rationale**:
- BotiEyes rendering logic already uses `Adafruit_GFX` interface (display-agnostic)
- SSD1306 adapter exists at `esp-idf/components/display/include/esp_ssd1306.h`
- ST7789 adapter will follow same pattern: inherit `Adafruit_GFX`, wrap native driver
- `display_init.cpp` already implements `initializeDisplay()` factory pattern
- Zero changes required in BotiEyes core library (fully abstracted)

**Class structure**:
```cpp
// esp_st7789.h
class ESP_ST7789 : public Adafruit_GFX {
public:
    ESP_ST7789(uint16_t w, uint16_t h);
    bool beginSpi(int mosi, int sclk, int cs, int dc, int rst, int bl);
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void display();  // Flush frame buffer to hardware
private:
    TFT_t _dev;  // Native st7789 driver handle
    uint16_t *_buffer;  // Optional frame buffer (RGB565)
};
```

**Alternatives considered**:
- **Direct ST7789 API in BotiEyes**: Rejected. Breaks hardware abstraction, couples to driver.
- **Generic C interface**: Rejected. C++ abstraction already proven with SSD1306.
- **Virtual base class**: Rejected. Adafruit_GFX already provides needed abstraction.

---

### 5. Color Format Handling (Monochrome vs RGB565)

**Question**: How should the rendering layer handle SSD1306 (1-bit monochrome) vs ST7789 (16-bit RGB565)?

**Decision**: BotiEyes rendering logic uses Adafruit_GFX color values (uint16_t). Adapters convert to native format.

**Rationale**:
- Adafruit_GFX already defines standard color constants (WHITE, BLACK, RGB565 colors)
- SSD1306 adapter converts non-zero values to 1 (lit pixel)
- ST7789 adapter passes RGB565 values directly to driver
- BotiEyes core can use simple colors (WHITE/BLACK) or full RGB565 for color displays
- No conditional compilation needed in application layer

**Rendering strategy**:
- **Eye outlines, pupils**: Use WHITE (0xFFFF) - works on both displays
- **Background**: Use BLACK (0x0000) - works on both displays
- **Future enhancements**: Could use color gradients on ST7789 (e.g., red angry eyes, blue calm eyes)

**Alternatives considered**:
- **Color theme configuration**: Deferred to future feature. Start with monochrome compatibility.
- **Separate rendering code paths**: Rejected. Violates DRY, increases maintenance burden.
- **Runtime color detection**: Rejected. Build-time selection sufficient.

---

### 6. Frame Buffer Memory Management

**Question**: Should ST7789 use frame buffer (65KB for 240x135) or direct rendering given ESP32 SRAM constraints?

**Decision**: Make frame buffer **optional via Kconfig**, disabled by default. Follow reference implementation pattern.

**Rationale**:
- ESP32 has ~520KB SRAM total; 240x135 RGB565 buffer = 65KB (12.5% of SRAM)
- Reference implementation supports both modes via `CONFIG_FRAME_BUFFER` Kconfig option
- Frame buffer improves performance (batch transfers) but not essential for 10 FPS target
- Direct rendering mode supported by ST7789 driver (pixel-by-pixel or region writes)
- ESP32-S3 with PSRAM can enable frame buffer for better performance
- 240x135 resolution is more memory-friendly than 240x240 (65KB vs 115KB)

**Memory tradeoff analysis**:
- **With frame buffer**: Faster rendering (DMA bulk transfers), smoother animation, 65KB SRAM
- **Without frame buffer**: More SRAM for other tasks, slightly slower rendering
- **Target performance**: 10 FPS achievable without frame buffer (40ms budget, tested in reference)

**Alternatives considered**:
- **Always use frame buffer**: Rejected. Not all boards have sufficient SRAM/PSRAM.
- **Partial frame buffer**: Deferred. Adds complexity, premature optimization.
- **Dynamic allocation**: Supported. Reference implementation allocates at init with error handling.

---

### 7. SPI Bus Configuration

**Question**: Which SPI host and clock speed should be used for ST7789?

**Decision**: Use SPI2_HOST (default) with 20 MHz clock speed, configurable up to 80 MHz via Kconfig.

**Rationale from reference implementation**:
- Reference uses `CONFIG_SPI2_HOST` / `CONFIG_SPI3_HOST` Kconfig choice
- Default clock: 20 MHz (SPI_MASTER_FREQ_20M)
- Reference tests show stable operation up to 80 MHz on ESP32
- SPI2_HOST (HSPI) available on all ESP32 variants
- SPI3_HOST (VSPI) available on ESP32/S2/S3 but not C3

**SPI configuration from reference** (`st7789.c`):
```c
#define SPI_DEFAULT_FREQUENCY SPI_MASTER_FREQ_20M  // 20MHz
spi_device_interface_config_t devcfg = {
    .clock_speed_hz = clock_speed_hz,  // Configurable
    .mode = 3,                          // SPI mode 3
    .queue_size = 7,                    // Transaction queue depth
    .flags = SPI_DEVICE_NO_DUMMY
};
```

**Alternatives considered**:
- **I2C mode**: N/A. ST7789 is SPI-only controller.
- **Parallel interface**: N/A. Reference uses SPI, parallel adds complexity.
- **Higher default clock**: Conservative 20 MHz ensures compatibility. Users can increase if hardware supports.

---

### 8. Display Initialization Sequence

**Question**: What initialization sequence ensures ST7789 compatibility across different panel variants?

**Decision**: Use reference implementation's initialization sequence verbatim.

**Key initialization steps** (from reference `lcdInit()` in st7789.c):
1. Software Reset (0x01)
2. Sleep Out (0x11) + 255ms delay
3. Interface Pixel Format (0x3A): 0x55 (16-bit RGB565)
4. Memory Data Access Control (0x36): 0x00 (default orientation)
5. Display Inversion On (0x21) if CONFIG_INVERSION
6. Normal Display Mode On (0x13)
7. Display ON (0x29) + 255ms delay
8. Backlight GPIO enable (if configured)

**Rationale**:
- Sequence tested on hardware with reference implementation
- Delays (150ms, 255ms) required by ST7789 datasheet for power-on and mode changes
- Orientation and inversion configurable via Memory Access Control (0x36 command)
- Matches standard ST7789 application notes from display vendors

**Alternatives considered**:
- **Simplified sequence**: Rejected. Initialization is sensitive, changes risk display malfunction.
- **Dynamic orientation**: Deferred. 0x36 parameter can be exposed in future for screen rotation.

---

### 9. Integration with Feature 004 HAL Architecture

**Question**: How does ST7789 integration comply with Feature 004's industrial firmware architecture?

**Decision**: ST7789 driver is a component. HAL board layer selects and initializes the driver. Application uses abstraction.

**Architecture layers** (compliance with Feature 004):
- **Application Layer** (`main/main.cpp`): Uses `Adafruit_GFX*` interface, display-agnostic
- **HAL Layer** (`components/hal_board`): Initializes selected display via `hal_board_init()`
- **Display Abstraction** (`components/display`): Provides Adafruit_GFX adapters
- **Driver Layer** (`components/st7789`, `components/ssd1306`): Hardware-specific implementations
- **ESP-IDF Drivers**: `esp_driver_spi`, `esp_driver_gpio` (used by st7789)

**Communication patterns** (from ARCHITECTURE_ENFORCEMENT.md):
- ✅ Component boundaries: `st7789/include/*.h` public API, `st7789/src/*.c` private
- ✅ No direct hardware access in application: GPIO/SPI only in driver layer
- ✅ Proper error handling: `esp_err_t` return codes, error logging
- ✅ Mutex for shared resource: Display access serialized (single task in current design)

**Alternatives considered**:
- **Application-level driver selection**: Rejected. Violates HAL abstraction.
- **Runtime driver loading**: Rejected. Embedded systems use compile-time linking.

---

### 10. Backward Compatibility with Existing Configurations

**Question**: How to ensure existing SSD1306 configurations continue working unchanged?

**Decision**: Default `CONFIG_DISPLAY_TYPE` to `SSD1306_I2C`. Existing Kconfig values preserved.

**Compatibility matrix**:

| Existing Config | Action | Result |
|----------------|--------|--------|
| No `CONFIG_DISPLAY_TYPE` set | Default to SSD1306_I2C | Existing behavior preserved |
| SSD1306 I2C pins configured | Remain in `main/Kconfig.projbuild` | Works unchanged |
| SSD1306 SPI pins configured | Remain in display component | Works unchanged |
| Build without modification | Default build uses SSD1306 I2C | No breaking changes |

**Migration path for users**:
1. Run `idf.py menuconfig`
2. Navigate to "Display Configuration" or "HAL Board Configuration"
3. Select display type (SSD1306_I2C, SSD1306_SPI, or ST7789_SPI)
4. Configure GPIO pins for selected type
5. Build and flash

**Testing requirements**:
- ✅ Build with default config → SSD1306 I2C
- ✅ Build with ST7789 selected → ST7789 driver linked
- ✅ Existing SSD1306 hardware → no code changes needed

**Alternatives considered**:
- **Automatic detection**: Rejected. No reliable way to detect display type at runtime without config.
- **Separate firmware images**: Rejected. Menuconfig provides better UX than manual builds.

---

## Summary of Key Decisions

| Area | Decision | Validated By |
|------|----------|--------------|
| **Component structure** | Dedicated st7789 component | Reference repo structure |
| **Display selection** | Kconfig enum in hal_board | ESP-IDF standard pattern |
| **GPIO defaults** | Reference implementation values | Hardware testing |
| **Rendering abstraction** | Adafruit_GFX interface | Existing SSD1306 adapter |
| **Color format** | RGB565 passed through adapters | Adafruit_GFX standard |
| **Frame buffer** | Optional, disabled by default | Reference implementation |
| **SPI configuration** | SPI2_HOST, 20 MHz default | Reference implementation |
| **Initialization** | Reference sequence verbatim | ST7789 datasheet + tests |
| **HAL integration** | Feature 004 architecture compliant | ARCHITECTURE_ENFORCEMENT.md |
| **Backward compatibility** | SSD1306 default, no breaking changes | Existing config preserved |

All research questions resolved. Ready for Phase 1 design artifacts.
