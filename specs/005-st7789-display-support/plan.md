# Implementation Plan: ST7789 Display Driver Support

**Branch**: `005-st7789-display-support` | **Date**: 2026-07-06 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `specs/005-st7789-display-support/spec.md`

## Summary

Add ST7789 SPI color display driver support to botieyes firmware while maintaining backward compatibility with existing SSD1306 displays. Integrate the nopnop2002/st7789 component from the ESP Component Registry (same author as the existing SSD1306 driver), extend the HAL layer architecture, and provide build-time configuration through menuconfig for selecting display type and GPIO pin assignments.

## Technical Context

**Language/Version**: C++ (Arduino/PlatformIO for embedded library), C (ESP-IDF v5.0+ for firmware)

**Primary Dependencies**: 
- ESP-IDF v5.0+ (esp_driver_spi, esp_driver_gpio, FreeRTOS)
- Adafruit GFX (managed component for rendering abstraction)
- nopnop2002/st7789 (managed component via ESP Component Registry)
- nopnop2002/esp-idf-ssd1306 (existing, for backward compatibility)

**Storage**: N/A (embedded firmware with NVS for configuration)

**Testing**: Hardware validation on ESP32 with ST7789 display (target board matches reference implementation)

**Target Platform**: ESP32/ESP32-S2/ESP32-S3/ESP32-C3 with ST7789 SPI TFT displays (240x135 default, other resolutions supported)

**Project Type**: Embedded firmware component (ESP-IDF component library)

**Performance Goals**: 
- Display initialization <2 seconds from boot
- Animation rendering ≥10 FPS (smooth eye movement)
- SPI communication at 20-80 MHz (configurable)

**Constraints**: 
- Frame buffer optional (65KB for 240x135 RGB565) due to ESP32 SRAM limits
- Must work with Feature 004 industrial firmware architecture patterns
- Backward compatible with SSD1306 I2C/SPI configurations

**Scale/Scope**: 
- Single ESP-IDF component (~2000 LOC from reference ST7789 driver)
- 7 GPIO pins configurable (MOSI, SCLK, CS, DC, RESET, BL + power)
- 2 display types initially (SSD1306, ST7789), designed for future expansion
- Default resolution: 240x135 (TTGO T-Display), configurable via menuconfig

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

**✓ I. Pragmatic Simplicity**: Display type selection via menuconfig is simplest approach (standard ESP-IDF pattern). Reference ST7789 component reused rather than reimplemented.

**✓ II. Maintainable Code**: Common display interface isolates driver-specific code. Clear separation: component/st7789 (driver), component/display (abstraction), hal_board (initialization).

**✓ III. Performance-First Design**: ST7789 SPI can run at 20-80 MHz (vs I2C ~400 kHz). Frame buffer optional to balance speed vs memory. nopnop2002 component is proven and optimized. Must profile frame rates after integration.

**✓ IV. Hardware Abstraction**: Display interface already abstracts SSD1306. ST7789 extends same abstraction. Core BotiEyes logic unchanged (uses Adafruit_GFX interface).

**✓ V. Emotion-Driven Design**: N/A for this feature (display driver only, does not affect emotion model).

**✓ VI. Cross-Platform Emulation**: Python emulator unchanged (uses Pygame). ST7789 is hardware-only addition.

**✓ VII. Extensible Architecture**: Design explicitly supports future display drivers by implementing common interface. Display type enum extensible.

**✓ VIII. Continuous Learning**: nopnop2002 community-maintained component provides validated configuration values for common hardware (TTGO T-Display, etc.).

## Project Structure

### Documentation (this feature)

```text
specs/005-st7789-display-support/
├── plan.md              # This file
├── research.md          # Phase 0: ST7789 integration approach
├── data-model.md        # Phase 1: Display configuration entities
├── quickstart.md        # Phase 1: Developer guide for ST7789 setup
└── contracts/           # Phase 1: Display driver interface contract
    └── display-driver-interface.h
```

### Source Code (repository root)

```text
esp-idf/
├── components/
│   ├── st7789/                      # NEW: ST7789 driver component
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig.projbuild       # Display type selection & GPIO config
│   │   ├── include/
│   │   │   └── st7789.h            # Public ST7789 API
│   │   └── src/
│   │       ├── st7789.c            # ST7789 driver implementation
│   │       ├── fontx.c             # Font support (from reference)
│   │       └── fontx.h
│   │
│   ├── display/                     # MODIFIED: Extended for ST7789
│   │   ├── include/
│   │   │   ├── display_init.h      # Updated: ST7789 initialization
│   │   │   ├── esp_ssd1306.h       # Existing: SSD1306 adapter
│   │   │   └── esp_st7789.h        # NEW: ST7789 Adafruit_GFX adapter
│   │   └── src/
│   │       ├── display_init.cpp    # Updated: Select driver based on Kconfig
│   │       ├── esp_ssd1306.cpp     # Existing: SSD1306 implementation
│   │       └── esp_st7789.cpp      # NEW: ST7789 implementation
│   │
│   └── hal_board/                   # MODIFIED: Display type selection
│       ├── Kconfig.projbuild       # NEW: Display type enum
│       └── src/
│           └── hal_board.c         # Updated: Initialize selected display
│
└── main/
    ├── Kconfig.projbuild           # Existing: BotiEyes app config
    └── main.cpp                     # Minimal changes (already uses abstraction)

BotiEyes/                            # Arduino library (UNCHANGED)
└── src/
    └── *.{h,cpp}                   # BotiEyes core (display-agnostic)
```

**Structure Decision**: 
- ST7789 driver as dedicated ESP-IDF component following reference implementation structure
- Display component extended (not replaced) to support multiple drivers via Adafruit_GFX adapters
- HAL board layer handles build-time driver selection based on Kconfig
- Arduino library layer remains unchanged (display-agnostic by design)

## Complexity Tracking

> No constitutional violations to justify. All principles satisfied.

**Rationale for multi-component design**:
- Separate st7789 component: Maintains upstream compatibility with reference implementation
- Display abstraction component: Already exists, natural extension point
- HAL board component: Feature 004 architecture requirement, handles hardware differences

This follows ESP-IDF best practices for driver componentization.
