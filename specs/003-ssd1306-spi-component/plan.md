# Implementation Plan: SSD1306 SPI Component Integration

**Branch**: `003-ssd1306-spi-component` | **Date**: 2026-06-13 | **Spec**: [spec.md](spec.md)

**Input**: Feature specification from `/specs/003-ssd1306-spi-component/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/plan-template.md` for the execution workflow.

## Summary

Add SPI protocol support to the BotiEyes library for ESP32-S3, enabling developers to connect SSD1306 OLED displays via SPI interface. Integration uses the nopnop2002/esp-idf-ssd1306 component for hardware communication while preserving the existing Adafruit_GFX rendering layer. Configuration is build-time via Kconfig/menuconfig with sensible pin defaults (MOSI=GPIO11, SCK=GPIO12, CS=GPIO10, DC=GPIO9, RST=GPIO8). SPI communication uses SPI2_HOST with DMA at up to 10 MHz for 25 FPS display refresh.

## Technical Context

**Language/Version**: C++ (Arduino 1.8+/PlatformIO) for BotiEyes library; ESP-IDF 6.0.1 for ESP32-S3 firmware

**Primary Dependencies**: 
- nopnop2002/esp-idf-ssd1306 component (SPI hardware communication)
- Adafruit_GFX (graphics rendering - preserved unchanged)
- ESP-IDF SPI Master driver (SPI2_HOST with DMA)

**Storage**: N/A (configuration stored in sdkconfig from menuconfig)

**Testing**: Hardware validation on ESP32-S3 + SSD1306 OLED; Arduino example sketches; ESP-IDF monitor for error logging

**Target Platform**: ESP32-S3 (primary); ESP32 family (secondary compatibility)

**Project Type**: Embedded library (Arduino) + ESP-IDF firmware application

**Performance Goals**: 25 FPS display refresh (40ms frame time) with smooth eye animations

**Constraints**: 
- SPI clock ≤10 MHz with DMA enabled
- 2-second initialization timeout
- Display init must complete before rendering
- Zero changes to existing Adafruit_GFX rendering code

**Scale/Scope**: 
- Single SSD1306 display (128x64 pixels)
- 5 configurable GPIO pins (MOSI, SCK, CS, DC, RST)
- Build-time configuration only (no runtime protocol switching)

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Initial Assessment (Pre-Research)

| Principle | Status | Rationale |
|-----------|--------|-----------|
| I. Pragmatic Simplicity | ✅ PASS | Uses existing nopnop2002/esp-idf-ssd1306 component instead of custom SPI driver; avoids over-engineering |
| II. Maintainable Code | ✅ PASS | Kconfig/menuconfig provides clear, maintainable configuration without code changes; pin defaults documented |
| III. Performance-First | ✅ PASS | DMA requirement ensures 25 FPS target; timeout prevents hangs; performance measured and specified |
| IV. Hardware Abstraction | ✅ PASS | SPI added as alternative to I2C without modifying core rendering; protocol selection at build time via Kconfig |
| V. Emotion-Driven Design | ✅ PASS | Hardware layer change preserves emotion rendering logic; Adafruit_GFX layer unchanged |
| VI. Cross-Platform Emulation | ✅ PASS | SPI is ESP32-specific but emulator uses separate rendering path; no impact on PC emulation |
| VII. Extensible Architecture | ✅ PASS | Adds new protocol option without modifying existing I2C code; clear extension point via Kconfig |
| VIII. Continuous Learning | ✅ PASS | Spec clarifications documented (pin validation, timeout, DMA handling, LED behavior) |

**Gate Status**: ✅ **PASS** - All principles aligned. No violations requiring justification.

### Post-Design Assessment (After Phase 1)

**Date**: 2026-06-13
**Artifacts Reviewed**: [research.md](research.md), [data-model.md](data-model.md), [contracts/Kconfig-API.md](contracts/Kconfig-API.md), [quickstart.md](quickstart.md)

| Principle | Status | Design Validation |
|-----------|--------|-------------------|
| I. Pragmatic Simplicity | ✅ PASS | Component integration via idf_component.yml avoids custom driver complexity; Kconfig provides simple configuration |
| II. Maintainable Code | ✅ PASS | Data model clearly documents 5 entities; contracts define stable Kconfig API; quickstart enables onboarding in 15 min |
| III. Performance-First | ✅ PASS | Research confirms <1ms display update @ 10 MHz SPI with DMA; meets 25 FPS target with <3% frame time |
| IV. Hardware Abstraction | ✅ PASS | Buffer API is protocol-agnostic (same for I2C/SPI); rendering layer unchanged; conditional compilation isolates SPI code |
| V. Emotion-Driven Design | ✅ PASS | No impact on emotion logic; Adafruit_GFX rendering preserved; framebuffer format identical across protocols |
| VI. Cross-Platform Emulation | ✅ PASS | Emulator unaffected (Python-based, no ESP-IDF dependency); SPI code isolated in esp-idf/ directory |
| VII. Extensible Architecture | ✅ PASS | Kconfig choice pattern extensible to additional protocols (e.g., parallel 8080); clear separation of concerns |
| VIII. Continuous Learning | ✅ PASS | Research documented 3 major decisions with rationale; clarifications integrated into spec; quickstart captures lessons |

**Gate Status**: ✅ **PASS** - Design maintains all constitutional principles. No architectural concerns.

**Key Design Strengths**:
1. **Simplicity**: Leverages existing component, avoids reinventing SPI driver
2. **Maintainability**: Kconfig contract is versioned (1.0.0) and stability-guaranteed
3. **Performance**: DMA auto-enabled, 10 MHz clock meets target with margin
4. **Abstraction**: Protocol switching via build-time config, zero runtime overhead

**Complexity Remained Justified**: None - no violations introduced during design.

---

## Project Structure

### Documentation (this feature)

```text
specs/003-ssd1306-spi-component/
├── spec.md              # Feature specification with clarifications
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   └── Kconfig-API.md   # Build-time configuration contract
├── checklists/
│   └── requirements.md  # Spec validation checklist
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
esp-idf/
├── main/
│   ├── CMakeLists.txt           # Build configuration
│   ├── Kconfig.projbuild        # SPI pin + protocol configuration
│   ├── idf_component.yml        # nopnop2002/esp-idf-ssd1306 dependency
│   ├── main.cpp                 # Application entry point
│   ├── display_init.cpp         # SPI/I2C display initialization dispatcher
│   └── display_init.h           # Display init interface
├── components/                  # (future: if SPI wrapper needed)
└── sdkconfig                    # Generated from menuconfig

BotiEyes/
├── src/
│   ├── BotiEyes.h               # Main library header
│   ├── BotiEyes.cpp             # Core rendering logic (unchanged)
│   ├── DisplayConfig.h          # Display configuration structures
│   └── [other rendering files]  # Unchanged
└── examples/
    ├── BasicEmotion/            # Test with SPI config
    └── [other examples]/        # Verify SPI compatibility

esp-idf/ssd1306_esp32s3.md       # Hardware wiring reference (ESP32-S3 pins)
```

**Structure Decision**: Dual-target structure with ESP-IDF project (`esp-idf/`) for hardware integration and Arduino library (`BotiEyes/`) for rendering. SPI hardware layer lives in ESP-IDF with Kconfig; rendering layer in BotiEyes remains protocol-agnostic. Hardware wiring documented at project root for reference.

## Complexity Tracking

> **Fill ONLY if Constitution Check has violations that must be justified**

No complexity violations identified. Feature aligns with all constitutional principles, particularly Pragmatic Simplicity (reusing existing component) and Hardware Abstraction (protocol-agnostic rendering layer).

---

## Phase 0: Research & Design Decisions
