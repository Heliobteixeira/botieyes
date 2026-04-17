# BotiEyes Quickstart Guide

**Version**: 1.0.0  
**Target**: Arduino Developers  
**Time to First Expression**: < 15 minutes

## What is BotiEyes?

BotiEyes is a parametric emotion-driven eye animation library for OLED displays. Instead of fixed expressions like "happy" or "sad", you control emotions using continuous **valence** (negative ↔ positive) and **arousal** (calm ↔ excited) coordinates. Perfect for AI-driven robots that need nuanced emotional expression.

**Key Features**:
- ✅ Continuous emotional expressions (not discrete moods)
- ✅ Independent 2D eye position control (converge, diverge, look around)
- ✅ Smooth animations (blink, wink, roll)
- ✅ Minimal dependencies (Adafruit GFX only)
- ✅ Works on Arduino Mega, ESP32, and PC emulator

---

## Installation

### Arduino IDE

1. **Download BotiEyes library** (ZIP from GitHub releases)
2. **Install**: Sketch → Include Library → Add .ZIP Library
3. **Install dependency**: Library Manager → Search "Adafruit GFX" → Install

### PlatformIO

```ini
[env:mega2560]
platform = atmelavr
board = megaatmega2560
framework = arduino
lib_deps =
    BotiEyes
    adafruit/Adafruit GFX Library
```

---

## Hardware Setup

### Required Components

- **Microcontroller**: Arduino Mega 2560 or ESP32
- **Display**: Monochrome OLED (SSD1306 or SH1106), 128x64 pixels recommended
- **Connection**: I2C (SCL/SDA) or SPI

### Wiring (I2C)

| OLED Pin | Arduino Mega | ESP32 |
|----------|--------------|-------|
| VCC | 5V | 3.3V |
| GND | GND | GND |
| SCL | Pin 21 (SCL) | GPIO 22 (SCL) |
| SDA | Pin 20 (SDA) | GPIO 21 (SDA) |

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
    
    // Happy expression (positive valence, moderate arousal)
    eyes.setEmotion(0.3, 0.6, 1000);  // 1 second transition
}

void loop() {
    eyes.update();
    delay(33);
}
```

**Emotion Values Guide**:

| Emotion | Valence | Arousal | Description |
|---------|---------|---------|-------------|
| Happy | +0.35 | 0.55 | Wide eyes, "smiling" brows |
| Sad | -0.35 | 0.35 | Droopy eyelids, downturned brows |
| Angry | -0.30 | 0.80 | Narrow eyes, intense gaze |
| Calm | +0.20 | 0.25 | Relaxed, half-open |
| Excited | +0.30 | 0.90 | Very wide eyes, energetic |
| Tired | +0.05 | 0.10 | Heavy eyelids, low energy |

**Tip**: Values between these create intermediate expressions!

---

## Example 2: Cycle Through Emotions

```cpp
void loop() {
    static unsigned long lastChange = 0;
    static int emotionIndex = 0;
    
    // Change emotion every 3 seconds
    if (millis() - lastChange > 3000) {
        switch(emotionIndex) {
            case 0: eyes.setEmotion(0.3, 0.6); break;    // Happy
            case 1: eyes.setEmotion(-0.35, 0.35); break; // Sad
            case 2: eyes.setEmotion(-0.3, 0.8); break;   // Angry
            case 3: eyes.setEmotion(0.2, 0.25); break;   // Calm
        }
        emotionIndex = (emotionIndex + 1) % 4;
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
    
    // Move eyes every 2 seconds
    if (millis() - lastMove > 2000) {
        switch(step) {
            case 0: eyes.neutral(); break;      // Center
            case 1: eyes.lookLeft(); break;     // Look left
            case 2: eyes.lookRight(); break;    // Look right
            case 3: eyes.lookUp(); break;       // Look up
            case 4: eyes.converge(); break;     // Cross-eyed
        }
        step = (step + 1) % 5;
        lastMove = millis();
    }
    
    eyes.update();
    delay(33);
}
```

**Position Methods**:
- `neutral()` - Center gaze (0°, 0°)
- `lookLeft()` - Both eyes left
- `lookRight()` - Both eyes right
- `lookUp()` / `lookDown()` - Vertical gaze
- `converge()` - Eyes look inward
- `diverge()` - Eyes look outward

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
- `blink()` - Both eyes close and reopen
- `blink(duration)` - Slow blink
- `wink(EYE_LEFT)` / `wink(EYE_RIGHT)` - One eye
- `roll(ROLL_CW)` - Circular eye roll

---

## Example 5: AI Serial Control

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
        
        if (command.startsWith("EMO:")) {
            // Parse "EMO:valence,arousal"
            int comma = command.indexOf(',');
            float valence = command.substring(4, comma).toFloat();
            float arousal = command.substring(comma + 1).toFloat();
            
            ErrorCode result = eyes.setEmotion(valence, arousal);
            Serial.println(result == OK ? "OK" : "ERROR");
        }
        else if (command == "QUERY") {
            // Send current state as JSON
            Serial.println(eyes.getExpressionState());
        }
    }
    
    eyes.update();
    delay(33);
}
```

**Test Commands** (via Serial Monitor):
```
EMO:0.3,0.8      → Happy and excited
EMO:-0.35,0.35   → Sad and calm
QUERY            → Get JSON state
```

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

### Arduino Mega (20-25 FPS Target)

1. **Enable I2C Fast Mode** (400kHz):
   ```cpp
   Wire.setClock(400000);  // Before eyes.initialize()
   ```

2. **Adjust Frame Rate**:
   ```cpp
   delay(40);  // 25 FPS (slower but smoother on Mega)
   ```

3. **Monitor Memory**:
   ```cpp
   Serial.print("Free RAM: ");
   Serial.println(freeMemory());
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
- **Mega**: Enable I2C fast mode (`Wire.setClock(400000)`)
- **Frame rate**: Ensure `delay()` matches target FPS
- **Memory**: Check if RAM is running low

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
