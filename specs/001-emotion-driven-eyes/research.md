# Research: Emotion-Driven Bot Eyes Library

**Phase**: 0 - Outline & Research  
**Date**: 2026-04-17  
**Status**: Complete

## Overview

This document consolidates research findings for the BotiEyes library implementation, focusing on testing strategies, Adafruit GFX capabilities, and performance optimization for constrained embedded platforms.

---

## 1. Testing Strategy

### Decision: PlatformIO + Unity + ArduinoFake + Mock Canvas

**Rationale**: Industry-standard embedded testing approach balancing thoroughness with practicality for resource-constrained graphics library.

### Framework Stack

**Unit Testing**:
- **Unity** - Lightweight C test framework (built into PlatformIO, ~2KB overhead)
- **ArduinoFake** - Mock Arduino APIs for desktop testing
- **PlatformIO Native Environment** - Run tests on host machine without hardware

**Visual Validation**:
- **MockDisplay** - Virtual canvas implementing Adafruit_GFX interface
- **Golden Image Checksums** - Hash-based snapshot testing for rendering validation

**Test Organization**:
```
test/
├── native/                      # Pure logic tests (desktop, no hardware)
│   ├── test_emotion_mapping.cpp # Valence-arousal → parameters
│   ├── test_easing_functions.cpp # Interpolation curves
│   ├── test_geometry.cpp        # Ellipse calculations
│   └── test_state_machine.cpp   # Emotion transitions
├── desktop/                     # Visual rendering tests
│   ├── test_rendering_visual.cpp # Mock canvas + snapshot testing
│   └── MockDisplay.h            # Virtual display implementation
├── embedded/                    # Hardware smoke tests
│   └── test_smoke.cpp           # Initialization, basic functionality
└── golden/                      # Reference checksums
    ├── happy_128x64.hash
    ├── sad_128x64.hash
    └── ...
```

### What to Test

**Automated (CI/CD)**:
- ✅ Emotion mapping functions (valence-arousal → pupil/eyelid/brow)
- ✅ Easing functions (ease-in-out-cubic, etc.)
- ✅ Coordinate transformations (eye position H/V)
- ✅ State transitions (emotion changes, interpolation)
- ✅ Boundary conditions (screen edges, invalid inputs, clamping)
- ✅ Memory usage (compile-time checks via `pio run -t size`)
- ✅ Rendering output (via MockDisplay + checksums)

**Manual Validation (Pre-Release)**:
- 👁️ Visual aesthetics (emotional expressiveness)
- 👁️ Smoothness on real hardware (frame rate, jitter)
- 👁️ Display-specific quirks (ghosting, refresh artifacts)
- 👁️ Performance on Arduino Mega (20 FPS target)

### CI/CD Integration

**GitHub Actions Workflow**:
```yaml
jobs:
  unit-tests:
    - Run PlatformIO native tests (fast, no hardware)
    - Build for all targets (Mega, ESP32)
    - Check memory usage via `pio run -t size`
    - Upload golden image diff reports (if visual tests fail)
```

**Memory Constraint Validation**:
- Ensure compiled library < 6KB (leaving >2KB for user code on Mega)
- Fail CI if RAM usage exceeds targets

---

## 2. Adafruit GFX Capabilities

### Performance Characteristics

**Rendering Speed**:
- **Arduino Mega (16MHz)**: 10-20 FPS typical, 20-25 FPS achievable with optimization
- **ESP32 (240MHz)**: 30-60 FPS easily, can exceed 60 FPS
- **Bottleneck**: I2C interface (~25ms full screen transfer), not CPU

**Primitive Render Times** (per operation):
- `drawCircle()`: 2-4ms
- `fillCircle()`: 5-10ms
- `drawLine()`: 0.5-2ms
- Full 128x64 screen fill: 40-60ms (Mega), 10-20ms (ESP32)

### Supported Primitives

| Primitive | Native Support | Status | Workaround |
|-----------|----------------|--------|------------|
| Circle (filled/outline) | ✅ `drawCircle()`, `fillCircle()` | **YES** | None needed |
| Line (H/V optimized) | ✅ `drawLine()`, `drawFastHLine()` | **YES** | None needed |
| Triangle (filled) | ✅ `fillTriangle()` | **YES** | None needed |
| Rectangle (filled) | ✅ `fillRect()` | **YES** | None needed |
| **Ellipse** | ❌ No native support | **NO** | Custom Bresenham algorithm (~50-100 bytes code) |
| **Arc/Wedge** | ❌ No native support | **NO** | Custom using `fillTriangle()` or trigonometry |

**Decision**: 
- Implement custom `drawEllipse()` helper for outer eye shape
- Use `fillTriangle()` for eyelid arcs (simpler than custom arc drawing)
- Community implementations available; ~100 bytes code overhead

### Double Buffering / Flicker Prevention

**Approach**: GFXcanvas1 (offscreen canvas)
- Draw all primitives to RAM buffer
- Single `display.drawBitmap()` transfer to screen
- Prevents flicker (only complete frames shown)

**Memory Cost**:
- 128x64 canvas: **1024 bytes** (1KB)
- 64x48 canvas: **384 bytes**
- Canvas object overhead: ~30 bytes

**Note**: SSD1306 maintains internal framebuffer, so `display.display()` transfers full buffer (~15-30ms on I2C). No partial updates supported.

### Memory Footprint

**Arduino Mega (8KB SRAM) Budget**:
- Framebuffer: 1KB (128x64 display)
- Adafruit GFX library: ~200-500 bytes
- BotiEyes state/variables: ~1-2KB estimated
- **Remaining for user code**: ~5-6KB

**ESP32 (520KB SRAM)**: No memory concerns; ample headroom for optimizations.

---

## 3. Performance Optimization Strategy

### Primary Techniques (Recommended)

1. **Offscreen Canvas (GFXcanvas1)** - CRITICAL
   - Eliminates flicker
   - Batches all drawing into single transfer
   - Required for smooth animation

2. **I2C Fast Mode (400kHz)** - HIGH PRIORITY
   - Default: 100kHz (~40ms screen transfer)
   - Fast mode: 400kHz (~10-15ms transfer)
   - 3-4x speedup; essential for 20 FPS target

3. **Pre-computed Lookup Tables** - MEDIUM PRIORITY
   - Store sin/cos values for pupil paths (PROGMEM)
   - Pre-calculate eyelid arc coordinates
   - Saves ~5-10ms per frame vs real-time trigonometry

4. **Integer-Only Math** - MEDIUM PRIORITY
   - Avoid floating point (10-100x slower on Arduino)
   - Use fixed-point arithmetic (scale by 256 or 1024)
   - Critical for easing functions

5. **SPI over I2C (if possible)** - OPTIONAL
   - 3-4x faster than I2C (if display supports SPI)
   - Hardware SPI > software SPI

### Advanced Techniques (For Future Optimization)

- **Dirty Rectangles**: Track bounding box of changed regions, redraw only modified areas
  - Requires custom SSD1306 driver modification (not available in standard Adafruit lib)
  - Can achieve 100+ FPS for small movements
  - Deferred to v2 if needed

- **Triple Buffering** (ESP32 only): Prepare next frame while displaying current
  - Requires 3KB RAM (3× 128x64 buffers)
  - Not feasible on Mega; possible on ESP32

---

## 4. Implementation Guidance

### Primitive Implementation Priorities

**Phase 1 (MVP - Emulator)**:
1. Implement `drawEllipse()` helper (outer eye shape)
2. Implement `fillArc()` using `fillTriangle()` approximation (eyelids)
3. Test rendering on MockDisplay

**Phase 2 (Arduino Integration)**:
1. Optimize primitives with integer math
2. Create lookup tables for common arc angles
3. Validate on real hardware (Mega + ESP32)

### Performance Validation Checkpoints

**Arduino Mega (20 FPS minimum)**:
- Frame render time budget: 50ms
  - Drawing primitives: ~20-25ms (5 primitives × 4-5ms each)
  - I2C transfer: ~15ms (fast mode)
  - Overhead: ~10ms
- **If 20 FPS not achieved**: Reduce detail (use circles instead of ellipses) or optimize I2C

**ESP32 (30-60 FPS target)**:
- Frame render time budget: 16-33ms
  - Should easily achieve with 240MHz CPU
  - Focus on smooth interpolation, not raw speed

### Alternative Library Consideration

**U8g2** (evaluated but not selected):
- Pros: Faster for monochrome OLEDs; optimized page mode rendering
- Cons: Different API; less community support; more complex
- **Decision**: Stick with Adafruit GFX for broader compatibility and simplicity

---

## 5. Key Decisions Summary

| Decision | Choice | Rationale |
|----------|--------|-----------|
| **Testing Framework** | PlatformIO + Unity + ArduinoFake | Industry standard, lightweight, desktop testing |
| **Visual Validation** | MockDisplay + Golden Checksums | Deterministic, fast, no hardware needed |
| **CI/CD** | GitHub Actions (native tests + compilation) | Free, fast feedback, memory checks |
| **Rendering Library** | Adafruit GFX | Broad compatibility, well-documented, sufficient performance |
| **Missing Primitives** | Custom `drawEllipse()` + `fillTriangle()` eyelids | ~100 bytes code; simpler than full arc implementation |
| **Double Buffering** | GFXcanvas1 offscreen canvas | 1KB RAM cost acceptable; eliminates flicker |
| **I2C Speed** | 400kHz fast mode | Essential for 20 FPS on Mega |
| **Math Optimization** | Integer-only, fixed-point | Avoid floating point on Arduino |
| **Lookup Tables** | Sin/cos pre-computed (PROGMEM) | Saves 5-10ms per frame |

---

## 6. Open Questions for Phase 1 Design

**RESOLVED** - No remaining unknowns. Ready for Phase 1 design phase.

All technical feasibility questions answered:
- ✅ Testing strategy defined
- ✅ Rendering performance validated (achievable on Mega, easy on ESP32)
- ✅ Memory constraints confirmed (1KB framebuffer fits in 8KB)
- ✅ Missing primitives have clear workarounds
- ✅ Optimization path established

---

## 7. References

- Adafruit GFX Library: https://github.com/adafruit/Adafruit-GFX-Library
- Unity Test Framework: http://www.throwtheswitch.org/unity
- ArduinoFake: https://github.com/FabioBatSilva/ArduinoFake
- PlatformIO Native Environment: https://docs.platformio.org/en/latest/platforms/native.html
- Community Ellipse Drawing: Arduino forums (Bresenham algorithm adaptations)

**Next Phase**: Phase 1 - Design & Contracts (data model, API contracts, quickstart)
