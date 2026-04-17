# Data Model: Emotion-Driven Bot Eyes Library

**Phase**: 1 - Design & Contracts  
**Date**: 2026-04-17  
**Status**: Complete

## Overview

This document defines the core data structures and their relationships for the BotiEyes library. The model centers around continuous emotional expression via valence-arousal coordinates, independent 2D eye position, and smooth interpolated transitions.

---

## Core Entities

### 1. EmotionState

**Purpose**: Represents current and target emotional state using valence-arousal model.

**Fields**:
```cpp
struct EmotionState {
    float currentValence;     // Current emotion: -0.5 (negative) to +0.5 (positive)
    float currentArousal;     // Current energy: 0.0 (calm) to 1.0 (excited)
    float targetValence;      // Target emotion for interpolation
    float targetArousal;      // Target energy for interpolation
    uint32_t startTime;       // Interpolation start timestamp (millis())
    uint16_t duration;        // Transition duration in milliseconds
};
```

**Invariants**:
- `currentValence` ∈ [-0.5, 0.5] (clamped)
- `currentArousal` ∈ [0.0, 1.0] (clamped)
- `targetValence` ∈ [-0.5, 0.5] (clamped)
- `targetArousal` ∈ [0.0, 1.0] (clamped)
- `duration` > 0 when interpolating (0 = instant transition)

**State Transitions**:
- `setEmotion(v, a, d)` → sets `targetValence`, `targetArousal`, `duration`, `startTime = millis()`
- `update()` → advances `currentValence`/`currentArousal` toward target using easing function
- Interpolation complete when `millis() - startTime >= duration`

**Memory**: 20 bytes (5 floats + 1 uint32 + 1 uint16)

---

### 2. EyePositionState

**Purpose**: Represents coupled 2D position for both eyes (simplified from independent control per review decisions).

**Fields**:
```cpp
struct EyePositionState {
    // Current position (both eyes move together)
    int16_t horizontal;       // -90° (full left) to +90° (full right)
    int16_t vertical;         // -45° (down) to +45° (up)
    
    // Target positions for interpolation
    int16_t targetH;
    int16_t targetV;
    
    // Interpolation state
    uint32_t startTime;       // Interpolation start (millis())
    uint16_t duration;        // Transition duration (default 300ms, configurable per review decision)
};
```

**Invariants**:
- Horizontal angle ∈ [-90, 90] degrees
- Vertical angle ∈ [-45, 45] degrees
- `duration` ≥ 0 (0 = instant transition, configurable per API fix)

**Predefined Behaviors** (convenience methods):
- `lookLeft()`: Both eyes → (-45°, 0°)
- `lookRight()`: Both eyes → (+45°, 0°)
- `lookUp()`: Both eyes → (0°, +30°)
- `lookDown()`: Both eyes → (0°, -30°)
- `neutral()`: Both eyes → (0°, 0°)

**Memory**: 12 bytes (4 int16 + 1 uint32 + 1 uint16)

---

### 3. ExpressionParameters

**Purpose**: Derived visual parameters computed from EmotionState by EmotionMapper.

**Fields**:
```cpp
struct ExpressionParameters {
    float pupilDilation;      // 0.0 (tiny) to 1.0 (large) - pupil size multiplier
    float eyelidOpenness;     // 0.0 (closed) to 1.0 (fully open)
    float browAngle;          // -1.0 (sad/angry) to +1.0 (happy/surprised) - NOW RENDERED as 6th primitive (eyebrow arc)
    float eyeSquint;          // 0.0 (none) to 1.0 (full squint) - affects eyelid curve; added for anxiety disambiguation
    
    // Computed from EyePositionState (for rendering, coupled for both eyes)
    int16_t pupilOffsetX;     // Horizontal offset from center (pixels, both eyes)
    int16_t pupilOffsetY;     // Vertical offset from center (pixels, both eyes)
};
```

**Derivation** (from EmotionState via EmotionMapper):
```cpp
pupilDilation = 0.3 + (arousal × 0.5) + (max(0, valence) × 0.4)
eyelidOpenness = 0.4 + (arousal × 0.5) + ((valence + 0.5) × 0.2)
browAngle = valence × 2.0  // Range: -1.0 to +1.0
eyeSquint = max(0, (0.5 - arousal) × 0.4) if valence < 0 else 0
```

**Invariants**:
- All float values ∈ [0.0, 1.0] except `browAngle` ∈ [-1.0, 1.0]
- Pupil offsets calculated from eye position angles (see GeometryHelper)

**Memory**: 24 bytes (4 floats + 2 int16) - reduced from 32 bytes via coupled control

---

### 4. AnimationState

**Purpose**: Track active animation (blink, wink) with progress.

**Fields**:
```cpp
enum AnimationType {
    ANIM_NONE = 0,
    ANIM_BLINK,
    ANIM_WINK_LEFT,
    ANIM_WINK_RIGHT
};

struct AnimationState {
    AnimationType type;       // Current animation type
    uint32_t startTime;       // Animation start (millis())
    uint16_t duration;        // Animation duration (configurable)
    float progress;           // 0.0 to 1.0 (normalized time)
    bool active;              // True if animation in progress
};
```

**Behavior** (per Q14 clarification):
- New animation **interrupts current** (does not queue)
- Animation takes precedence over emotion (emotion applies after completion)
- `progress = (millis() - startTime) / duration`, clamped to [0.0, 1.0]
- When `progress >= 1.0`: `active = false`, restore emotion-driven state

**Memory**: 12 bytes (1 enum + 1 uint32 + 1 uint16 + 1 float + 1 bool)

---

### 5. DisplayConfig

**Purpose**: Display hardware configuration (per Q2 clarification - manual setup required).

**Fields**:
```cpp
enum DisplayType {
    DISPLAY_SSD1306_128x64,
    DISPLAY_SSD1306_128x32,
    DISPLAY_SH1106_128x64,
    DISPLAY_SSD1306_64x48
};

enum DisplayProtocol {
    PROTOCOL_I2C,
    PROTOCOL_SPI_HW,      // Hardware SPI
    PROTOCOL_SPI_SW       // Software SPI
};

struct DisplayConfig {
    DisplayType type;
    DisplayProtocol protocol;
    
    // I2C Configuration
    uint8_t i2cAddress;   // Typically 0x3C or 0x3D
    
    // SPI Configuration (if protocol = SPI)
    int8_t spiCS;         // Chip select pin
    int8_t spiDC;         // Data/command pin
    int8_t spiReset;      // Reset pin (-1 if shared)
    
    // Display dimensions
    uint16_t width;       // Pixels (typically 128 or 64)
    uint16_t height;      // Pixels (typically 64, 48, or 32)
};
```

**Usage**:
```cpp
DisplayConfig config = {
    .type = DISPLAY_SSD1306_128x64,
    .protocol = PROTOCOL_I2C,
    .i2cAddress = 0x3C,
    .width = 128,
    .height = 64
};
BotiEyes eyes;
eyes.initialize(config);
```

**Memory**: 12 bytes (2 enums + 4 int8 + 2 uint16)

---

### 6. ErrorCode (Enum)

**Purpose**: Error handling for all API methods (per Q5 clarification).

**Values**:
```cpp
enum ErrorCode {
    OK = 0,                   // Success
    INVALID_INPUT,            // Parameter out of bounds (e.g., valence > 0.5)
    HARDWARE_ERROR,           // Display initialization failed
    TIMEOUT,                  // Operation took too long
    DISPLAY_NOT_FOUND,        // I2C/SPI device not responding
    MEMORY_ERROR              // Static allocation failed (should never happen)
};
```

**Usage**:
- All API methods return `ErrorCode`
- User must check return value: `if (eyes.setEmotion(0.3, 0.6) != OK) { /* handle error */ }`
- Serial interface returns `"ERROR:<code>"` strings for failures

---

## Entity Relationships

```
┌─────────────┐
│ BotiEyes    │ (Main API)
│ (singleton) │
└──────┬──────┘
       │
       ├──► EmotionState (1:1) ─────► EmotionMapper ────► ExpressionParameters
       │                              (transforms)         (derived)
       │
       ├──► EyePositionState (1:1) ──► GeometryHelper ──► Pupil offsets
       │                                (calculates)
       │
       ├──► AnimationState (1:1) ─────► Interpolator ───► Frame-by-frame state
       │                                (easing)
       │
       ├──► DisplayConfig (1:1) ───────► OLEDRenderer ──► Actual display output
       │                                 (Adafruit GFX)
       │
       └──► Update Loop (per frame)
            │
            └─► Interpolator.advance(EmotionState)
            └─► Interpolator.advance(EyePositionState)
            └─► AnimationState.update()
            └─► EmotionMapper.compute(ExpressionParameters)
            └─► OLEDRenderer.draw(ExpressionParameters, EyePositionState)
```

**Key Flows**:

1. **Emotion Change**:
   ```
   setEmotion(v, a, d) → EmotionState.target = (v, a, d)
   → update() → Interpolator.ease() → EmotionState.current
   → EmotionMapper.compute() → ExpressionParameters
   → OLEDRenderer.draw()
   ```

2. **Eye Position Change**:
   ```
   setEyePosition(lh, lv, rh, rv) → EyePositionState.target = (lh, lv, rh, rv)
   → update() → Interpolator.ease(300ms) → EyePositionState.current
   → GeometryHelper.toPupilOffset() → ExpressionParameters.pupilOffset
   → OLEDRenderer.draw()
   ```

3. **Animation**:
   ```
   blink(duration) → AnimationState = (ANIM_BLINK, duration)
   → update() → AnimationState.progress++
   → Override ExpressionParameters.eyelidOpenness
   → When complete: restore emotion-driven eyelidOpenness
   ```

---

## Memory Budget Summary

**Total Core State** (per BotiEyes instance):
- EmotionState: 20 bytes
- EyePositionState: 12 bytes (reduced from 20 - coupled control)
- ExpressionParameters: 24 bytes (reduced from 32 - coupled control)
- AnimationState: 12 bytes
- DisplayConfig: 12 bytes
- **Subtotal**: 80 bytes (down from 96 bytes via simplifications)

**Display Framebuffer** (external, Adafruit GFX):
- 128×64 monochrome: 1024 bytes (1KB)
- 64×48 monochrome: 384 bytes

**Library Code** (estimated):
- BotiEyes core logic: ~2KB
- EmotionMapper + Interpolator: ~1KB
- OLEDRenderer (including custom ellipse/arc): ~1.5KB
- Lookup tables (sin/cos in PROGMEM): ~0.5KB
- **Total code**: ~5KB

**Arduino Nano Budget Check** (2KB SRAM, PRIMARY TARGET):
- Library state: 80 bytes (reduced via coupled eye control, simplified features)
- Framebuffer (128x64): 1024 bytes
- Library stack/heap: ~670 bytes (INCREASED: +40B eyebrow, +80B thinking, +100B idle behaviors, +50B anxiety disambiguation = +270B from baseline)
- Wire/Serial buffers: 134 bytes
- **Used**: ~1.31KB (increased from 1.04KB)
- **Remaining for user**: ~730 bytes ✅ **VIABLE** - Sufficient for dedicated eye controller with design enhancements
- **Alternative**: Use 128x32 display (512 bytes framebuffer → ~1200 bytes user code)
- **Note**: Design expert reviews identified critical enhancements (eyebrows, thinking state, idle behaviors) worth the +270B cost

**Arduino Mega Budget Check** (8KB SRAM):
- Library state: 80 bytes
- Framebuffer: 1024 bytes
- Library stack/heap: ~400 bytes
- Wire/Serial buffers: 134 bytes
- **Used**: ~1.04KB
- **Remaining for user**: ~7KB ✅ **COMFORTABLE** - Ample headroom

---

## State Machine: Emotion Transitions

```
┌─────────────┐
│   IDLE      │ (no animation, emotion stable)
│ (t >= dur)  │
└──────┬──────┘
       │
       │ setEmotion(v, a, d)
       ▼
┌─────────────┐
│INTERPOLATING│ (smooth transition)
│ (t < dur)   │◄─────┐
└──────┬──────┘      │
       │             │ update() (not complete)
       │ update()    │
       │ (t >= dur)  │
       ▼             │
┌─────────────┐      │
│   STABLE    │──────┘
│(current =   │
│  target)    │
└─────────────┘
```

**Concurrent Handling** (Q10 clarification):
- Emotion and Position are **independent**
- Both can interpolate simultaneously
- Both apply to rendering (additive)
- Example: "Happy expression while looking left" = both states active

---

## Validation Rules

**Input Validation** (returns INVALID_INPUT if violated):
- Valence: must be in [-0.5, 0.5]
- Arousal: must be in [0.0, 1.0]
- Eye angles: horizontal [-90, 90], vertical [-45, 45]
- Duration: must be > 0 (0 = instant, no interpolation)

**Hardware Validation** (returns HARDWARE_ERROR if failed):
- Display initialization successful (I2C/SPI responding)
- Framebuffer allocation successful (should never fail with static allocation)

**Runtime Constraints**:
- `update()` MUST be called at target FPS (Q13 clarification)
- No dynamic memory allocation (Q8 clarification - static only)
- All math integer-based or fixed-point (performance requirement)

---

**Next**: [contracts/](contracts/) - Public API contract definition
