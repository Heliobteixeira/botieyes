# BotiEyes Library

[![Arduino](https://img.shields.io/badge/Arduino-Compatible-00979D?logo=arduino)](https://www.arduino.cc/)
[![Platform](https://img.shields.io/badge/Platform-AVR%20%7C%20ESP32-blue)](https://platformio.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**Parametric emotion-driven eye animation library for OLED displays**

BotiEyes enables continuous emotional expression using a valence-arousal model with smooth transitions, 12 emotion presets, coupled 2D eye position control, and cross-platform support. Designed for Arduino Nano (2KB SRAM), Mega, and ESP32 with static memory allocation and minimal dependencies.

## Features

- **Parametric Emotion Control**: Continuous valence-arousal coordinates for nuanced expressions
- **12 Emotion Presets**: happy, sad, angry, calm, excited, tired, surprised, anxious, content, curious, thinking, confused
- **Coupled Eye Position**: 2D gaze direction (both eyes move together) with predefined behaviors
- **Smooth Transitions**: Configurable interpolation with ease-in-out-cubic easing
- **Memory Efficient**: ~1.04KB RAM footprint on Arduino Nano (52% library, 48% user code available)
- **Minimal Dependencies**: Adafruit GFX only
- **Cross-Platform**: Arduino Nano/Mega/ESP32 + PC emulator for development
- **Built-in Animations**: Blink, wink, idle behaviors

## Quick Start

### Installation

1. Install via Arduino Library Manager: Search for "BotiEyes"
2. Or download and extract to your Arduino `libraries/` folder
3. Install dependency: **Adafruit GFX Library**

### Basic Usage

```cpp
#include <BotiEyes.h>
#include <Adafruit_SSD1306.h>

// Initialize display (128x64 OLED on I2C)
Adafruit_SSD1306 display(128, 64, &Wire, -1);
BotiEyes eyes;

void setup() {
  // Configure display
  DisplayConfig config;
  config.type = OLED_SSD1306;
  config.protocol = I2C;
  config.width = 128;
  config.height = 64;
  config.i2cAddress = 0x3C;
  
  // Initialize library
  display.begin(SSD1306_SWITCHCAPVCC, config.i2cAddress);
  ErrorCode err = eyes.initialize(&display, config);
  
  if (err != OK) {
    // Handle initialization error
    while(1);
  }
}

void loop() {
  // Show happy emotion
  eyes.happy(1.0);
  
  // Update and render (call at target FPS, e.g., 20 Hz)
  eyes.update();
  display.display();
  
  delay(50);  // ~20 FPS
}
```

### Emotion Control

```cpp
// Using emotion helpers (recommended)
eyes.happy(1.0);      // Full intensity happy
eyes.sad(0.5);        // Mild sadness
eyes.excited(0.8);    // High excitement
eyes.calm(1.0);       // Deep calm
eyes.thinking(0.6);   // Thoughtful (asymmetric)
eyes.confused(0.7);   // Confused (asymmetric)

// Using parametric control (advanced)
eyes.setEmotion(0.3, 0.7, 400);  // valence +0.3, arousal 0.7, 400ms transition
```

### Eye Position Control

```cpp
// Using predefined behaviors
eyes.lookLeft();      // Look left (-45°, 0°)
eyes.lookRight();     // Look right (+45°, 0°)
eyes.lookUp();        // Look up (0°, +30°)
eyes.lookDown();      // Look down (0°, -30°)
eyes.neutral();       // Center gaze (0°, 0°)

// Custom position
eyes.setEyePosition(20, -10, 300);  // H: +20°, V: -10°, 300ms transition
```

### Animations

```cpp
eyes.blink(150);           // Blink both eyes for 150ms
eyes.wink(LEFT, 200);      // Wink left eye for 200ms
eyes.enableIdleBehavior(true);  // Enable micro-blinks every 3-5 seconds
```

## Examples

- **BasicEmotion.ino**: Cycle through all 12 emotion presets
- **EyePosition.ino**: Demonstrate eye gaze control and behaviors
- **SerialControl.ino**: Control via serial commands (AI integration ready)

## Memory Requirements

| Platform | Total SRAM | Library RAM | User Code Available | Status |
|----------|------------|-------------|---------------------|--------|
| Arduino Nano | 2KB | ~1.04KB (52%) | ~1000 bytes (48%) | ✅ Viable |
| Arduino Mega | 8KB | ~1.04KB (13%) | ~7KB (87%) | ✅ Comfortable |
| ESP32 | 520KB | ~1.04KB (<1%) | >500KB (>99%) | ✅ Excellent |

**Note**: Arduino Nano users should carefully manage user code to stay within ~1000 byte budget. Consider 128x32 displays for more headroom (~1500 bytes).

## PC Emulator

For AI-driven development and visual testing:

```bash
cd emulator/
pip install -r requirements.txt
python botieyes_emulator.py
```

Features:
- Real-time emotion/position sliders
- PNG frame capture for multimodal AI feedback
- JSON state export for debugging
- Matches embedded rendering exactly

## API Reference

See [`docs/API.md`](docs/API.md) for complete API documentation.

## Performance

- **Arduino Nano**: 15-20 FPS (target: 18-20ms render + 15ms I2C)
- **Arduino Mega**: 20-25 FPS
- **ESP32**: 30-60 FPS
- **PC Emulator**: 60+ FPS

## License

MIT License - see [LICENSE](LICENSE) file for details.

## Contributing

Contributions welcome! See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## Acknowledgments

Built with [Adafruit GFX Library](https://github.com/adafruit/Adafruit-GFX-Library)

---

**Designed for emotional robotics and AI-driven interactive systems** 🤖✨
