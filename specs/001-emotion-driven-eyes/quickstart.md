# BotiEyes Quickstart Guide

**Version**: 1.0.0  
**Target**: Arduino Developers  
**Time to First Expression**: < 15 minutes

## What is BotiEyes?

BotiEyes is a parametric emotion-driven eye animation library for OLED displays. Instead of fixed expressions like "happy" or "sad", you control emotions using continuous **valence** (negative ↔ positive) and **arousal** (calm ↔ excited) coordinates. Perfect for AI-driven robots that need nuanced emotional expression.

**Key Features**:
- ✅ Continuous emotional expressions (not discrete moods)
- ✅ Emotion helper methods for easy use (`happy()`, `sad()`, `angry()`, etc.)
- ✅ Coupled 2D eye position control (look around together)
- ✅ Smooth animations (blink, wink)
- ✅ Minimal dependencies (Adafruit GFX only)
- ✅ Works on Arduino Nano (primary), Mega, ESP32, and PC emulator

---

## Installation

### Arduino IDE

1. **Download BotiEyes library** (ZIP from GitHub releases)
2. **Install**: Sketch → Include Library → Add .ZIP Library
3. **Install dependency**: Library Manager → Search "Adafruit GFX" → Install

### PlatformIO

```ini
[env:nanoatmega328]
platform = atmelavr
board = nanoatmega328
framework = arduino
lib_deps =
    BotiEyes
    adafruit/Adafruit GFX Library
```

---

## Hardware Setup

### Required Components

- **Microcontroller**: Arduino Nano (recommended), Arduino Mega 2560, or ESP32
- **Display**: Monochrome OLED (SSD1306 or SH1106), 128x64 pixels (or 128x32 for tighter Nano memory)
- **Connection**: I2C (SCL/SDA) recommended for Nano (saves pins)

### Wiring (I2C)

| OLED Pin | Arduino Nano | Arduino Mega | ESP32 |
|----------|--------------|--------------|-------|
| VCC | 5V | 5V | 3.3V |
| GND | GND | GND | GND |
| SCL | A5 (SCL) | Pin 21 (SCL) | GPIO 22 (SCL) |
| SDA | A4 (SDA) | Pin 20 (SDA) | GPIO 21 (SDA) |

**I2C Address**: Usually `0x3C` or `0x3D` (check your display documentation)

---

## First Program: Neutral Eyes

Create a new Arduino sketch with this code:

```cpp
#include <BotiEyes.h>

BotiEyes eyes;

void setup() {
    Serial.begin(115200);
    
    // Configure your display
    DisplayConfig config = {
        .type = DISPLAY_SSD1306_128x64,
        .protocol = PROTOCOL_I2C,
        .i2cAddress = 0x3C,  // Change to 0x3D if needed
        .width = 128,
        .height = 64
    };
    
    // Initialize (automatically shows neutral expression)
    ErrorCode result = eyes.initialize(config);
    if (result != OK) {
        Serial.println("Display init failed!");
        while(1);  // Stop if initialization fails
    }
    
    Serial.println("BotiEyes ready!");
}

void loop() {
    eyes.update();  // Render frame
    delay(33);      // ~30 FPS
}
```

**Upload and Run**:
- You should see neutral eyes (relaxed, moderate arousal) on the display
- Eyes will be centered (looking straight ahead)

**Troubleshooting**:
- "Init failed!" → Check I2C wiring and address
- Blank screen → Verify display power (VCC/GND)
- Garbled display → Wrong display type in config

---

## Example 1: Show Different Emotions

```cpp
void setup() {
    // ... (initialization code from above)
    
    // Happy expression using emotion helper
    eyes.happy();  // Simple and intuitive!
}

void loop() {
    eyes.update();
    delay(33);
}
```

**Emotion Helper Methods** (easy to use):

| Method | Description |
|--------|-------------|
| `happy()` | Wide eyes, "smiling" brows |
| `sad()` | Droopy eyelids, downturned brows |
| `angry()` | Narrow eyes, intense gaze |
| `calm()` | Relaxed, half-open |
| `excited()` | Very wide eyes, energetic |
| `tired()` | Heavy eyelids, low energy |
| `surprised()` | Very wide eyes, raised brows |
| `anxious()` | Tense, high arousal |
| `content()` | Peaceful, satisfied |
| `curious()` | Attentive, interested |

**Advanced**: Use `setEmotion(valence, arousal, duration)` for custom emotions:
```cpp
eyes.setEmotion(0.42, 0.73, 600);  // Custom emotion for AI integration
```

---

## Example 2: Cycle Through Emotions

```cpp
void loop() {
    static unsigned long lastChange = 0;
    static int emotionIndex = 0;
    
    // Change emotion every 3 seconds
    if (millis() - lastChange > 3000) {
        switch(emotionIndex) {
            case 0: eyes.happy(); break;
            case 1: eyes.sad(); break;
            case 2: eyes.angry(); break;
            case 3: eyes.calm(); break;
            case 4: eyes.surprised(); break;
        }
        emotionIndex = (emotionIndex + 1) % 5;
        lastChange = millis();
    }
    
    eyes.update();
    delay(33);
}
```

---

## Example 3: Eye Position Control

```cpp
void loop() {
    static unsigned long lastMove = 0;
    static int step = 0;
    
    // Move eyes every 2 seconds (both eyes move together)
    if (millis() - lastMove > 2000) {
        switch(step) {
            case 0: eyes.neutral(); break;      // Center
            case 1: eyes.lookLeft(); break;     // Look left
            case 2: eyes.lookRight(); break;    // Look right
            case 3: eyes.lookUp(); break;       // Look up
            case 4: eyes.lookDown(); break;     // Look down
        }
        step = (step + 1) % 5;
        lastMove = millis();
    }
    
    eyes.update();
    delay(33);
}
```

**Position Methods** (coupled control - both eyes move together):
- `neutral()` - Center gaze (0°, 0°)
- `lookLeft()` - Both eyes left
- `lookRight()` - Both eyes right
- `lookUp()` / `lookDown()` - Vertical gaze
- `setEyePosition(h, v, duration)` - Custom position with configurable speed

---

## Example 4: Animations

```cpp
void loop() {
    static unsigned long lastBlink = 0;
    
    // Blink every 5 seconds
    if (millis() - lastBlink > 5000) {
        eyes.blink();  // Quick blink (150ms)
        lastBlink = millis();
    }
    
    eyes.update();
    delay(33);
}
```

**Animation Methods**:
- `blink()` - Both eyes close and reopen (default 150ms)
- `blink(duration)` - Slow blink (custom duration)
- `wink(EYE_LEFT)` / `wink(EYE_RIGHT)` - One eye wink

**Note**: `roll()` animation removed in v1 for memory efficiency. Use emotion + position combinations instead.

---

## Example 5: AI Serial Control

**Note**: Serial protocol is **not built into the library**. See `examples/SerialControl/SerialControl.ino` for a complete reference implementation.

**Example Implementation**:
```cpp
void setup() {
    Serial.begin(115200);
    // ... (initialization code)
}

void loop() {
    // Check for serial commands
    if (Serial.available()) {
        String command = Serial.readStringUntil('\n');
        command.trim();
        
        // Emotion helpers (simplified)
        if (command == "HAPPY") eyes.happy();
        else if (command == "SAD") eyes.sad();
        else if (command == "ANGRY") eyes.angry();
        
        // Or parametric control
        else if (command.startsWith("EMO:")) {
            int comma = command.indexOf(',');
            float valence = command.substring(4, comma).toFloat();
            float arousal = command.substring(comma + 1).toFloat();
            eyes.setEmotion(valence, arousal);
        }
        
        Serial.println("OK");
    }
    
    eyes.update();
    delay(33);
}
```

**Test Commands** (via Serial Monitor):
```
HAPPY            → Show happy emotion
SAD              → Show sad emotion
EMO:0.3,0.8      → Custom emotion (parametric)
```

**PC Emulator**: JSON export available in Python emulator for AI analysis (not on Arduino for memory reasons).

---

## PC Emulator (Python)

For development without hardware:

```bash
cd emulator
pip install pygame
python botieyes_emulator.py
```

**Controls**:
- **Valence Slider**: Drag to change emotion polarity
- **Arousal Slider**: Drag to change energy level
- **Export PNG**: Click to save frame for AI analysis

**Use Case**: Develop and test expressions before uploading to Arduino.

---

## Performance Tips

### Arduino Nano (15-20 FPS Target) - PRIMARY PLATFORM

**Memory Budget**: Library uses ~1.04KB, leaving **~1000 bytes for user code** ✅ (viable for dedicated eye controller)

1. **Enable I2C Fast Mode** (400kHz) - **CRITICAL**:
   ```cpp
   Wire.setClock(400000);  // Before eyes.initialize()
   ```

2. **Minimize Memory Usage**:
   ```cpp
   // BAD: Uses RAM
   String message = "Hello";
   
   // GOOD: Uses Flash (PROGMEM)
   const char message[] PROGMEM = "Hello";
   ```

3. **Adjust Frame Rate**:
   ```cpp
   delay(50);  // 20 FPS (achievable with I2C fast mode)
   ```

4. **Monitor Memory** (recommended on Nano):
   ```cpp
   extern int __heap_start, *__brkval;
   int freeMemory() {
       int v;
       return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
   }
   Serial.print("Free RAM: ");
   Serial.println(freeMemory());  // Should be >100 bytes minimum
   ```

5. **Consider Smaller Display** (saves 512 bytes):
   ```cpp
   // 128x32 instead of 128x64 → ~1500 bytes user code
   .type = DISPLAY_SSD1306_128x32,
   .width = 128,
   .height = 32
   ```

### Arduino Mega (20-25 FPS Target)

1. **Enable I2C Fast Mode** (400kHz):
   ```cpp
   Wire.setClock(400000);  // Before eyes.initialize()
   ```

2. **Adjust Frame Rate**:
   ```cpp
   delay(40);  // 25 FPS (slower but smoother on Mega)
   ```

### ESP32 (30-60 FPS)

```cpp
delay(16);  // 60 FPS (ESP32 easily handles this)
```

---

## Common Issues

### Blank Screen
- **Check wiring**: Verify VCC, GND, SCL, SDA
- **Check I2C address**: Try `0x3C` and `0x3D`
- **Check display type**: Verify SSD1306 vs SH1106 in config

### Slow/Choppy Animation
- **Nano/Mega**: Enable I2C fast mode (`Wire.setClock(400000)`) - **CRITICAL**
- **Frame rate**: Ensure `delay()` matches target FPS
- **Memory**: Check if RAM is running low (especially Nano)

### Random Crashes or Glitches (Nano)
- **Out of memory**: Library uses ~1.04KB, ~1000 bytes available for user code (after simplifications)
- **Solutions**:
  - Move strings to PROGMEM: `const char[] PROGMEM`
  - Reduce global variables
  - Avoid String class (use char arrays)
  - Use 128x32 display (saves 512 bytes → ~1500 bytes user code)
  - Upgrade to Mega (8KB RAM → ~7KB user code)

### Eyes Look Wrong
- **Calibration**: Emotion values are subjective; adjust to taste
- **Resolution**: 64x48 displays may need larger primitive sizes

---

## Next Steps

1. **Explore Emotion Space**: Try intermediate valence/arousal values
2. **Combine Features**: Mix emotions with eye positions (e.g., "sad while looking down")
3. **Add Randomness**: Periodic blinks, slight gaze adjustments
4. **AI Integration**: Connect to ChatGPT/LLM for context-aware emotions
5. **Read Full API**: See [contracts/BotiEyes-API.md](contracts/BotiEyes-API.md)

---

## Resources

- **Examples**: `File → Examples → BotiEyes` in Arduino IDE
- **API Reference**: [contracts/BotiEyes-API.md](contracts/BotiEyes-API.md)
- **Data Model**: [data-model.md](data-model.md)
- **GitHub**: [botieyes repository](https://github.com/yourusername/botieyes)
- **Discord**: [Community support](https://discord.gg/botieyes)

---

## Minimal Working Example (Copy-Paste)

```cpp
#include <BotiEyes.h>

BotiEyes eyes;

void setup() {
    DisplayConfig config = {
        .type = DISPLAY_SSD1306_128x64,
        .protocol = PROTOCOL_I2C,
        .i2cAddress = 0x3C,
        .width = 128,
        .height = 64
    };
    eyes.initialize(config);
    eyes.setEmotion(0.3, 0.6);  // Happy
}

void loop() {
    eyes.update();
    delay(33);  // 30 FPS
}
```

**That's it!** You now have emotional eyes on your robot. 🤖👀

---

**Questions?** Open an issue on GitHub or ask in Discord!
