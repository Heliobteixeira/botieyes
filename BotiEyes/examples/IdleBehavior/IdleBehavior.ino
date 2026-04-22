/*
 * IdleBehavior.ino - BotiEyes Library Example
 *
 * Sets a single default emotion (content) and enables idle behavior.
 * Demonstrates how the built-in idle system produces life-like micro-blinks
 * and subtle morphing without the sketch doing anything per-frame.
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
 * - Eyes show the "content" emotion.
 * - Periodic blinks + subtle expression morphing while idle.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BotiEyes.h>

// Display configuration (TTGO LoRa32)
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA       4
#define OLED_SCL       15
#define OLED_RST       16

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

BotiEyes::BotiEyes eyes;

void setup() {
  Serial.begin(115200);
  Serial.println(F("BotiEyes - IdleBehavior Example"));
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

  setBrightness(BRIGHT_HIGH);

  display.clearDisplay();
  display.display();

  // Configure BotiEyes
  BotiEyes::DisplayConfig config;
  config.type       = BotiEyes::DISPLAY_SSD1306_128x64;
  config.protocol   = BotiEyes::PROTOCOL_I2C;
  config.i2cAddress = SCREEN_ADDRESS;
  config.width      = SCREEN_WIDTH;
  config.height     = SCREEN_HEIGHT;

  if (BotiEyes::BotiEyes::validateConfig(config) != BotiEyes::OK) {
    Serial.println(F("ERROR: Invalid configuration!"));
    while (1);
  }

  if (eyes.initialize(config) != BotiEyes::OK) {
    Serial.println(F("ERROR: BotiEyes init failed!"));
    while (1);
  }

  eyes.setDisplay(&display);

  // Set default emotion + enable idle behavior
  eyes.content(1.0f);
  eyes.enableIdleBehavior(true);

  Serial.println(F("Default emotion: content"));
  Serial.println(F("Idle behavior: enabled"));
}

void loop() {
  // Update interpolation + idle behavior, render, and push to OLED.
  eyes.update();
  display.display();

  // Target 20 FPS
  delay(50);
}
