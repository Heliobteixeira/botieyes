# Quickstart: SSD1306 SPI Integration

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)
**Target Audience**: Embedded developers integrating BotiEyes with ESP32-S3
**Prerequisites**: ESP-IDF 5.0+, ESP32-S3 board, SSD1306 OLED display
**Est. Time**: 15-20 minutes

---

## What You'll Achieve

By the end of this quickstart, you will:
1. ✅ Wire SSD1306 OLED to ESP32-S3 via SPI
2. ✅ Configure SPI pins through menuconfig
3. ✅ Build and flash BotiEyes firmware
4. ✅ Verify display shows eye animations at 25 FPS

---

## Step 1: Hardware Wiring (5 min)

### Required Components
- ESP32-S3 development board (any variant with >48 GPIO)
- SSD1306 OLED display (128x64, SPI-capable)
- 6 jumper wires (female-to-female recommended)
- USB-C cable for programming

### Wiring Table

| SSD1306 Pin | Signal | ESP32-S3 GPIO | Color Code |
|-------------|--------|---------------|------------|
| VCC | Power (3.3V) | 3V3 | Red |
| GND | Ground | GND | Black |
| D0 / SCK | SPI Clock | GPIO12 | Yellow |
| D1 / MOSI | SPI Data | GPIO11 | Blue |
| CS | Chip Select | GPIO10 | Green |
| DC / A0 | Data/Command | GPIO9 | Orange |
| RST / RES | Reset | GPIO8 | White |

### Wiring Diagram
```
ESP32-S3                SSD1306 OLED
┌─────────────┐         ┌──────────────┐
│             │         │              │
│  3V3 ●──────┼─────────┼──● VCC       │
│  GND ●──────┼─────────┼──● GND       │
│             │         │              │
│ GPIO12 ●────┼─────────┼──● D0/SCK    │
│ GPIO11 ●────┼─────────┼──● D1/MOSI   │
│ GPIO10 ●────┼─────────┼──● CS        │
│  GPIO9 ●────┼─────────┼──● DC/A0     │
│  GPIO8 ●────┼─────────┼──● RST/RES   │
│             │         │              │
└─────────────┘         └──────────────┘
```

### Verification Checklist
- [ ] VCC connected to 3.3V (NOT 5V - risk of damage)
- [ ] GND connected securely
- [ ] All 5 SPI signal wires connected
- [ ] No short circuits between adjacent pins
- [ ] Display powered on (backlight visible if present)

---

## Step 2: Clone Repository & Setup (3 min)

```bash
# Clone BotiEyes repository
git clone https://github.com/Heliobteixeira/botieyes.git
cd botieyes

# Checkout SPI feature branch
git checkout 003-ssd1306-spi-component

# Navigate to ESP-IDF project
cd esp-idf

# Set ESP-IDF target (ESP32-S3)
idf.py set-target esp32s3
```

**Expected output**:
```
Setting IDF_TARGET to esp32s3
Target set to esp32s3
```

---

## Step 3: Configure via menuconfig (5 min)

```bash
idf.py menuconfig
```

### Configuration Steps

1. **Navigate to display configuration**:
   - Arrow down to `BotiEyes Display Configuration`
   - Press `Enter`

2. **Select SPI protocol**:
   - Select `OLED communication protocol`
   - Press `Enter`
   - Arrow down to `Hardware SPI`
   - Press `Space` to select
   - Press `Enter` to confirm

3. **Verify pin assignments** (should match wiring table):
   ```
   OLED SPI MOSI pin: 11
   OLED SPI SCK pin: 12
   OLED SPI CS pin: 10
   OLED SPI DC pin: 9
   OLED SPI RST pin: 8
   SPI clock speed (Hz): 10000000
   ```

4. **Save configuration**:
   - Press `Q` to quit
   - Press `Y` to save changes

### Custom Pin Assignments (Optional)

If your wiring differs from defaults:
1. Navigate to each pin option (e.g., `OLED SPI MOSI pin`)
2. Press `Enter` to edit
3. Type new GPIO number
4. Press `Enter` to confirm
5. Repeat for all changed pins

**Valid GPIO range**: 0-48 (avoid GPIO6-11 if possible - used for flash)

---

## Step 4: Build Firmware (2 min)

```bash
idf.py build
```

**Expected output** (truncated):
```
[1/1] Generating project dependencies...
[1/5] Building esp-idf components
[2/5] Fetching nopnop2002__ssd1306...
[3/5] Linking botieyes-esp-idf.elf
[4/5] Generating binary image
Project build complete. To flash, run:
    idf.py flash
```

### Troubleshooting Build Errors

| Error | Cause | Solution |
|-------|-------|----------|
| `Component 'ssd1306' not found` | idf_component.yml not fetched | Run `idf.py reconfigure` |
| `undefined reference to spi_master_init` | Component not linked | Verify `idf_component.yml` exists in `main/` |
| `CONFIG_BOTIEYES_OLED_MOSI_PIN undeclared` | Kconfig not saved | Re-run `idf.py menuconfig`, save changes |

---

## Step 5: Flash & Monitor (3 min)

### Find USB Serial Port

**macOS/Linux**:
```bash
ls /dev/tty.usbserial* /dev/ttyUSB* 2>/dev/null
```

**Windows**:
```
Check Device Manager → Ports (COM & LPT)
```

### Flash Firmware

```bash
idf.py -p /dev/ttyUSB0 flash
```
(Replace `/dev/ttyUSB0` with your serial port)

**Expected output**:
```
Connecting.....
Chip is ESP32-S3 (revision v0.1)
Features: WiFi, BLE
Writing at 0x00010000... (100 %)
Wrote 1234567 bytes at 0x00010000
Hash of data verified.
Leaving...
Hard resetting via RTS pin...
```

### Monitor Serial Output

```bash
idf.py -p /dev/ttyUSB0 monitor
```

**Expected initialization logs**:
```
I (123) main: === BotiEyes ESP-IDF Startup ===
I (145) display_init: Initializing SSD1306 SPI: MOSI=11, SCK=12, CS=10, DC=9
I (163) display_init: SSD1306 init completed in 18 ms
I (168) display_init: SSD1306 SPI initialization successful
I (180) main: All systems initialized successfully
```

**Press `Ctrl+]` to exit monitor**

---

## Step 6: Verify Display Output (2 min)

### Success Indicators

✅ **Status LED**: Green (GPIO38)  
✅ **Serial log**: "SSD1306 SPI initialization successful"  
✅ **Display**: Shows BotiEyes logo or initial emotion state  
✅ **Animation**: Eyes blink and move smoothly (25 FPS)

### If Display is Blank

Check initialization log for errors:

| Error Message | Cause | Fix |
|---------------|-------|-----|
| `SSD1306 init timeout after 2000 ms` | Display not responding | Check wiring, verify 3.3V power |
| `SPI bus init failed: Invalid pin configuration` | GPIO conflict or invalid pin | Verify pin numbers in menuconfig |
| `DMA allocation failed` | Insufficient memory | Reduce stack sizes in menuconfig |
| `GPIO config failed` | Pin already in use | Check for pin conflicts with other peripherals |

**Status LED meanings**:
- 🔵 Blue: Initializing (stays <2 seconds)
- 🟢 Green: Success (normal operation)
- 🔴 Red: Fatal error (check serial logs)

---

## Step 7: Test Animation Performance (Optional)

### Measure Frame Rate

Add logging to main loop (temporary):
```cpp
int64_t last_time = esp_timer_get_time();
int frame_count = 0;

while (1) {
    // ... render frame ...
    
    frame_count++;
    int64_t now = esp_timer_get_time();
    if ((now - last_time) >= 1000000) {  // 1 second
        ESP_LOGI(TAG, "FPS: %d", frame_count);
        frame_count = 0;
        last_time = now;
    }
}
```

**Expected FPS**: 25-30 FPS (40ms frame time or better)

### Performance Benchmarks

| Clock Speed | Typical FPS | Display Update Time |
|-------------|-------------|---------------------|
| 10 MHz | 28-30 FPS | <0.9 ms |
| 5 MHz | 26-28 FPS | ~1.7 ms |
| 1 MHz | 20-22 FPS | ~8.5 ms |

If FPS < 20, check:
1. SPI clock speed (should be 10 MHz)
2. Rendering complexity (simplify graphics if needed)
3. Task priorities (display should run on high-priority task)

---

## Common Issues & Solutions

### Issue 1: `esp_restart()` Loops

**Symptom**: Device restarts continuously, never reaches display init.

**Cause**: Hardware fault (short circuit, loose wire) or persistent config error.

**Solution**:
1. Power off, check all connections
2. Run `idf.py erase-flash` to clear bad config
3. Rebuild and flash

### Issue 2: Display Shows Garbage/Noise

**Symptom**: Display powered, but shows random pixels or lines.

**Cause**: SPI clock too fast, poor wiring, wrong protocol.

**Solution**:
1. Lower SPI clock to 5 MHz or 1 MHz in menuconfig
2. Use shorter, higher-quality jumper wires
3. Verify display is SPI-capable (not I2C-only model)
4. Check DC pin is not floating (must be actively driven)

### Issue 3: Build Succeeds but Flash Fails

**Symptom**: `idf.py flash` hangs at "Connecting..."

**Cause**: Wrong USB port, driver issue, or board in download mode.

**Solution**:
1. Verify correct port with `ls /dev/tty*`
2. Install CH340/CP210x drivers if needed
3. Hold BOOT button while running `idf.py flash`
4. Check USB cable supports data (not charge-only)

### Issue 4: Status LED Red Immediately

**Symptom**: Firmware boots, LED turns red, system halts.

**Cause**: Display initialization failed (see serial log for details).

**Solution**:
1. Check serial monitor output (`idf.py monitor`)
2. Verify error code (timeout, DMA, invalid pins)
3. Fix root cause per error message
4. Rebuild and reflash

---

## Next Steps

### Customize Pin Assignments
If your hardware uses different GPIOs, simply re-run `idf.py menuconfig`, change the pin numbers, and rebuild. No code changes required.

### Integrate with BotiEyes Library
The SPI integration is transparent to the BotiEyes rendering layer. Existing emotion and animation code works unchanged:

```cpp
#include "BotiEyes.h"

BotiEyes eyes(128, 64);  // Display size
eyes.setEmotion(HAPPY);
eyes.update();           // Renders to framebuffer
// ... framebuffer uploaded via SPI automatically ...
```

### Switch Back to I2C
To revert to I2C protocol:
1. `idf.py menuconfig`
2. Select `OLED communication protocol` → `I2C`
3. Rebuild and reflash

---

## Quick Reference Card

**Key Files**:
- `esp-idf/main/Kconfig.projbuild` - Configuration definitions
- `esp-idf/main/idf_component.yml` - Component dependencies
- `esp-idf/main/display_init.cpp` - SPI initialization code
- `esp-idf/main/main.cpp` - Application entry point

**Key Commands**:
```bash
idf.py menuconfig              # Configure pins
idf.py build                   # Compile firmware
idf.py -p PORT flash           # Upload to device
idf.py -p PORT monitor         # View logs
idf.py -p PORT flash monitor   # Flash + monitor (combined)
idf.py erase-flash             # Clear all stored config
```

**Default Pins** (ESP32-S3):
- MOSI: GPIO11, SCK: GPIO12, CS: GPIO10, DC: GPIO9, RST: GPIO8
- Clock: 10 MHz, Status LED: GPIO38

**Support**:
- Documentation: `/specs/003-ssd1306-spi-component/`
- Hardware reference: `/esp-idf/ssd1306_esp32s3.md`
- Issues: GitHub Issues (botieyes repository)

---

## Success Criteria Checklist

Use this to verify your integration meets specification requirements:

- [ ] **SC-001**: Configured SPI pins via menuconfig in <2 minutes
- [ ] **SC-002**: Display initialized successfully on first attempt
- [ ] **SC-003**: Animations run at ≥25 FPS (check serial logs)
- [ ] **SC-004**: Errors detected and reported within 500ms
- [ ] **SC-005**: No changes to BotiEyes rendering code required
- [ ] **SC-006**: Build time increase <10 seconds vs I2C build

**If all checked**: ✅ Integration complete and validated!

---

**Last Updated**: 2026-06-13 | **Version**: 1.0.0
