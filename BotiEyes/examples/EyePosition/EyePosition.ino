/*
 * EyePosition.ino - BotiEyes Library Example
 *
 * Demonstrates coupled 2D eye gaze control (US4).
 * Cycles through predefined look-at helpers and a custom (H, V) pose,
 * with a held "content" emotion so the shape stays readable.
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
 * - Eyes hold Content emotion
 * - Gaze cycles through: center -> left -> right -> up -> down ->
 *   diagonal up-right -> diagonal down-left -> center
 * - Each pose holds for 1.5 s with smooth 300 ms interpolation
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BotiEyes.h>

#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

enum BrightnessLevel { BRIGHT_LOW, BRIGHT_MED, BRIGHT_HIGH };

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

uint8_t  currentPose       = 0;
uint32_t lastPoseChange    = 0;
const uint16_t POSE_HOLD_MS = 1500;

void setup() {
  Serial.begin(115200);
  Serial.println(F("BotiEyes - EyePosition Example"));

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
    Serial.println(F("ERROR: SSD1306 allocation failed!"));
    while (1);
  }

  setBrightness(BRIGHT_HIGH);
  display.clearDisplay();
  display.display();

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
    Serial.println(F("ERROR: BotiEyes init failed"));
    while (1);
  }

  eyes.setDisplay(&display);

  // Hold a pleasant base emotion so eye shape is clear
  eyes.content(1.0f);
  eyes.neutral();
  lastPoseChange = millis();
}

void loop() {
  uint32_t now = millis();

  if (now - lastPoseChange >= POSE_HOLD_MS) {
    currentPose = (currentPose + 1) % 8;
    switch (currentPose) {
      case 0:
        Serial.println(F("Gaze: center"));
        eyes.neutral();
        break;
      case 1:
        Serial.println(F("Gaze: left"));
        eyes.lookLeft();
        break;
      case 2:
        Serial.println(F("Gaze: right"));
        eyes.lookRight();
        break;
      case 3:
        Serial.println(F("Gaze: up"));
        eyes.lookUp();
        break;
      case 4:
        Serial.println(F("Gaze: down"));
        eyes.lookDown();
        break;
      case 5:
        Serial.println(F("Gaze: diagonal up-right (+60, +20)"));
        eyes.setEyePosition(60, 20, 300);
        break;
      case 6:
        Serial.println(F("Gaze: diagonal down-left (-60, -20)"));
        eyes.setEyePosition(-60, -20, 300);
        break;
      case 7:
        Serial.println(F("Gaze: custom recenter via setEyePosition(0,0)"));
        eyes.setEyePosition(0, 0, 300);
        break;
    }

    // Log interpolated position for sanity
    int16_t h, v;
    eyes.getEyePosition(&h, &v);
    Serial.print(F("  target starting from h=")); Serial.print(h);
    Serial.print(F(" v=")); Serial.println(v);

    lastPoseChange = now;
  }

  eyes.update();
  display.display();
  delay(20);  // ~50 Hz
}
