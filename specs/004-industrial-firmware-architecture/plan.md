# Implementation Plan: Industrial Firmware Architecture Refactoring

**Branch**: `004-industrial-firmware-architecture` | **Date**: 2026-06-30 | **Spec**: [spec.md](./spec.md)

**Input**: Feature specification from `/specs/004-industrial-firmware-architecture/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/plan-template.md` for the execution workflow.

## Summary

Refactor the existing ESP32-based BotiEyes firmware to follow professional ESP-IDF industrial architecture patterns. The refactoring introduces layered component structure (Application → Services → HAL → Drivers → ESP-IDF), event-driven inter-task communication using ESP event loops and FreeRTOS queues, proper resource protection with mutexes, hardware abstraction for multi-board support, and comprehensive system health monitoring. This is a **refactoring effort** focused on improving code quality, maintainability, and reliability—**not adding new features**. The existing BotiEyes emotion-driven eye rendering and UDP network control functionality remains unchanged; only the architectural structure and supporting infrastructure are being improved.

## Technical Context

**Language/Version**: C++ (C++11/14) for ESP-IDF firmware, Python 3.8+ for PC emulator (unchanged)

**Primary Dependencies**: 
- ESP-IDF v5.0+ (framework)
- FreeRTOS (included in ESP-IDF)
- Adafruit GFX library (display rendering)
- espressif/led_strip component (status LED)
- WiFi stack (esp_wifi, lwIP)

**Storage**: NVS (Non-Volatile Storage) for WiFi credentials, configuration, crash logs

**Testing**: 
- On-hardware validation (TTGO LoRa32 board)
- Manual integration testing via serial console
- Network control testing via Python client
- Future: pytest-embedded for automated ESP32 testing

**Target Platform**: 
- ESP32 / ESP32-S3 (dual-core Xtensa/RISC-V)
- ESP-IDF toolchain (xtensa-esp32-elf-gcc / riscv32-esp-elf-gcc)
- Primary target: TTGO LoRa32 (ESP32 + SSD1306 128×64 OLED)

**Project Type**: Embedded firmware (bare-metal + FreeRTOS)

**Performance Goals**: 
- 25 FPS eye animation (40ms frame time)
- <20ms WiFi event response latency
- <10ms network command processing
- Smooth animation with no visible tearing

**Constraints**: 
- ~280 KB usable SRAM (ESP32), ~512 KB (ESP32-S3)
- Limited heap (target <80% utilization)
- Single-threaded display access (SPI/I2C hardware constraint)
- 30-second watchdog timeout for critical tasks
- Real-time responsiveness for network control

**Scale/Scope**: 
- ~5K-10K lines of firmware code
- 5-8 FreeRTOS tasks
- 3-5 ESP-IDF components
- 2-3 board variants (I2C vs SPI display)
- Single UDP control connection at a time

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

| Principle | Status | Evaluation |
|-----------|--------|------------|
| **I. Pragmatic Simplicity** | ✅ PASS | Refactoring introduces necessary complexity (layers, HAL, event loops) that is **justified** by industrial requirements: maintainability, portability, reliability. Each abstraction solves a documented problem (e.g., HAL enables multi-board support per FR-036/037/038). Using standard ESP-IDF patterns, not inventing new paradigms. |
| **II. Maintainable Code** | ✅ PASS | Core goal of refactoring. Component modularity (US-1) enables independent development and testing. Layered architecture reduces cognitive load. Clear interfaces improve readability. Hierarchical logging aids debugging. |
| **III. Performance-First** | ✅ PASS (Post-Design) | Mitigation strategies defined in research.md: (1) Task priorities and core pinning maintain real-time guarantees; (2) Lightweight primitives (task notifications) used where possible; (3) Queue sizes bounded for latency; (4) Display rendering remains on dedicated core 1; (5) Profiling instrumentation planned. Performance metrics in data-model.md: 40ms frame target, <20ms WiFi event latency, <10ms network command latency. **Acceptance: ≤10% frame rate regression acceptable.** |
| **IV. Hardware Abstraction** | ✅ PASS | Central to refactoring (US-3). HAL layer enables I2C/SPI abstraction (FR-036), multi-LED types (FR-037), board-specific configs (FR-038). Design in data-model.md shows compile-time board selection (zero runtime overhead). |
| **V. Emotion-Driven Design** | ✅ PASS | BotiEyes core library remains untouched per assumptions. Refactoring only changes surrounding infrastructure. Emotion model and rendering logic unchanged. |
| **VI. Cross-Platform Emulation** | ✅ PASS | Python emulator remains separate and unchanged per assumptions. ESP-IDF firmware refactoring does not affect emulator. |
| **VII. Extensible Architecture** | ✅ PASS | Component structure with clear interfaces (FR-002, contracts/component-api.md) enables extension. New boards can be added via HAL configs (quickstart.md documents process). Event-driven architecture allows adding observers without modifying publishers. |
| **VIII. Continuous Learning** | ✅ PASS | This refactoring incorporates learnings from initial implementation. Architecture enforcement docs (SYNCHRONIZATION_GUIDE.md, copilot-instructions.md) encode knowledge for future work. Research.md documents design decisions and alternatives considered. |

**Overall Gate**: ✅ **PASS** (Initial: PASS with Performance Review Required → Post-Design: PASS)

**Post-Design Validation**:
- ✅ Performance mitigation strategies defined and documented (research.md Section 8)
- ✅ Profiling instrumentation planned in data-model.md and quickstart.md
- ✅ Acceptance criteria specified: ≤10% regression from 25 FPS (≥22.5 FPS)
- ✅ Design minimizes overhead: compile-time HAL selection, lightweight task notifications, bounded queues
- ✅ Real-time guarantees maintained: task priorities, core pinning, watchdog timeouts

## Project Structure

### Documentation (this feature)

```text
specs/004-industrial-firmware-architecture/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   └── component-api.md # Component interface contracts
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
esp-idf/                           # ESP-IDF firmware project
├── CMakeLists.txt                 # Project-level build config
├── sdkconfig                      # Configuration (generated from Kconfig)
├── sdkconfig.defaults.ttgo_lora32 # TTGO LoRa32 board defaults (new)
├── sdkconfig.defaults.esp32s3_spi # ESP32-S3 SPI variant defaults (new)
│
├── main/                          # Application layer (top level)
│   ├── CMakeLists.txt
│   ├── Kconfig.projbuild         # User-facing config options
│   ├── main.cpp                  # Entry point, app_main()
│   ├── app_task.cpp              # Application task (refactored from main)
│   └── app_task.h
│
├── components/                    # ESP-IDF components (modular)
│   │
│   ├── botieyes/                 # BotiEyes library wrapper (exists, minimal changes)
│   │   ├── CMakeLists.txt        # Links to ../../../BotiEyes/src
│   │   └── (symlinks to Arduino library)
│   │
│   ├── wifi_manager/             # Service: WiFi lifecycle management (new component)
│   │   ├── CMakeLists.txt
│   │   ├── Kconfig               # Component-specific config
│   │   ├── include/
│   │   │   └── wifi_manager.h   # Public API
│   │   └── src/
│   │       ├── wifi_manager.c   # Connection state machine
│   │       └── wifi_events.c    # ESP event handlers
│   │
│   ├── state_machine/            # Service: Application state management (new)
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── app_state.h      # Public API
│   │   └── src/
│   │       ├── app_state.c      # State machine logic
│   │       └── state_events.c   # State change event posting
│   │
│   ├── config_manager/           # Service: NVS abstraction (new)
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── config_manager.h
│   │   └── src/
│   │       └── config_manager.c
│   │
│   ├── health_monitor/           # Service: Watchdog & crash recovery (new)
│   │   ├── CMakeLists.txt
│   │   ├── include/
│   │   │   └── health_monitor.h
│   │   └── src/
│   │       ├── watchdog.c       # Task watchdog management
│   │       └── crash_log.c      # Crash log persistence
│   │
│   └── hal_board/                # HAL: Board-specific abstractions (new)
│       ├── CMakeLists.txt
│       ├── Kconfig               # Board selection
│       ├── include/
│       │   ├── hal_board.h      # Initialization
│       │   ├── hal_display.h    # Display abstraction
│       │   └── hal_led.h        # LED abstraction
│       └── src/
│           ├── boards/
│           │   ├── ttgo_lora32.c       # TTGO LoRa32 pin config
│           │   └── esp32s3_spi.c      # ESP32-S3 SPI variant
│           ├── hal_display_i2c.c      # I2C display implementation
│           ├── hal_display_spi.c      # SPI display implementation
│           └── hal_led.cpp            # LED wrapper (GPIO/WS2812)
│
└── managed_components/            # External dependencies (unchanged)
    ├── espressif__led_strip/
    └── fasani__adafruit_gfx/

BotiEyes/                          # Arduino library (mostly unchanged)
├── src/
│   ├── BotiEyes.cpp              # Core library
│   ├── EmotionMapper.cpp
│   ├── net/                      # Network control (minor refactor for queues)
│   │   ├── BotiEyesServer.cpp   # UDP server (refactored to use queues)
│   │   ├── CommandCodec.cpp
│   │   └── SessionManager.cpp
│   └── ...
└── examples/                      # Arduino examples (unchanged)

docs/                              # Documentation (partially exists)
├── ARCHITECTURE_ENFORCEMENT.md   # AI enforcement strategy (exists)
└── SYNCHRONIZATION_GUIDE.md      # FreeRTOS primitives guide (exists)

tests/                             # Test infrastructure (future)
└── esp-idf/                      # pytest-embedded tests (future)
```

**Structure Decision**: 

The refactored firmware follows **ESP-IDF component-based architecture** with clear layer separation:

1. **main/**: Application layer - orchestrates components, houses app_main() and application task
2. **components/**: Service and HAL layers - isolated, reusable modules with public APIs
   - **Service components**: wifi_manager, state_machine, config_manager, health_monitor
   - **HAL component**: hal_board (abstracts hardware differences)
   - **Library wrapper**: botieyes (links to shared Arduino library in BotiEyes/)
3. **BotiEyes/**: Shared library - Arduino-compatible code used by both ESP-IDF and Arduino IDE
4. **Driver layer**: Managed components (Adafruit GFX, LED strip) + ESP-IDF built-ins

This structure achieves:
- **Modularity**: Each component has clear responsibility, buildable independently (US-1)
- **Portability**: HAL isolates board-specific code, enables multi-board support (US-3)
- **Maintainability**: Changes to WiFi, state, config, or health are contained within components (US-1)
- **Testability**: Components can be tested in isolation (future pytest-embedded integration)

## Complexity Tracking

**No constitution violations requiring justification.**

All introduced complexity (layered components, HAL, event loops, mutexes, watchdog) follows standard ESP-IDF industrial patterns and is justified by functional requirements. Constitution Check passed with Performance Review action item.
