# Implementation Plan: Emotion-Driven Bot Eyes Library

**Branch**: `001-emotion-driven-eyes` | **Date**: 2026-04-17 | **Spec**: [spec.md](spec.md)
**Input**: Feature specification from `/specs/001-emotion-driven-eyes/spec.md`

**Note**: This template is filled in by the `/speckit.plan` command. See `.specify/templates/plan-template.md` for the execution workflow.

## Summary

Build a parametric emotion-driven eye animation library for OLED displays using valence-arousal model. Primary requirement: Enable AI-driven development through PC emulation with PNG export for multimodal feedback. The library provides continuous emotional expression (vs discrete moods in RoboEyes), independent 2D eye position control, and cross-platform support (Arduino Mega, ESP32, PC emulator). Technical approach: Minimal dependencies (Adafruit GFX only for embedded), static memory allocation, standalone C++ library structure for Arduino IDE, MVP prioritizing emulator PNG export before full embedded implementation.

## Technical Context

**Language/Version**: C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator  
**Primary Dependencies**: Adafruit GFX (embedded rendering only); Pygame (emulator only)  
**Storage**: N/A (stateless library; emotion/position state in RAM only)  
**Testing**: NEEDS CLARIFICATION (unit tests for mapping functions; emulator visual validation; Arduino hardware validation)  
**Target Platform**: Arduino Mega 2560 (8KB SRAM), ESP32 (520KB SRAM), PC emulator (Windows/macOS/Linux)
**Project Type**: Embedded library (Arduino IDE compilation) + PC emulator (development tool)  
**Performance Goals**: 20-25 FPS (Mega), 30-60 FPS (ESP32), 60+ FPS (PC emulator); <300ms transition latency  
**Constraints**: Static allocation only (no malloc/new); 8KB SRAM minimum (Mega); <6KB library footprint to leave >2KB for user code; minimal dependencies  
**Scale/Scope**: Single library; 5 geometric primitives per eye; 10+ emotion anchors; 3 built-in animations; 2 platform targets + emulator

## Constitution Check

*GATE: Must pass before Phase 0 research. Re-check after Phase 1 design.*

### Initial Check (Pre-Research)

| Principle | Status | Notes |
|-----------|--------|-------|
| I. Pragmatic Simplicity | ✅ **PASS** | Minimal dependencies (Adafruit GFX only); static allocation; built-in animations only; hardcoded mapping formulas |
| II. Maintainable Code | ✅ **PASS** | Standalone C++ library structure; clear .h/.cpp separation; Arduino IDE conventions; minimal API surface |
| III. Performance-First | ✅ **PASS** | Explicit FPS targets (20-60 FPS); profiling via emulator before embedded; <300ms transitions; static allocation avoids heap fragmentation |
| IV. Hardware Abstraction | ✅ **PASS** | Display adapters (OLED vs Emulator); platform profiles (Mega/ESP32/PC); rendering logic separated from hardware |
| V. Emotion-Driven | ✅ **PASS** | Valence-arousal continuous model core requirement; explicit emotion mapping table; parametric expressions |
| VI. Cross-Platform Emulation | ✅ **PASS** | PC emulator is MVP priority #1; PNG export for AI feedback before Arduino implementation |
| VII. Extensible Architecture | ⚠️ **JUSTIFIED** | Mapping formulas NOT customizable in v1 (Q16 clarification) - **Principle I (Pragmatic Simplicity) takes priority** per governance. Extension points exist for display adapters and future emotion models. |
| VIII. Continuous Learning | ✅ **PASS** | 17 clarifications documented (Q1-Q17); architecture decisions captured; MVP approach enables iterative learning |

**Gate Result**: ✅ **PASS** - One justified complexity tradeoff (extensibility deferred to v2 in favor of simplicity)

**Justification for Principle VII Limitation**:
- Per governance, Principle I (Pragmatic Simplicity) has higher priority than Principle VII (Extensibility)
- Custom mapping API adds significant complexity (validation, runtime safety, memory overhead)
- Baseline formulas designed to work across use cases
- Extension deferred to v2 based on user feedback
- Users needing customization can fork library (open source)

---

### Post-Design Check (After Phase 1)

| Principle | Status | Design Validation |
|-----------|--------|-------------------|
| I. Pragmatic Simplicity | ✅ **PASS** | Data model minimal (96 bytes total state); API surface small (12 public methods); no unnecessary abstractions |
| II. Maintainable Code | ✅ **PASS** | Clear entity separation (EmotionState, EyePositionState, ExpressionParameters); documented contracts; straightforward data flow |
| III. Performance-First | ✅ **PASS** | Integer math planned; lookup tables in PROGMEM; GFXcanvas offscreen rendering; I2C fast mode specified; memory budget validated (1.6KB/8KB = 20% usage) |
| IV. Hardware Abstraction | ✅ **PASS** | DisplayConfig explicit; renderer separated from logic; EyeRenderer abstraction enables future displays |
| V. Emotion-Driven | ✅ **PASS** | Valence-arousal core to all design; EmotionMapper central entity; continuous model throughout |
| VI. Cross-Platform Emulation | ✅ **PASS** | PC emulator specified in architecture; PNG export in API contract; MockDisplay for testing |
| VII. Extensible Architecture | ✅ **PASS** | Despite non-customizable mapping, architecture supports new display types via DisplayConfig enum, new animations via AnimationState enum, future emotion models via EmotionMapper refactor |
| VIII. Continuous Learning | ✅ **PASS** | Research.md documents testing insights; 17 clarifications referenced in design; API contract traceable to spec decisions |

**Post-Design Gate Result**: ✅ **PASS** - All principles satisfied; design ready for task generation

**Notes**:
- Memory budget confirmed feasible (1.6KB used / 8KB available on Mega)
- Performance targets achievable per research findings (20 FPS Mega, 30-60 FPS ESP32)
- Testing strategy defined (PlatformIO + Unity + MockDisplay)
- No new complexity violations introduced during design

## Project Structure

### Documentation (this feature)

```text
specs/[###-feature]/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

## Project Structure

### Documentation (this feature)

```text
specs/001-emotion-driven-eyes/
├── plan.md              # This file (/speckit.plan command output)
├── research.md          # Phase 0 output (/speckit.plan command)
├── data-model.md        # Phase 1 output (/speckit.plan command)
├── quickstart.md        # Phase 1 output (/speckit.plan command)
├── contracts/           # Phase 1 output (/speckit.plan command)
│   └── BotiEyes-API.md  # Public API contract
└── tasks.md             # Phase 2 output (/speckit.tasks command - NOT created by /speckit.plan)
```

### Source Code (repository root)

```text
# Arduino Library Structure (Arduino IDE standard)
BotiEyes/
├── src/
│   ├── BotiEyes.h               # Main library header (public API)
│   ├── BotiEyes.cpp             # Main library implementation
│   ├── EmotionState.h           # Emotion state data structure
│   ├── EmotionState.cpp
│   ├── EmotionMapper.h          # Valence-arousal → expression parameters
│   ├── EmotionMapper.cpp
│   ├── EyePosition.h            # 2D eye position state
│   ├── EyePosition.cpp
│   ├── Interpolator.h           # Smooth transitions with easing
│   ├── Interpolator.cpp
│   ├── EyeRenderer.h            # Abstract rendering interface
│   ├── EyeRenderer.cpp
│   ├── OLEDRenderer.h           # OLED-specific renderer (Adafruit GFX)
│   ├── OLEDRenderer.cpp
│   └── DisplayConfig.h          # Display configuration structs
├── examples/
│   ├── BasicEmotion/
│   │   └── BasicEmotion.ino     # Simple emotion demo
│   ├── EyePosition/
│   │   └── EyePosition.ino      # Eye movement demo
│   └── SerialControl/
│       └── SerialControl.ino    # AI serial control demo
├── keywords.txt                 # Arduino IDE syntax highlighting
├── library.properties           # Arduino library metadata
└── README.md                    # Library documentation

# PC Emulator (Python)
emulator/
├── botieyes_emulator.py         # Main emulator entry point
├── emotion_mapper.py            # Port of C++ EmotionMapper
├── eye_renderer.py              # Pygame-based renderer
├── ui_controls.py               # Sliders for valence/arousal/position
└── requirements.txt             # Python dependencies (pygame)

# Testing
tests/
├── test_emotion_mapper.cpp      # Unit tests for mapping functions
├── test_interpolator.cpp        # Unit tests for easing/interpolation
└── visual_regression/           # Emulator screenshot comparison
    └── golden/                  # Reference images for emotions

# Build/Config
.github/
└── workflows/
    ├── arduino-compile.yml      # Arduino library compilation CI
    └── emulator-tests.yml       # Emulator visual tests CI
```

**Structure Decision**: 
- **Arduino Library**: Standard Arduino library layout (`src/`, `examples/`, `library.properties`) for Arduino IDE compatibility
- **Separation of Concerns**: Clear separation between state management (EmotionState, EyePosition), logic (EmotionMapper, Interpolator), and rendering (EyeRenderer abstraction, OLEDRenderer implementation)
- **Emulator Independence**: Python emulator in separate `emulator/` directory; shares logic conceptually but not code (ports C++ mapping to Python for rapid MVP)
- **Hardware Abstraction**: EyeRenderer abstract interface enables future display types (e.g., TFT, LED matrix) without changing core logic
- **Examples**: Arduino IDE standard - each example in own directory with .ino sketch

## Complexity Tracking

| Violation | Why Needed | Simpler Alternative Rejected Because |
|-----------|------------|-------------------------------------|
| Non-extensible emotion mapping (Principle VII) | v1 scope control; baseline formulas work across use cases; reduces API complexity | Custom mapping API would require: validation logic, runtime safety checks, memory overhead for callbacks/function pointers, complex error handling. Adds ~1-2KB code + testing burden. Users with special needs can fork (open source). |
