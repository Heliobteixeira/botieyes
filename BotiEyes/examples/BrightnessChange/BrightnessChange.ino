/*********
  SSD1306 OLED Brightness Test (TTGO LoRa32)
  Cycles through 3 brightness levels: LOW / MED / HIGH
*********/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED pins (TTGO LoRa32)
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

enum BrightnessLevel { BRIGHT_LOW, BRIGHT_MED, BRIGHT_HIGH };

// Single entry point for brightness control.
// Combines contrast (0x81), pre-charge (0xD9) and VCOMH (0xDB)
// because the SSD1306 contrast register alone has minimal effect.
void setBrightness(BrightnessLevel level) {
  uint8_t contrast, precharge, vcomh;
  const char* name;

  switch (level) {
    case BRIGHT_LOW:  contrast = 0x00; precharge = 0x11; vcomh = 0x00; name = "LOW";  break;
    case BRIGHT_MED:  contrast = 0x80; precharge = 0x55; vcomh = 0x20; name = "MED";  break;
    case BRIGHT_HIGH:
    default:          contrast = 0xFF; precharge = 0xF1; vcomh = 0x40; name = "HIGH"; break;
  }

  display.ssd1306_command(0x81); display.ssd1306_command(contrast);
  display.ssd1306_command(0xD9); display.ssd1306_command(precharge);
  display.ssd1306_command(0xDB); display.ssd1306_command(vcomh);

  Serial.print("Brightness: "); Serial.println(name);

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(0, 0);
  display.println(name);
  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print("C=0x"); display.println(contrast, HEX);
  display.print("P=0x"); display.println(precharge, HEX);
  display.print("V=0x"); display.println(vcomh, HEX);
  display.display();
}

void setup() {
  Serial.begin(115200);

  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;);
  }

  Serial.println("OLED Brightness Test");
}

void loop() {
  setBrightness(BRIGHT_LOW);
  delay(2000);
  setBrightness(BRIGHT_MED);
  delay(2000);
  setBrightness(BRIGHT_HIGH);
  delay(2000);
}