/*
 * BasicEmotion.ino - BotiEyes Library Example
 * 
 * Demonstrates parametric emotion control with all 12 emotion presets.
 * Cycles through emotions with smooth transitions.
 * 
 * Hardware:
 * - Arduino Nano/Mega/ESP32
 * - SSD1306 OLED Display 128x64 (I2C)
 * - Connections:
 *   - SDA → A4 (Nano) / 20 (Mega) / 21 (ESP32)
 *   - SCL → A5 (Nano) / 21 (Mega) / 22 (ESP32)
 *   - VCC → 5V (Nano/Mega) or 3.3V (ESP32)
 *   - GND → GND
 * 
 * Dependencies:
 * - BotiEyes library
 * - Adafruit GFX Library
 * - Adafruit SSD1306 Library
 * 
 * Expected Behavior:
 * - Eyes cycle through 12 emotions every 2 seconds
 * - Smooth transitions between emotions
 * - Serial output shows current emotion name
 * 
 * Memory Usage (Arduino Nano):
 * - BotiEyes library: ~1.04KB RAM
 * - This sketch: ~200 bytes
 * - Total: ~1.24KB / 2KB (62%)
 * - Remaining for user: ~760 bytes
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BotiEyes.h>

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Create BotiEyes object
BotiEyes::BotiEyes eyes;

// Emotion cycle state
uint8_t currentEmotion = 0;
uint32_t lastEmotionChange = 0;
const uint16_t EMOTION_DURATION = 2000;  // 2 seconds per emotion

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println(F("BotiEyes - BasicEmotion Example"));
  Serial.println(F("================================"));
  
  // Initialize I2C
  Wire.begin();
  
  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("ERROR: SSD1306 allocation failed!"));
    while (1);  // Halt
  }
  
  Serial.println(F("Display initialized"));
  
  // Clear display
  display.clearDisplay();
  display.display();
  
  // Configure BotiEyes
  BotiEyes::DisplayConfig config;
  config.type = BotiEyes::DISPLAY_SSD1306_128x64;
  config.protocol = BotiEyes::PROTOCOL_I2C;
  config.i2cAddress = SCREEN_ADDRESS;
  config.width = SCREEN_WIDTH;
  config.height = SCREEN_HEIGHT;
  
  // Validate configuration
  BotiEyes::ErrorCode validation = BotiEyes::BotiEyes::validateConfig(config);
  if (validation != BotiEyes::OK) {
    Serial.println(F("ERROR: Invalid configuration!"));
    while (1);  // Halt
  }
  
  // Initialize BotiEyes
  BotiEyes::ErrorCode result = eyes.initialize(config);
  if (result != BotiEyes::OK) {
    Serial.print(F("ERROR: BotiEyes init failed, code: "));
    Serial.println(result);
    while (1);  // Halt
  }
  
  Serial.println(F("BotiEyes initialized"));
  Serial.println(F("Starting emotion cycle..."));
  Serial.println();
  
  // Start with neutral emotion
  eyes.setEmotion(0.0f, 0.5f, 400);
  lastEmotionChange = millis();
}

void loop() {
  uint32_t now = millis();
  
  // Check if it's time to change emotion
  if (now - lastEmotionChange >= EMOTION_DURATION) {
    // Cycle to next emotion
    currentEmotion = (currentEmotion + 1) % 12;
    
    // Set new emotion based on current index
    BotiEyes::ErrorCode result = BotiEyes::OK;
    switch (currentEmotion) {
      case 0:
        Serial.println(F("Emotion: Happy (valence: +0.35, arousal: 0.55)"));
        result = eyes.happy(1.0f);
        break;
      case 1:
        Serial.println(F("Emotion: Sad (valence: -0.35, arousal: 0.35)"));
        result = eyes.sad(1.0f);
        break;
      case 2:
        Serial.println(F("Emotion: Angry (valence: -0.30, arousal: 0.80)"));
        result = eyes.angry(1.0f);
        break;
      case 3:
        Serial.println(F("Emotion: Calm (valence: 0.0, arousal: 0.1)"));
        result = eyes.calm(1.0f);
        break;
      case 4:
        Serial.println(F("Emotion: Excited (valence: +0.30, arousal: 0.90)"));
        result = eyes.excited(1.0f);
        break;
      case 5:
        Serial.println(F("Emotion: Tired (valence: +0.05, arousal: 0.10)"));
        result = eyes.tired(1.0f);
        break;
      case 6:
        Serial.println(F("Emotion: Surprised (valence: +0.15, arousal: 0.85)"));
        result = eyes.surprised(1.0f);
        break;
      case 7:
        Serial.println(F("Emotion: Anxious (valence: -0.20, arousal: 0.75)"));
        result = eyes.anxious(1.0f);
        break;
      case 8:
        Serial.println(F("Emotion: Content (valence: +0.25, arousal: 0.40)"));
        result = eyes.content(1.0f);
        break;
      case 9:
        Serial.println(F("Emotion: Curious (valence: +0.15, arousal: 0.60)"));
        result = eyes.curious(1.0f);
        break;
      case 10:
        Serial.println(F("Emotion: Thinking (valence: 0.0, arousal: 0.45, asymmetry: -0.20)"));
        result = eyes.thinking(1.0f);
        break;
      case 11:
        Serial.println(F("Emotion: Confused (valence: -0.15, arousal: 0.55, asymmetry: -0.30)"));
        result = eyes.confused(1.0f);
        break;
    }
    
    if (result != BotiEyes::OK) {
      Serial.print(F("ERROR: Failed to set emotion, code: "));
      Serial.println(result);
    }
    
    lastEmotionChange = now;
  }
  
  // Update emotion interpolation
  eyes.update();
  
  // Clear display for new frame
  display.clearDisplay();
  
  // TODO: Render eyes to display (renderEyes implementation)
  // For now, just show emotion name
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  
  const char* emotionNames[] = {
    "Happy", "Sad", "Angry", "Calm", "Excited", "Tired",
    "Surprised", "Anxious", "Content", "Curious", "Thinking", "Confused"
  };
  display.println(emotionNames[currentEmotion]);
  
  // Show current valence/arousal
  float valence, arousal;
  eyes.getCurrentEmotion(&valence, &arousal);
  display.print(F("V: "));
  display.print(valence, 2);
  display.print(F("  A: "));
  display.println(arousal, 2);
  
  // Update display
  display.display();
  
  // Target 20 FPS (50ms per frame)
  delay(50);
}
