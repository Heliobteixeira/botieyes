
/*
 * BasicEmotion.ino - BotiEyes Library Example
 * 
 * Demonstrates parametric emotion control with all 12 emotion presets.
 * Cycles through emotions with smooth transitions.
 * 
 * Hardware:
 * - TTGO LoRa32 (ESP32 + integrated SSD1306 OLED 128x64 I2C)
 * - OLED pins: SDA=GPIO4, SCL=GPIO15, RST=GPIO16
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

// Display configuration (TTGO LoRa32)
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

// Create display object
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// ---- Brightness control (from BrightnessChange example) ----
enum BrightnessLevel { BRIGHT_LOW, BRIGHT_MED, BRIGHT_HIGH };

// SSD1306 contrast alone has minimal effect; combine 0x81 + 0xD9 + 0xDB.
void setBrightness(BrightnessLevel level) {
  uint8_t contrast, precharge, vcomh;
  switch (level) {
    case BRIGHT_LOW:  contrast = 0x00; precharge = 0x11; vcomh = 0x00; break;
    case BRIGHT_MED:  contrast = 0x80; precharge = 0x55; vcomh = 0x20; break;
    case BRIGHT_HIGH:
    default:          contrast = 0xFF; precharge = 0xF1; vcomh = 0x40; break;
  }
  display.ssd1306_command(0x81); display.ssd1306_command(contrast);
  display.ssd1306_command(0xD9); display.ssd1306_command(precharge);
  display.ssd1306_command(0xDB); display.ssd1306_command(vcomh);
}

// Create BotiEyes object
BotiEyes::BotiEyes eyes;

// Emotion cycle state
uint8_t currentEmotion = 0;
uint32_t lastEmotionChange = 0;
const uint16_t EMOTION_DURATION = 3000;  // 3 seconds per emotion

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  Serial.println(F("BotiEyes - BasicEmotion Example"));
  Serial.println(F("================================"));

  // Hardware reset the OLED (required on TTGO LoRa32)
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  // Initialize I2C on TTGO LoRa32 OLED pins
  Wire.begin(OLED_SDA, OLED_SCL);

  // Initialize display
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
    Serial.println(F("ERROR: SSD1306 allocation failed!"));
    while (1);  // Halt
  }

  Serial.println(F("Display initialized"));

  // Set maximum brightness
  setBrightness(BRIGHT_HIGH);

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

  // Attach the display so BotiEyes can render eyes directly
  eyes.setDisplay(&display);

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
  
  // Update emotion interpolation + render eyes into the display buffer
  eyes.update();

  // Push buffer to the OLED
  display.display();

  // Target 20 FPS (50ms per frame)
  delay(50);
}
