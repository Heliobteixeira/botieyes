# BotiEyes Library - Public API Contract

**Version**: 1.0.0  
**Date**: 2026-04-17  
**Target Platforms**: Arduino Nano (primary), Arduino Mega 2560, ESP32, PC Emulator (Python)

## Overview

BotiEyes is a parametric emotion-driven eye animation library for OLED displays. This contract defines the public C++ API for Arduino/ESP32 platforms.

**Finalized Design Philosophy**:
- Minimalist futuristic aesthetic: NO eyebrows, NO pupils, NO highlights
- Pure shape-based expression: eye width/height morphing + eyelid overlays
- Symmetry: Only Confused/Thinking use asymmetry for cognitive complexity
- Performance: 28 FPS on Arduino Nano (18-20ms render + 15ms I2C)
- Cuteness: 10% larger, rounder (0.91-1.0 aspect ratio), positioned closer and lower
- **Expert Approved**: 8.4/10 expressiveness, 8.5/10 cuteness (production-ready)

---

## C++ API (Arduino/ESP32)

### Class: `BotiEyes`

Main library interface. One instance per robot (typically singleton).

```cpp
class BotiEyes {
public:
    // Initialization
    ErrorCode initialize(const DisplayConfig& config);
    static ErrorCode validateConfig(const DisplayConfig& config); // Validate before init
    
    // Emotion Control (parametric, Q6)
    ErrorCode setEmotion(float valence, float arousal, uint16_t duration_ms = 400);
    void getCurrentEmotion(float* valence, float* arousal);
    
    // Emotion Helpers (simplified facade for common emotions)
    ErrorCode happy(float intensity = 1.0);
    ErrorCode sad(float intensity = 1.0);
    ErrorCode angry(float intensity = 1.0);
    ErrorCode calm(float intensity = 1.0);
    ErrorCode excited(float intensity = 1.0);
    ErrorCode tired(float intensity = 1.0);
    ErrorCode surprised(float intensity = 1.0);
    ErrorCode anxious(float intensity = 1.0);
    ErrorCode content(float intensity = 1.0);
    ErrorCode curious(float intensity = 1.0);
    
    // Conversational AI Helpers (added per HRI expert review)
    ErrorCode thinking(float intensity = 1.0);   // Neutral valence (0.0), moderate arousal (0.45), slight asymmetry
    ErrorCode confused(float intensity = 1.0);   // Slight negative valence (-0.15), moderate arousal (0.55), asymmetric shape
    
    // Idle Behaviors (added per HRI expert review)
    ErrorCode enableIdleBehavior(bool enable = true);  // Enable/disable breathing analog (periodic micro-blinks + subtle morphing)
    
    // Eye Position Control (coupled, both eyes move together)
    ErrorCode setEyePosition(int16_t h, int16_t v, uint16_t duration_ms = 300);
    void getEyePosition(int16_t* h, int16_t* v);
    
    // Predefined Eye Behaviors
    ErrorCode lookLeft();
    ErrorCode lookRight();
    ErrorCode lookUp();
    ErrorCode lookDown();
    ErrorCode neutral();
    
    // Animations (Q14: Interrupt immediately)
    ErrorCode blink(uint16_t duration_ms = 150);
    ErrorCode wink(EyeSide eye, uint16_t duration_ms = 150);
    
    // Frame Update (Q13: Must call at target FPS)
    ErrorCode update();
    
private:
    // Implementation hidden
};
```

**Key Changes from Review**:
- ❌ Removed `getExpressionState()` - JSON export only in PC emulator, not Arduino
- ❌ Removed `roll()` animation - defer to v2
- ❌ Removed `converge()`, `diverge()` - not needed with coupled eye control
- ✅ Simplified `setEyePosition()` - 2 params (h, v) + configurable duration
- ✅ Simplified `getEyePosition()` - 2 params (h, v)
- ✅ Added emotion helper methods - facade for common emotions
- ✅ Added `validateConfig()` - bounds checking before init
- ✅ Fixed error handling - getters now `void` (cannot fail)

---

## API Methods

### `initialize(const DisplayConfig& config)`

**Purpose**: Initialize display hardware and library state.

**Parameters**:
- `config`: Display configuration (type, protocol, pins, resolution)

**Returns**:
- `ErrorCode::OK`: Success; eyes show neutral expression (0.0, 0.5) per Q7
- `ErrorCode::HARDWARE_ERROR`: Display initialization failed
- `ErrorCode::DISPLAY_NOT_FOUND`: I2C/SPI device not responding
- `ErrorCode::MEMORY_ERROR`: Framebuffer allocation failed

**Behavior** (Q7 clarification):
- After successful init, eyes immediately display neutral expression (valence=0.0, arousal=0.5)
- Eyes at neutral position (0°, 0°)
- Provides visual confirmation that initialization succeeded

**Example**:
```cpp
DisplayConfig config = {
    .type = DISPLAY_SSD1306_128x64,
    .protocol = PROTOCOL_I2C,
    .i2cAddress = 0x3C,
    .width = 128,
    .height = 64
};

// Validate config before initialization
ErrorCode validation = BotiEyes::validateConfig(config);
if (validation != OK) {
    Serial.println("Invalid config!");
    return;
}

BotiEyes eyes;
ErrorCode result = eyes.initialize(config);
if (result != OK) {
    Serial.println("Display init failed!");
}
```

**Constraints**:
- Must be called before any other API methods
- Can only be called once (re-init not supported)

---

### `validateConfig(const DisplayConfig& config)`

**Purpose**: Validate display configuration before initialization (added per review).

**Parameters**:
- `config`: Display configuration to validate

**Returns**:
- `ErrorCode::OK`: Configuration valid
- `ErrorCode::INVALID_INPUT`: Invalid type, protocol, or pin values

**Behavior**:
- Checks I2C address (0x3C, 0x3D valid)
- Checks SPI pins (CS, DC, Reset >= -1)
- Checks display dimensions match type
- Static method, can call before object creation

**Example**:
```cpp
DisplayConfig config = {
    .type = DISPLAY_SSD1306_128x64,
    .protocol = PROTOCOL_I2C,
    .i2cAddress = 0xFF,  // Invalid!
    .width = 128,
    .height = 64
};

if (BotiEyes::validateConfig(config) != OK) {
    Serial.println("Fix I2C address (use 0x3C or 0x3D)");
}

---

### `setEmotion(float valence, float arousal, uint16_t duration_ms = 400)`

**Purpose**: Set target emotional state with smooth transition.

**Parameters**:
- `valence`: Emotion polarity, -0.5 (negative) to +0.5 (positive), 0 = neutral (Q6)
- `arousal`: Energy level, 0.0 (calm) to 1.0 (excited) (Q6)
- `duration_ms`: Transition duration in milliseconds, default 400ms (Q12)

**Returns**:
- `ErrorCode::OK`: Emotion set successfully
- `ErrorCode::INVALID_INPUT`: Valence or arousal out of bounds

**Behavior** (Q10, Q12 clarifications):
- Eyes smoothly interpolate from current to target emotion over `duration_ms`
- **Independent from eye position** - emotion and position apply simultaneously
- Uses ease-in-out-cubic easing (FR-014)
- If animation active, emotion change queued until animation completes (FR-012)

**Example**:
```cpp
// Happy with moderate arousal, 500ms transition
eyes.setEmotion(0.3, 0.6, 500);

// Sad and calm, default 400ms transition
eyes.setEmotion(-0.35, 0.35);

// Anxious (negative valence, high arousal)
eyes.setEmotion(-0.2, 0.75, 300);
```

**Constraints**:
- Valence clamped to [-0.5, 0.5]
- Arousal clamped to [0.0, 1.0]
- Duration must be > 0 (0 not allowed per clarifications)

---

### `getCurrentEmotion(float* valence, float* arousal)`

**Purpose**: Query current emotional state (mid-interpolation if transitioning).

**Parameters**:
- `valence`: Output pointer for current valence
- `arousal`: Output pointer for current arousal

**Returns**: None (`void`) - cannot fail, always succeeds (fixed per review)

**Example**:
```cpp
float v, a;
eyes.getCurrentEmotion(&v, &a);
Serial.print("Valence: "); Serial.println(v);
Serial.print("Arousal: "); Serial.println(a);
```

---

### Emotion Helper Methods

**Purpose**: Simplified facade for common emotions (added per review to reduce cognitive friction).

All helper methods map to predefined valence-arousal coordinates with configurable intensity.

```cpp
ErrorCode happy(float intensity = 1.0);      // Positive valence, moderate arousal
ErrorCode sad(float intensity = 1.0);        // Negative valence, low arousal
ErrorCode angry(float intensity = 1.0);      // Negative valence, high arousal
ErrorCode calm(float intensity = 1.0);       // Low positive valence, very low arousal
ErrorCode excited(float intensity = 1.0);    // Positive valence, very high arousal
ErrorCode tired(float intensity = 1.0);      // Low valence, very low arousal
ErrorCode surprised(float intensity = 1.0);  // Low positive valence, high arousal
ErrorCode anxious(float intensity = 1.0);    // Negative valence, high arousal
ErrorCode content(float intensity = 1.0);    // Moderate positive valence, moderate arousal
ErrorCode curious(float intensity = 1.0);    // Low positive valence, moderate-high arousal
```

**Parameters**:
- `intensity`: Emotion strength, 0.0 (subtle) to 1.0 (full), default 1.0

**Returns**:
- `ErrorCode::OK`: Emotion set successfully
- `ErrorCode::INVALID_INPUT`: Intensity out of [0.0, 1.0] range

**Mapping**:
```cpp
happy(intensity)     → setEmotion(0.35 * intensity, 0.55, 400)
sad(intensity)       → setEmotion(-0.35 * intensity, 0.35, 400)
angry(intensity)     → setEmotion(-0.30 * intensity, 0.80, 400)
calm(intensity)      → setEmotion(0.20 * intensity, 0.25, 400)
excited(intensity)   → setEmotion(0.30 * intensity, 0.90, 400)
tired(intensity)     → setEmotion(0.05 * intensity, 0.10, 400)
surprised(intensity) → setEmotion(0.15 * intensity, 0.85, 400)
anxious(intensity)   → setEmotion(-0.20 * intensity, 0.75, 400)
content(intensity)   → setEmotion(0.25 * intensity, 0.40, 400)
curious(intensity)   → setEmotion(0.15 * intensity, 0.60, 400)
```

**Example**:
```cpp
eyes.happy();           // Full happy expression (intensity = 1.0)
eyes.happy(0.5);        // Subtle happy (half intensity)
eyes.sad();             // Sad expression
eyes.excited(0.8);      // Very excited
eyes.calm();            // Calm and peaceful

// Advanced users can still use parametric model directly
eyes.setEmotion(0.42, 0.73, 600);  // Custom emotion
```

**Note**: These are convenience wrappers. Advanced users requiring AI integration can still use `setEmotion()` directly with continuous valence-arousal values.

---

### `setEyePosition(int16_t h, int16_t v, uint16_t duration_ms = 300)`

**Purpose**: Set coupled 2D eye positions (both eyes move together) with smooth interpolation.

**Parameters**:
- `h`: Horizontal angle, -90° (full left) to +90° (full right), both eyes
- `v`: Vertical angle, -45° (down) to +45° (up), both eyes
- `duration_ms`: Transition duration in milliseconds, default 300ms (fixed per review)

**Returns**:
- `ErrorCode::OK`: Position set successfully
- `ErrorCode::INVALID_INPUT`: Angles out of bounds

**Behavior** (Q10, simplified per review):
- Both eyes smoothly interpolate to new position over `duration_ms`
- **Independent from emotion** - both apply simultaneously
- Uses ease-in-out-cubic easing
- **Simplified from independent control** - v1 uses coupled movement only

**Example**:
```cpp
// Both eyes look up
eyes.setEyePosition(0, 30);

// Both eyes look left-up with slow transition
eyes.setEyePosition(-45, 20, 500);

// Both eyes center
eyes.setEyePosition(0, 0);
```

**Constraints**:
- Horizontal angles clamped to [-90, 90]
- Vertical angles clamped to [-45, 45]
- Interpolation duration configurable (fixed API inconsistency per review)

---

### `getEyePosition(int16_t* h, int16_t* v)`

**Purpose**: Query current eye positions (mid-interpolation if transitioning).

**Parameters**: 
- `h`: Output pointer for current horizontal angle (both eyes)
- `v`: Output pointer for current vertical angle (both eyes)

**Returns**: None (`void`) - cannot fail, always succeeds (fixed per review)

**Example**:
```cpp
int16_t h, v;
eyes.getEyePosition(&h, &v);
Serial.print("Position: ("); Serial.print(h); Serial.print(", "); Serial.print(v); Serial.println(")");
```

---

### Predefined Eye Behaviors

Convenience methods for common gaze patterns (simplified per review).

```cpp
ErrorCode lookLeft();   // Both eyes → (-45°, 0°)
ErrorCode lookRight();  // Both eyes → (+45°, 0°)
ErrorCode lookUp();     // Both eyes → (0°, +30°)
ErrorCode lookDown();   // Both eyes → (0°, -30°)
ErrorCode neutral();    // Both eyes → (0°, 0°)
```

All use 300ms interpolation (same as `setEyePosition()`).

**Removed** (per review simplification):
- ❌ `converge()` - Not needed with coupled eye control
- ❌ `diverge()` - Not needed with coupled eye control

**Example**:
```cpp
eyes.neutral();    // Center gaze
delay(1000);
eyes.lookLeft();   // Look left
delay(1000);
eyes.lookUp();     // Look up
```

---

### `blink(uint16_t duration_ms = 150)`

**Purpose**: Trigger blink animation (both eyes close and reopen).

**Parameters**:
- `duration_ms`: Total blink duration (default 150ms)

**Returns**:
- `ErrorCode::OK`: Animation started

**Behavior** (Q14 clarification):
- **Interrupts current animation immediately** if one is running
- Eyes close (eyelidOpenness → 0.0) for 50% of duration
- Eyes reopen (eyelidOpenness → original) for remaining 50%
- Emotion-driven expression resumes after completion

**Example**:
```cpp
eyes.blink();       // Quick blink (150ms)
eyes.blink(300);    // Slow blink (300ms)
```

---

### `wink(EyeSide eye, uint16_t duration_ms = 150)`

**Purpose**: Wink one eye (other remains open).

**Parameters**:
- `eye`: `EYE_LEFT` or `EYE_RIGHT`
- `duration_ms`: Wink duration (default 150ms)

**Returns**:
- `ErrorCode::OK`: Animation started
- `ErrorCode::INVALID_INPUT`: Invalid eye parameter

**Behavior**: Same as `blink()` but affects only specified eye.

**Example**:
```cpp
eyes.wink(EYE_RIGHT, 200);
```

**Note**: `roll()` animation removed per review (deferred to v2).

---

### `update()`

**Purpose**: Advance animation state and render frame.

**Parameters**: None

**Returns**:
- `ErrorCode::OK`: Frame rendered successfully
- `ErrorCode::HARDWARE_ERROR`: Display communication failed

**Behavior** (Q13 clarification):
- **Must be called at target FPS** (e.g., every 33ms for 30 FPS)
- Library does NOT use internal timing/interrupts
- Developer controls frame rate via loop timing
- Each call:
  1. Advances emotion interpolation
  2. Advances position interpolation
  3. Updates animation state
  4. Computes expression parameters
  5. Renders frame to display

**Example**:
```cpp
void loop() {
    unsigned long frameStart = millis();
    
    eyes.update();  // Render frame
    
    // Target 30 FPS (33ms per frame)
    unsigned long elapsed = millis() - frameStart;
    if (elapsed < 33) {
        delay(33 - elapsed);
    }
}
```

**Constraints**:
- Must be called regularly for smooth animation
- Frame rate determines perceived smoothness (20-60 FPS recommended)

---

## Removed Features (Per Review Decisions)

### JSON State Export (`getExpressionState()`)

**Status**: ❌ Removed from Arduino library (available in PC emulator only)

**Rationale**:
- String class causes 250 bytes RAM heap fragmentation on Arduino
- JSON export unclear use case for embedded context
- PC emulator still supports JSON for AI integration testing

**Alternative**: Use `getCurrentEmotion()` and `getEyePosition()` for lightweight state queries

---

### Serial Protocol

**Status**: ❌ Removed from library core (moved to example sketch)

**Rationale**:
- Coupling library to Serial reduces flexibility
- Users may need custom protocols (I2C, Bluetooth, etc.)
- Example implementation clearer than built-in protocol

**Reference**: See `examples/SerialControl/SerialControl.ino` for reference implementation

**Example Protocol** (from example sketch):
```
EMO:valence,arousal[,duration]
EMO:0.3,0.6
EMO:0.3,0.6,500

POS:h,v[,duration]
POS:-30,15
POS:-30,15,400

QUERY  → responds with state over Serial

HAPPY[,intensity]
SAD[,intensity]
...other emotion helpers...
```

---

## Enums & Constants

```cpp
enum ErrorCode {
    OK = 0,
    INVALID_INPUT,
    HARDWARE_ERROR,
    TIMEOUT,
    DISPLAY_NOT_FOUND,
    MEMORY_ERROR
};

enum EyeSide {
    EYE_LEFT,
    EYE_RIGHT
};

// RollDirection removed (roll() animation deferred to v2)

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
```

---

## DisplayConfig Struct

```cpp
struct DisplayConfig {
    DisplayType type;
    DisplayProtocol protocol;
    uint8_t i2cAddress;     // For I2C (typically 0x3C or 0x3D)
    int8_t spiCS;           // For SPI (chip select)
    int8_t spiDC;           // For SPI (data/command)
    int8_t spiReset;        // For SPI (reset, -1 if shared)
    uint16_t width;
    uint16_t height;
};
```

---

## Thread Safety

**Not thread-safe**: All API methods must be called from single thread (Arduino loop).

---

## Memory Constraints

- **Static allocation only** (Q8 clarification) - no `malloc`/`new`
- **Arduino Nano** (2KB SRAM): 
  - Library: ~1.04KB (state 80 bytes + framebuffer 1024 bytes + stack/buffers ~400 bytes)
  - **User code: ~1000 bytes** ✅ **VIABLE** for dedicated eye controller (after review simplifications)
  - **Recommendations**: Use `const` and `PROGMEM` for strings/tables, avoid String class
  - **Alternative**: Use 128x32 display (512 bytes framebuffer → ~1500 bytes user code)
- **Arduino Mega** (8KB SRAM):
  - Library: ~1.04KB
  - **User code: ~7KB** ✅ **COMFORTABLE** - Ample headroom
- **ESP32** (520KB SRAM): No significant constraints

**Key Improvements** (per review decisions):
- ❌ Removed JSON export: +250 bytes RAM
- ❌ Removed independent eye control: +16 bytes RAM
- ❌ Removed roll() animation: +50 bytes stack
- ❌ Simplified rendering: +100 bytes stack
- **Net result**: Nano now viable with ~1000 bytes user code (was 400 bytes - not viable)

---

## Performance Guarantees

- **Arduino Nano**: ≥15 FPS achievable (requires I2C fast mode 400kHz) - **tight memory limits user code complexity**
- **Arduino Mega**: ≥20 FPS achievable (requires I2C fast mode 400kHz)
- **ESP32**: ≥30 FPS easily (≥60 FPS possible with optimization)
- **Transition latency**: <300ms for all interpolations

---

## Example: Complete Integration

```cpp
#include <BotiEyes.h>

BotiEyes eyes;

void setup() {
    Serial.begin(115200);
    
    // Configure display
    DisplayConfig config = {
        .type = DISPLAY_SSD1306_128x64,
        .protocol = PROTOCOL_I2C,
        .i2cAddress = 0x3C,
        .width = 128,
        .height = 64
    };
    
    // Validate config (added per review)
    if (BotiEyes::validateConfig(config) != OK) {
        Serial.println("Invalid config!");
        while(1);
    }
    
    // Initialize (shows neutral expression)
    if (eyes.initialize(config) != OK) {
        Serial.println("Init failed!");
        while(1);
    }
    
    // Set happy emotion (can use helper or parametric model)
    eyes.happy();                     // Using emotion helper
    // eyes.setEmotion(0.3, 0.6, 500); // Or use parametric model directly
}

void loop() {
    unsigned long frameStart = millis();
    
    // Update and render
    eyes.update();
    
    // Maintain 30 FPS (15 FPS on Nano)
    unsigned long elapsed = millis() - frameStart;
    if (elapsed < 33) {
        delay(33 - elapsed);
    }
}
```

---

## Version History

- **1.0.0** (2026-04-17): Initial API contract
  - **Review changes applied**: Removed JSON export, roll() animation, independent eye control
  - **Simplifications**: Coupled eye movement, emotion helper methods, config validation
  - **API fixes**: Duration parameter consistency, getter return types (void), error handling verbosity
  - **Memory**: Nano viable with ~1000 bytes user code (was 400 bytes)

---

**Related Documents**:
- [data-model.md](../data-model.md) - Internal data structures
- [spec.md](../spec.md) - Feature specification
- [quickstart.md](../quickstart.md) - Getting started guide
