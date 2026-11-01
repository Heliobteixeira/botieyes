# BotiEyes

Emotion-driven eye animation library for Arduino/ESP32 OLED and TFT displays.

---

## Overview

BotiEyes solves the problem of making robot eyes look alive. Instead of hand-animating frames, you describe an emotional state (valence + arousal) and the library continuously interpolates eye shape, brow position, and expression geometry to match.

There are two ways to use it:

| Mode | What you need | When to use |
|------|--------------|-------------|
| **Arduino library** | Any Arduino-compatible board + supported display | Embedded deployment — wire up a display, `#include <BotiEyes.h>`, call `eyes.happy(1.0)` |
| **ESP-IDF firmware** | TENSTAR TS-ESP32-S3 + ST7789 TFT | Production firmware with Wi-Fi network control |

On ESP32, the library also provides a **UDP network control service** so a host program can drive the eyes in real time over Wi-Fi. The `esp-idf/` folder contains a production-grade ESP-IDF v6.0.1 firmware build targeting the TENSTAR TS-ESP32-S3 board.

---

## Features

- **Valence-arousal emotion model** — continuous coordinates (valence: −0.5…+0.5, arousal: 0.0…1.0) map to parametric eye geometry
- **13 named emotion presets** — `happy`, `sad`, `angry`, `calm`, `excited`, `tired`, `surprised`, `anxious`, `content`, `curious`, `thinking`, `confused`, `neutral`
- **Smooth transitions** — configurable ease-in-out-cubic interpolation between any two emotional states
- **Coupled eye position** — 2D gaze direction (h: −90°…+90°, v: −45°…+45°) moves both eyes together; helper methods `lookLeft()`, `lookRight()`, `lookUp()`, `lookDown()`
- **Built-in animations** — `blink()`, `wink(LEFT|RIGHT)`, autonomous idle behavior (periodic micro-blinks + subtle morphing)
- **Supported displays**
  - SSD1306 OLED 128×64 (I2C or SPI) — primary target for the Arduino library
  - SSD1306 OLED 128×32
  - SH1106 OLED 128×64
  - SSD1306 OLED 64×48
  - ST7789 TFT 240×135 RGB565 (ESP32 only, frame-buffered)
- **UDP network control** — `BotiEyesServer` (ESP32) + Python `BotiEyesClient` reference implementation; single-controller lock, heartbeat, 5 s timeout
- **WS2812 status LED** (ESP-IDF firmware)
- **Memory-efficient** — ~1.04 KB RAM on Arduino Nano; static allocation throughout

---

## Repository Structure

```
BotiEyes/           Arduino library (install via Library Manager or copy to libraries/)
  src/              Core C++ source: BotiEyes, EmotionMapper, Interpolator, RenderingHelpers…
  examples/         Five Arduino examples (BasicEmotion, EyePosition, IdleBehavior, NetworkControl…)

controller/         Python 3 UDP reference controller
  botieyes_client.py   BotiEyesClient class (stdlib only, no pip deps)
  cli.py               Interactive CLI for driving a live device

esp-idf/            ESP-IDF v6.0.1 firmware (TENSTAR TS-ESP32-S3 + ST7789 TFT)
  main/             Application entry point, task architecture, display init
  components/       Eight custom components: botieyes, config_manager, display,
                    hal_board, health_monitor, ssd1306_config, state_machine, wifi_manager
  managed_components/  Third-party: espressif__led_strip, fasani__adafruit_gfx, nopnop2002__st7789

scripts/
  check-postponed.sh   Reports incomplete tasks, code TODOs, and temporary components

specs/              Feature specs and implementation plans (001–005)
docs/               Architecture docs, display config review, synchronization guide,
                    postponed features tracker

tests/net/          Off-target C++ tests for CommandCodec and SessionManager
  flood_test.py     On-hardware soak test (run from controller/)
```

---

## Hardware Requirements

### Arduino Library (examples)

All five examples are wired and tested on:

| | |
|---|---|
| **Board** | TTGO LoRa32 (ESP32 + integrated SSD1306 128×64) |
| **Display** | SSD1306 OLED 128×64, I2C |
| **OLED SDA** | GPIO 4 |
| **OLED SCL** | GPIO 15 |
| **OLED RST** | GPIO 16 |
| **I2C address** | 0x3C |

The library itself is portable to any Arduino-compatible platform with an Adafruit GFX–compatible display driver.

### ESP-IDF Firmware

| | |
|---|---|
| **Board** | TENSTAR TS-ESP32-S3 (ESP32-S3-WROOM-1) |
| **Display** | 1.14" ST7789 TFT, 240×135 RGB565, SPI |
| **MOSI** | GPIO 35 |
| **SCLK** | GPIO 36 |
| **CS** | GPIO 7 |
| **DC** | GPIO 39 |
| **RST** | GPIO 40 |
| **Backlight** | GPIO 45 |
| **Status LED** | GPIO 38 (WS2812) |
| **GRAM offsets** | X+40, Y+53 (verified for this board) |

> **Discrepancy note:** `docs/firmware-architecture.md` describes the target as "ESP32-S3 with SSD1306 OLED (SPI)" — this reflects Feature 004. `sdkconfig.defaults` and Feature 005 migrated to the ST7789 SPI TFT. ST7789 is the current build target.

---

## Getting Started

### Path A — Arduino Library (TTGO LoRa32 / any ESP32 + SSD1306)

**Dependencies (install via Arduino Library Manager):**
- Adafruit GFX Library
- Adafruit SSD1306

**Install BotiEyes:**
1. Download this repository.
2. Copy `BotiEyes/` into your Arduino `libraries/` folder.
3. Restart the Arduino IDE.
4. Open an example: `File → Examples → BotiEyes → BasicEmotion`.

---

### Path B — ESP-IDF Firmware (TENSTAR TS-ESP32-S3)

**Toolchain:** ESP-IDF v6.0.1

```bash
# Activate IDF (adjust path for your install)
source ~/.espressif/tools/activate_idf_v6.0.1.sh

cd esp-idf

# Configure: set Wi-Fi credentials and verify display pins
idf.py menuconfig
# → BotiEyes Network Control → set Wi-Fi SSID / password

# Build
idf.py build

# Flash (connect board via USB)
idf.py flash

# Monitor serial output
idf.py monitor
```

The firmware starts, connects to Wi-Fi, and begins rendering eye animations. The device IP is available on the serial monitor; use it with the CLI controller (see Usage below).

---

## Usage

### Arduino — triggering emotions

```cpp
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BotiEyes.h>

Adafruit_SSD1306 display(128, 64, &Wire, 16); // RST = GPIO16
BotiEyes::BotiEyes eyes;

void setup() {
    Wire.begin(4, 15); // SDA=4, SCL=15 (TTGO LoRa32)
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

    DisplayConfig cfg;
    cfg.type     = BotiEyes::DISPLAY_SSD1306_128x64;
    cfg.protocol = BotiEyes::PROTOCOL_I2C;
    cfg.width    = 128;
    cfg.height   = 64;
    cfg.i2cAddress = 0x3C;

    eyes.initialize(cfg);
    eyes.setDisplay(&display);
}

void loop() {
    // Named preset (intensity 0.0–1.0)
    eyes.happy(1.0);
    delay(2000);

    // Raw valence-arousal (v: -0.5..+0.5, a: 0.0..1.0, transition: ms)
    eyes.setEmotion(-0.30f, 0.80f, 400); // equivalent to eyes.angry(1.0)
    delay(2000);

    // Gaze + animation
    eyes.lookLeft();
    eyes.blink();
    eyes.wink(BotiEyes::RIGHT);

    // Render frame (~25 FPS)
    eyes.update();
    display.display();
    delay(40);
}
```

### Arduino — idle behavior

```cpp
eyes.content(1.0);
eyes.enableIdleBehavior(true); // periodic micro-blinks + subtle morphing
// Just call eyes.update() in loop(); no manual animation code needed
```

### Network control — CLI (requires running firmware)

```bash
cd controller
python3 cli.py --host 192.168.1.139   # substitute your device IP

# Commands at the prompt:
preset happy 1.0
emotion -0.30 0.80 400
gaze 45 0
blink
wink left
idle on
status
```

### Network control — Python API

```python
from botieyes_client import BotiEyesClient

with BotiEyesClient("192.168.1.139") as eyes:
    eyes.preset("happy", 1.0)
    eyes.set_emotion(-0.30, 0.80, duration_ms=400)
    eyes.set_gaze(h=45, v=0)
    eyes.blink()
    print(eyes.status())
```

> **Security note:** The UDP service has no authentication. Use it only on a trusted local network; never expose port 4210 to the public internet.

---

## Testing

### Off-target unit tests (no hardware required)

Tests cover `CommandCodec` (frame encode/decode, truncated/oversized frames, bad version, unknown opcode, out-of-range rejection) and `SessionManager` (lock acquire/reject/takeover, heartbeat refresh, 5 s timeout, newest-wins coalescing).

```bash
cd tests/net
make          # builds and runs all suites
make clean
```

Run individual suites:

```bash
make test_command_codec   && ./test_command_codec
make test_session_manager && ./test_session_manager
```

### On-hardware flood / stability test

Streams emotion + gaze updates at ~10× normal rate for a configurable duration. Requires a running device.

```bash
cd controller
python ../tests/net/flood_test.py --host <device-ip> --duration 300
```

Pass criteria (manual observation + final `status()` call): eyes keep animating smoothly, device does not reset, heap stays stable.

### Check for deferred work

```bash
./scripts/check-postponed.sh
```

Reports open tasks from `specs/**/tasks.md`, code `TODO`/`FIXME` markers, and temporary components.

---

## Project Status / Roadmap

Five features are implemented:

| Feature | Status |
|---------|--------|
| 001 — Emotion-driven eyes (Arduino library + emulator) | Done |
| 002 — ESP32 UDP network control service | Done |
| 003 — SSD1306 SPI component | Done |
| 004 — Industrial ESP-IDF firmware architecture | Done |
| 005 — ST7789 SPI color display support | Done |

**Known deferred items** (tracked in `docs/postponed-features.md`):

- `hal_display_spi.c` / `hal_display_i2c.c` — HAL display implementations are stubs (42 lines each); the `display/` component is a working temporary workaround.
- T060 / T080 — Kconfig and NVS split documentation (non-blocking, low priority).

---

## License

No `LICENSE` file is present at the repository root. The `BotiEyes/library.properties` file lists the library as authored by Helio Teixeira but does not declare a license. Third-party managed components (e.g. `esp-idf/managed_components/espressif__led_strip/`) carry their own licenses.
