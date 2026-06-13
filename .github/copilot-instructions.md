# botieyes Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-04-17

## Active Technologies

- C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator + Adafruit GFX (embedded rendering only); Pygame (emulator only) (001-emotion-driven-eyes)
- C++ (Arduino/PlatformIO, ESP32 core) network control layer + Python 3.8+ reference controller client + ESP32 Wi-Fi (WiFi/WiFiUDP); stdlib socket (controller) (002-esp32-network-service)

## Project Structure

```text
src/
tests/
```

## Commands

cd src; pytest; ruff check .

## Code Style

C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator: Follow standard conventions

## Recent Changes

- 002-esp32-network-service: Added ESP32 UDP network control service (best-effort streaming + acks/heartbeats, single-controller lock, 5 s timeout) layered on the existing BotiEyes API
- 001-emotion-driven-eyes: Added C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator + Adafruit GFX (embedded rendering only); Pygame (emulator only)

<!-- MANUAL ADDITIONS START -->

## Commit policy for embedded C++ changes

Before committing any change under `BotiEyes/src/**` or `BotiEyes/examples/**`
that affects rendering, timing, I2C, display init, or the public API:

1. Build and upload the relevant example sketch to the TTGO LoRa32 board.
2. Visually verify the behavior on the OLED.
3. Only then run `git add` + `git commit`.

Python emulator changes (`emulator/**`) can be validated by running the
emulator alone.

### Hardware reference

- Board: TTGO LoRa32 (ESP32 + integrated SSD1306 128x64).
- OLED pins: SDA=GPIO4, SCL=GPIO15, RST=GPIO16.

<!-- MANUAL ADDITIONS END -->

<!-- SPECKIT START -->
For additional context about technologies to be used, project structure,
shell commands, and other important information, read the current plan:
specs/003-ssd1306-spi-component/plan.md
<!-- SPECKIT END -->
