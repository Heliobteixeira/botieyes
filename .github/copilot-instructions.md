# botieyes Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-04-17

## Active Technologies

- C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator + Adafruit GFX (embedded rendering only); Pygame (emulator only) (001-emotion-driven-eyes)

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
