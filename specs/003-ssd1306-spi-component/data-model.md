# Data Model: SSD1306 SPI Component Integration

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md) | **Research**: [research.md](research.md)
**Date**: 2026-06-13
**Phase**: 1 (Design)

## Overview

This document defines the key data entities and their relationships for SPI display integration. All entities support the functional requirements (FR-001 through FR-016) from the specification.

---

## Entity 1: SSD1306 Display Device

**Purpose**: Represents the physical SSD1306 OLED display hardware connected via SPI interface.

**Attributes**:
| Attribute | Type | Description | Validation |
|-----------|------|-------------|------------|
| `_width` | `int` | Display width in pixels | Must be 128 |
| `_height` | `int` | Display height in pixels | Must be 64 |
| `_pages` | `int` | Number of 8-pixel vertical pages | Calculated: height / 8 = 8 |
| `_page[]` | `PAGE_t[8]` | Internal framebuffer (1024 bytes total) | 128 bytes per page |
| `_spi_device_handle` | `spi_device_handle_t` | ESP-IDF SPI device handle | Valid handle from `spi_bus_add_device()` |

**State Transitions**:
```
[Uninitialized] 
    ↓ spi_master_init() + ssd1306_init()
[Initialized & Ready]
    ↓ ssd1306_set_buffer() + ssd1306_show_buffer()
[Displaying Content]
    ↓ repeated buffer updates
[Displaying Content] (steady state)
```

**Relationships**:
- Managed by: `SPI Configuration` (initialization parameters)
- Contains: `Display Buffer` (internal framebuffer)
- Updated by: Adafruit_GFX rendering operations

**Notes**:
- Device handle is owned by the display initialization code
- Buffer is owned by the nopnop2002/esp-idf-ssd1306 component
- Display remains in "Displaying Content" state indefinitely (no shutdown)

---

## Entity 2: SPI Configuration

**Purpose**: Build-time configuration parameters for SPI interface, captured in `sdkconfig` from menuconfig.

**Attributes**:
| Attribute | Kconfig Symbol | Type | Default | Range | Description |
|-----------|----------------|------|---------|-------|-------------|
| `protocol` | `BOTIEYES_OLED_PROTOCOL_SPI` | `bool` | `false` | N/A | Protocol selection (choice: I2C or SPI) |
| `mosi_pin` | `BOTIEYES_OLED_MOSI_PIN` | `int` | `11` | 0-48 | SPI MOSI/SDA data line |
| `sck_pin` | `BOTIEYES_OLED_SCK_PIN` | `int` | `12` | 0-48 | SPI clock line |
| `cs_pin` | `BOTIEYES_OLED_CS_PIN` | `int` | `10` | -1, 0-48 | Chip select (active low), -1 = unused |
| `dc_pin` | `BOTIEYES_OLED_DC_PIN` | `int` | `9` | 0-48 | Data/Command select (low=cmd, high=data) |
| `rst_pin` | `BOTIEYES_OLED_RST_PIN` | `int` | `8` | -1, 0-48 | Reset pin (active low), -1 = tied high |
| `clock_speed_hz` | `BOTIEYES_SPI_CLOCK_SPEED_HZ` | `int` | `10000000` | 100000-10000000 | SPI clock frequency in Hz |

**Validation Rules**:
1. **Pin uniqueness**: No two SPI signal pins can share the same GPIO (except cs_pin or rst_pin = -1)
2. **GPIO restrictions** (ESP32-S3):
   - Avoid GPIO6-11 (flash/PSRAM)
   - GPIO0 has pullup resistor (avoid for CS/RST)
   - No restriction enforcement at build time (runtime detection only)
3. **Clock speed**: Maximum 10 MHz per SSD1306 datasheet (100ns minimum cycle time)
4. **Conditional visibility**: All SPI pin configs only visible when `BOTIEYES_OLED_PROTOCOL_SPI` is selected

**Relationships**:
- Configures: `SSD1306 Display Device` (via initialization functions)
- Used by: `display_init.cpp` (reads CONFIG_* macros)
- Alternative: `I2C Configuration` (mutually exclusive choice)

**Notes**:
- Configuration is compile-time only (no runtime switching per FR-011)
- Values are accessed via preprocessor macros: `CONFIG_BOTIEYES_OLED_MOSI_PIN`
- Invalid configurations detected at runtime during GPIO/SPI initialization

---

## Entity 3: Display Buffer

**Purpose**: Framebuffer holding the pixel data to be displayed, shared between Adafruit_GFX rendering and SPI transmission.

**Attributes**:
| Attribute | Type | Description | Size |
|-----------|------|-------------|------|
| `framebuffer` | `uint8_t[1024]` | Complete display buffer (128x64 pixels, 1 bit per pixel) | 1024 bytes |
| `page[i]` | `uint8_t[128]` | Single 128x8 pixel row | 128 bytes |

**Memory Layout**:
```
Byte address:  0-127    128-255   256-383   ...   896-1023
Page:          Page 0   Page 1    Page 2    ...   Page 7
Screen region: Rows 0-7 Rows 8-15 Rows 16-23 ... Rows 56-63

Within each byte (column-major, LSB = top):
Bit 0 (0x01) = Row 0 of page
Bit 1 (0x02) = Row 1 of page
...
Bit 7 (0x80) = Row 7 of page
```

**Operations**:
| Operation | Function | Description |
|-----------|----------|-------------|
| **Read** | `ssd1306_get_buffer(&dev, buffer)` | Copy component's internal buffer to external array |
| **Write** | `ssd1306_set_buffer(&dev, buffer)` | Copy external array to component's internal buffer |
| **Transmit** | `ssd1306_show_buffer(&dev)` | Send internal buffer to display via SPI (uses DMA) |
| **Clear** | `memset(buffer, 0, 1024)` | Erase all pixels (all bits = 0) |
| **Fill** | `memset(buffer, 0xFF, 1024)` | Fill all pixels (all bits = 1) |

**State Transitions**:
```
[Application Buffer]
    ↓ Adafruit_GFX drawing operations
[Application Buffer (modified)]
    ↓ ssd1306_set_buffer()
[Component Internal Buffer]
    ↓ ssd1306_show_buffer()
[SPI Transaction] → [Display Hardware]
```

**Relationships**:
- Owned by: Application code (external) or `SSD1306 Display Device` (internal)
- Modified by: Adafruit_GFX rendering functions
- Transmitted by: nopnop2002/esp-idf-ssd1306 component via SPI

**Performance Characteristics**:
- Full buffer transfer: 1024 bytes via SPI @ 10 MHz = ~0.82ms
- DMA overhead: ~0.1ms
- **Total display update time**: <1ms (well below 40ms/frame target for 25 FPS)

**Notes**:
- Buffer format is identical for I2C and SPI (protocol-agnostic rendering)
- Adafruit_GFX operates on byte-oriented buffer, no awareness of SPI vs I2C

---

## Entity 4: Status LED

**Purpose**: Visual indicator for initialization success/failure, providing user feedback when display is unavailable.

**Attributes**:
| Attribute | Type | Description | Default |
|-----------|------|-------------|---------|
| `gpio_pin` | `int` | GPIO pin number for LED | 38 (ESP32-S3 convention) |
| `led_handle` | `led_strip_handle_t*` | RMT-based LED driver handle | NULL (uninitialized) |
| `color` | `RGB(r, g, b)` | Current LED color | Off (0, 0, 0) |

**Color States**:
| State | Color | RGB Value | Meaning |
|-------|-------|-----------|---------|
| **Initializing** | Blue | (0, 0, 255) | System starting, display init in progress |
| **Success** | Green | (0, 255, 0) | Display initialized successfully |
| **Error** | Red | (255, 0, 0) | Display initialization failed (fatal) |
| **Off** | Black | (0, 0, 0) | LED disabled or not configured |

**State Transitions**:
```
[Off]
    ↓ init_status_led() success
[Blue - Initializing]
    ↓ display init success
[Green - Success]

[Blue - Initializing]
    ↓ display init failure (timeout, DMA, invalid pins)
[Red - Error] (terminal state, system halted)
```

**Validation Rules**:
1. LED initialization failure is **non-fatal** (FR-016)
2. If `gpio_pin < 0`, LED is disabled (valid configuration)
3. If LED init fails (pin conflict), system logs warning and continues

**Relationships**:
- Controlled by: `app_main()` initialization sequence
- Independent from: `SSD1306 Display Device` (optional diagnostic feature)

**Notes**:
- Uses ESP-IDF `led_strip` component with RMT peripheral
- Remains in "Red" state indefinitely if display init fails (system halts)
- Not required for operation; display is the primary interface

---

## Entity 5: Initialization Context

**Purpose**: Runtime state and timing information during display initialization, used for timeout detection and error logging.

**Attributes**:
| Attribute | Type | Description |
|-----------|------|-------------|
| `start_time_us` | `int64_t` | Timestamp when init began (microseconds since boot) |
| `timeout_us` | `int64_t` | Maximum allowed init time (2,000,000 µs = 2 seconds) |
| `elapsed_ms` | `int64_t` | Calculated elapsed time (for logging) |
| `error_code` | `esp_err_t` | ESP-IDF error code from failed operation |

**Lifecycle**:
```
[Created]
    ↓ init_ssd1306_with_timeout() entry
[Timing Active]
    ↓ SPI transactions, timeout checks
[Completed] (success) OR [Timed Out] (failure)
    ↓ destroy/return
[Released]
```

**Validation Rules**:
1. Timeout check occurs before each SPI command batch
2. Elapsed time logged on both success and failure
3. Error code propagated to caller for detailed diagnostics

**Relationships**:
- Used by: SPI initialization functions
- Captures: Error codes from ESP-IDF SPI/GPIO drivers

---

## Data Flow Diagram

```
┌─────────────────┐
│ menuconfig      │
│ (Build Time)    │
└────────┬────────┘
         │
         ↓ Generates sdkconfig.h
┌─────────────────────────┐
│ CONFIG_* Macros         │
│ (SPI Pin Configuration) │
└────────┬────────────────┘
         │
         ↓ Read by display_init.cpp
┌─────────────────────────┐      ┌──────────────────┐
│ ssd1306_spi_init()      │─────→│ Status LED       │
│ (Initialization)        │      │ (Blue → Green/Red)│
└────────┬────────────────┘      └──────────────────┘
         │
         ↓ Success
┌─────────────────────────┐
│ SSD1306 Display Device  │
│ (Initialized, Ready)    │
└────────┬────────────────┘
         │
         ↓ Rendering loop
┌─────────────────────────┐
│ Adafruit_GFX            │
│ (Draw to buffer)        │
└────────┬────────────────┘
         │
         ↓ ssd1306_set_buffer()
┌─────────────────────────┐
│ Display Buffer          │
│ (Component Internal)    │
└────────┬────────────────┘
         │
         ↓ ssd1306_show_buffer()
┌─────────────────────────┐
│ SPI Transaction (DMA)   │
└────────┬────────────────┘
         │
         ↓ Hardware
┌─────────────────────────┐
│ Physical OLED Display   │
└─────────────────────────┘
```

---

## Constraints & Invariants

### System-Wide Invariants
1. **Single protocol**: Either I2C OR SPI is active, never both (enforced by Kconfig choice)
2. **Immutable config**: SPI pin configuration cannot change after build (no runtime switching)
3. **Buffer size**: Always 1024 bytes for 128x64 display (fixed by hardware)
4. **Initialization order**: LED → SPI bus → SPI device → GPIO → Display init → Rendering

### Performance Constraints
1. **Display refresh**: <1ms per frame (1024 bytes @ 10 MHz SPI with DMA)
2. **Initialization timeout**: 2 seconds maximum (FR-014)
3. **Frame rate target**: 25 FPS (40ms/frame), display update is <3% of frame time

### Error Handling Constraints
1. **Fatal errors halt system**: Timeout, DMA failure, invalid pins (FR-008)
2. **Non-fatal errors logged**: LED initialization failure (FR-016)
3. **No recovery**: Once halted, manual reset required (no automatic restart)

---

## Summary

All entities support the functional requirements:
- **FR-001 to FR-006**: `SPI Configuration` + `SSD1306 Display Device`
- **FR-007 to FR-009**: `Initialization Context` + error logging
- **FR-010 to FR-012**: Scope/platform constraints
- **FR-013 to FR-016**: Error handling states and validation

**Ready for contract definition** (Phase 1 continues).
