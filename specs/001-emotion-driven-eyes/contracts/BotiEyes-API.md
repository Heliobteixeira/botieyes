# BotiEyes Library - Public API Contract

**Version**: 1.0.0  
**Date**: 2026-04-17  
**Target Platforms**: Arduino Nano (primary), Arduino Mega 2560, ESP32, PC Emulator (Python)

## Overview

BotiEyes is a parametric emotion-driven eye animation library for OLED displays. This contract defines the public C++ API for Arduino/ESP32 platforms.

---

## C++ API (Arduino/ESP32)

### Class: `BotiEyes`

Main library interface. One instance per robot (typically singleton).

```cpp
class BotiEyes {
public:
    // Initialization
    ErrorCode initialize(const DisplayConfig& config);
    
    // Emotion Control (Q12: Per-call duration parameter)
    ErrorCode setEmotion(float valence, float arousal, uint16_t duration_ms = 400);
    ErrorCode getCurrentEmotion(float* valence, float* arousal);
    
    // Eye Position Control (Q15: Fixed 300ms interpolation)
    ErrorCode setEyePosition(int16_t leftH, int16_t leftV, 
                            int16_t rightH, int16_t rightV);
    ErrorCode getEyePosition(int16_t* leftH, int16_t* leftV,
                             int16_t* rightH, int16_t* rightV);
    
    // Predefined Eye Behaviors
    ErrorCode converge();
    ErrorCode diverge();
    ErrorCode lookLeft();
    ErrorCode lookRight();
    ErrorCode lookUp();
    ErrorCode lookDown();
    ErrorCode neutral();
    
    // Animations (Q14: Interrupt immediately)
    ErrorCode blink(uint16_t duration_ms = 150);
    ErrorCode wink(EyeSide eye, uint16_t duration_ms = 150);
    ErrorCode roll(RollDirection direction, uint8_t speed = 128);
    
    // Frame Update (Q13: Must call at target FPS)
    ErrorCode update();
    
    // State Inspection (Q11: No debug output, use JSON)
    String getExpressionState(); // Returns JSON
    
private:
    // Implementation hidden
};
```

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

**Returns**:
- `ErrorCode::OK`: Values written to pointers
- `ErrorCode::INVALID_INPUT`: Null pointers

**Example**:
```cpp
float v, a;
eyes.getCurrentEmotion(&v, &a);
Serial.print("Valence: "); Serial.println(v);
Serial.print("Arousal: "); Serial.println(a);
```

---

### `setEyePosition(int16_t leftH, int16_t leftV, int16_t rightH, int16_t rightV)`

**Purpose**: Set independent 2D eye positions with smooth interpolation.

**Parameters**:
- `leftH`: Left eye horizontal angle, -90° (full left) to +90° (full right)
- `leftV`: Left eye vertical angle, -45° (down) to +45° (up)
- `rightH`: Right eye horizontal angle, -90° to +90°
- `rightV`: Right eye vertical angle, -45° to +45°

**Returns**:
- `ErrorCode::OK`: Position set successfully
- `ErrorCode::INVALID_INPUT`: Angles out of bounds

**Behavior** (Q10, Q15 clarifications):
- Eyes smoothly interpolate to new position over **fixed 300ms duration**
- **Independent from emotion** - both apply simultaneously
- Uses ease-in-out-cubic easing

**Example**:
```cpp
// Eyes converge (look inward)
eyes.setEyePosition(-30, 0, 30, 0);

// Left eye looks left-up, right eye looks right-down
eyes.setEyePosition(-45, 20, 45, -20);

// Both eyes look up
eyes.setEyePosition(0, 30, 0, 30);
```

**Constraints**:
- Horizontal angles clamped to [-90, 90]
- Vertical angles clamped to [-45, 45]
- Interpolation duration is fixed (300ms, not configurable per Q15)

---

### `getEyePosition(int16_t* leftH, int16_t* leftV, int16_t* rightH, int16_t* rightV)`

**Purpose**: Query current eye positions (mid-interpolation if transitioning).

**Parameters**: Output pointers for current angles

**Returns**:
- `ErrorCode::OK`: Values written to pointers
- `ErrorCode::INVALID_INPUT`: Null pointers

**Example**:
```cpp
int16_t lh, lv, rh, rv;
eyes.getEyePosition(&lh, &lv, &rh, &rv);
Serial.print("Left: ("); Serial.print(lh); Serial.print(", "); Serial.print(lv); Serial.println(")");
```

---

### Predefined Eye Behaviors

Convenience methods for common gaze patterns.

```cpp
ErrorCode converge();   // Both eyes look inward (cross-eyed)
ErrorCode diverge();    // Both eyes look outward (wall-eyed)
ErrorCode lookLeft();   // Both eyes → (-45°, 0°)
ErrorCode lookRight();  // Both eyes → (+45°, 0°)
ErrorCode lookUp();     // Both eyes → (0°, +30°)
ErrorCode lookDown();   // Both eyes → (0°, -30°)
ErrorCode neutral();    // Both eyes → (0°, 0°)
```

All use 300ms interpolation (same as `setEyePosition()`).

**Example**:
```cpp
eyes.neutral();    // Center gaze
delay(1000);
eyes.lookLeft();   // Look left
delay(1000);
eyes.converge();   // Cross eyes
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

---

### `roll(RollDirection direction, uint8_t speed = 128)`

**Purpose**: Roll eyes in circular motion.

**Parameters**:
- `direction`: `ROLL_CW` (clockwise) or `ROLL_CCW` (counter-clockwise)
- `speed`: 0 (very slow) to 255 (very fast), default 128

**Returns**:
- `ErrorCode::OK`: Animation started

**Behavior**:
- Eyes trace circular path
- Speed determines angular velocity
- Animation continues until interrupted

**Example**:
```cpp
eyes.roll(ROLL_CW, 200);     // Fast clockwise roll
delay(1000);
eyes.neutral();              // Stop rolling
```

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

### `getExpressionState()`

**Purpose**: Serialize current state to JSON for debugging/AI feedback.

**Returns**: JSON string with complete state

**Example Output**:
```json
{
  "emotion": {
    "valence": 0.3,
    "arousal": 0.6
  },
  "position": {
    "leftH": -30,
    "leftV": 0,
    "rightH": 30,
    "rightV": 0
  },
  "expression": {
    "pupilDilation": 0.68,
    "eyelidOpenness": 0.76,
    "browAngle": 0.6
  },
  "animation": {
    "active": false,
    "type": "none"
  }
}
```

**Example**:
```cpp
String state = eyes.getExpressionState();
Serial.println(state);  // Send to AI for analysis
```

---

## Serial Protocol

**Purpose**: Enable AI control over serial connection (115200 baud).

### Command Format

**Emotion Control**:
```
EMO:valence,arousal[,duration]
```
Example: `EMO:0.3,0.6` or `EMO:0.3,0.6,500`

**Position Control**:
```
POS:leftH,leftV,rightH,rightV
```
Example: `POS:-30,0,30,0`

**Query State**:
```
QUERY
```
Response: JSON from `getExpressionState()`

**Error Handling** (Q9 clarification):
Invalid commands return:
```
ERROR:INVALID_COMMAND
ERROR:PARSE_FAILED
ERROR:OUT_OF_BOUNDS
```

**Example Serial Session**:
```
>> EMO:0.3,0.8
<< OK

>> POS:0,30,0,30
<< OK

>> QUERY
<< {"emotion":{"valence":0.3,"arousal":0.8},...}

>> EMO:invalid
<< ERROR:PARSE_FAILED
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

enum RollDirection {
    ROLL_CW,    // Clockwise
    ROLL_CCW    // Counter-clockwise
};

enum DisplayType {
    DISPLAY_SSD1306_128x64,
    DISPLAY_SSD1306_128x32,
    DISPLAY_SH1106_128x64,
    DISPLAY_SSD1306_64x48
};

enum DisplayProtocol {
    PROTOCOL_I2C,
    PROTOCOL_SPI_HW,
    PROTOCOL_SPI_SW
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
- **Arduino Nano**: Library uses ~1.6KB RAM (including 1KB framebuffer) → **~400 bytes left for user code** ⚠️ **CRITICAL**
  - **Recommendation**: Minimize global variables, use `const` and `PROGMEM` for strings/tables, consider 128x32 display
- Arduino Mega: Library uses ~1.6KB RAM → 6.4KB available for user code
- ESP32: No significant constraints

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
    
    // Initialize (shows neutral expression)
    if (eyes.initialize(config) != OK) {
        Serial.println("Init failed!");
        while(1);
    }
    
    // Set happy emotion
    eyes.setEmotion(0.3, 0.6, 500);
}

void loop() {
    unsigned long frameStart = millis();
    
    // Update and render
    eyes.update();
    
    // Maintain 30 FPS
    unsigned long elapsed = millis() - frameStart;
    if (elapsed < 33) {
        delay(33 - elapsed);
    }
}
```

---

## Version History

- **1.0.0** (2026-04-17): Initial API contract based on 17 clarifications

---

**Related Documents**:
- [data-model.md](../data-model.md) - Internal data structures
- [spec.md](../spec.md) - Feature specification
- [quickstart.md](../quickstart.md) - Getting started guide
