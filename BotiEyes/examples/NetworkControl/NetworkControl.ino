/*
 * NetworkControl.ino - BotiEyes Library Example
 *
 * Remote network control of BotiEyes over Wi-Fi (UDP).
 * A controller on the SAME local network drives emotions, gaze, presets,
 * blink/wink and idle in real time with low latency and automatic recovery.
 *
 * Hardware:
 * - TTGO LoRa32 (ESP32 + integrated SSD1306 OLED 128x64 I2C)
 * - OLED pins: SDA=GPIO4, SCL=GPIO15, RST=GPIO16
 *
 * Dependencies:
 * - BotiEyes library (incl. BotiEyes/src/net/BotiEyesServer.*)
 * - Adafruit GFX Library
 * - Adafruit SSD1306 Library
 *
 * Behavior:
 * - Joins Wi-Fi, then shows the device IP on the OLED so an operator can
 *   connect on the first attempt (the controller targets that IP).
 * - Runs a non-blocking UDP server on port 4210. The render loop never
 *   blocks on the network: if Wi-Fi drops or the controller crashes, the
 *   eyes keep animating and the device returns to idle behavior after ~5 s
 *   of controller silence, then becomes controllable again automatically.
 *
 * SECURITY / TRUST MODEL (FR-018):
 * - This service has NO authentication or encryption. It is intended for a
 *   TRUSTED local network ONLY. Any host on the LAN that can reach UDP
 *   port 4210 can control the eyes. Do NOT port-forward or otherwise expose
 *   port 4210 to the public Internet or any untrusted network.
 *
 * See specs/002-esp32-network-service/quickstart.md for usage.
 */

#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BotiEyes.h>
#include <net/BotiEyesServer.h>

// ---- Wi-Fi credentials (EDIT THESE) ----
const char* WIFI_SSID = "your-ssid";
const char* WIFI_PASS = "your-password";

// ---- Display configuration (TTGO LoRa32) ----
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  64
#define SCREEN_ADDRESS 0x3C
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
BotiEyes::BotiEyes eyes;
BotiEyes::net::BotiEyesServer server(eyes);

// Render at ~25 FPS (40 ms/frame).
const uint16_t FRAME_INTERVAL_MS = 40;
uint32_t lastFrame = 0;

// Re-join Wi-Fi in the background without blocking the render loop.
uint32_t lastWifiCheck = 0;
const uint16_t WIFI_CHECK_INTERVAL_MS = 2000;

// Show the device IP briefly after (re)connecting.
void showAddress() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("BotiEyes Network"));
  display.print(F("IP: "));
  display.println(WiFi.localIP());
  display.print(F("UDP port: "));
  display.println(4210);
  display.display();
  delay(2000);  // one-time, during setup only
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("BotiEyes - NetworkControl Example"));

  // Hardware reset the OLED (required on TTGO LoRa32).
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false)) {
    Serial.println(F("ERROR: SSD1306 allocation failed!"));
    while (1);
  }
  display.clearDisplay();
  display.display();

  // Configure + initialize BotiEyes.
  BotiEyes::DisplayConfig config;
  config.type = BotiEyes::DISPLAY_SSD1306_128x64;
  config.protocol = BotiEyes::PROTOCOL_I2C;
  config.i2cAddress = SCREEN_ADDRESS;
  config.width = SCREEN_WIDTH;
  config.height = SCREEN_HEIGHT;
  if (eyes.initialize(config) != BotiEyes::OK) {
    Serial.println(F("ERROR: BotiEyes init failed!"));
    while (1);
  }
  eyes.setDisplay(&display);

  // Join Wi-Fi (non-fatal: loop() keeps retrying if this fails).
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print(F("Connecting to Wi-Fi"));
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print(F("Connected. IP: "));
    Serial.println(WiFi.localIP());
    showAddress();
  } else {
    Serial.println(F("Wi-Fi not connected yet; will retry in background."));
  }

  // Start the UDP control server (binds even before Wi-Fi is up; harmless).
  server.begin();
  Serial.println(F("Network control server started on UDP 4210"));
}

void loop() {
  const uint32_t now = millis();

  // Background Wi-Fi watchdog — never blocks rendering.
  if (now - lastWifiCheck >= WIFI_CHECK_INTERVAL_MS) {
    lastWifiCheck = now;
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.reconnect();
    }
  }

  // Drain inbound control datagrams (non-blocking).
  server.poll();

  // Render a frame at the target FPS.
  if (now - lastFrame >= FRAME_INTERVAL_MS) {
    lastFrame = now;
    server.applyPending();  // apply newest-wins targets + queued actions
    eyes.update();          // interpolate + draw
  }
}
