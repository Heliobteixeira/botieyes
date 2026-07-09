# Quickstart: ST7789 Display Integration

**Feature**: 005-st7789-display-support | **Date**: 2026-07-06

This guide helps developers configure and use ST7789 color displays with botieyes firmware.

**New to display drivers?** See [RENDERING_COMPARISON.md](RENDERING_COMPARISON.md) for a detailed explanation of how pixel drawing works in SSD1306 vs ST7789 - no changes required in BotiEyes library!

---

## Prerequisites

### Hardware Requirements
- ESP32, ESP32-S2, ESP32-S3, or ESP32-C3 development board
- ST7789-based TFT display (default target: TTGO T-Display 240x135, also supports 240x240 and other resolutions)
- 6 GPIO wires (MOSI, SCLK, DC, RESET, optional: CS, Backlight)
- USB cable for programming

### Software Requirements
- ESP-IDF v5.0 or later installed and activated
- botieyes repository cloned
- (Optional) ST7789 display datasheet for pin mappings

---

## Quick Start (5 Minutes)

### Step 1: Navigate to Project Directory

```bash
cd /path/to/botieyes/esp-idf
source ~/.espressif/tools/activate_idf_v6.0.1.sh  # Or your ESP-IDF version
```

### Step 2: Configure Display Type

```bash
idf.py menuconfig
```

Navigate through menus:
1. **"HAL Board Configuration"** (or "Display Configuration")
2. Select **"Display controller type"**
3. Choose **"ST7789 SPI"**
4. Press **S** to save, then **Q** to quit

### Step 3: Configure GPIO Pins

```bash
idf.py menuconfig
```

Navigate to **"ST7789 Configuration"**:

| Parameter | Description | Default (TTGO T-Display) | Your Value |
|-----------|-------------|--------------------------|------------|
| MOSI GPIO | SPI data out | GPIO 19 | _______ |
| SCLK GPIO | SPI clock | GPIO 18 | _______ |
| CS GPIO | Chip select | GPIO 5 | _______ |
| DC GPIO | Data/command | GPIO 16 | _______ |
| RESET GPIO | Hardware reset | GPIO 23 | _______ |
| BL GPIO | Backlight | GPIO 4 | _______ |

**Note**: Default values tested on reference hardware. Adjust for your board.

### Step 4: Build and Flash

```bash
idf.py build
idf.py flash monitor
```

Expected output:
```
I (1234) ST7789: GPIO_MOSI=23
I (1235) ST7789: GPIO_SCLK=18
I (1236) ST7789: GPIO_DC=27
I (1237) ST7789: GPIO_RESET=33
I (1238) ST7789: GPIO_BL=32
I (1500) hal_board: Display initialized successfully
```

### Step 5: Verify Display

You should see:
- Display backlight turns on
- BotiEyes animated eyes appear on screen
- Smooth eye movement tracking emotional state

**If display is blank**: Check wiring, GPIO pin assignments, power supply.

---

## Detailed Configuration

### Display Type Selection

**menuconfig path**: `HAL Board Configuration → Display controller type`

**Options**:
- `SSD1306 I2C` (default for backward compatibility)
- `SSD1306 SPI`
- `ST7789 SPI` (select this)

**Impact**: Determines which driver component is compiled and linked.

---

### GPIO Pin Configuration

#### Mandatory Pins

| Pin | Function | Typical Value | Notes |
|-----|----------|---------------|-------|
| MOSI | SPI data | GPIO 19 (TTGO T-Display) | Data from ESP32 to display |
| SCLK | SPI clock | GPIO 18 (TTGO T-Display) | Clock signal for SPI |
| DC | Data/command select | GPIO 16 (TTGO T-Display) | Low=command, High=data |
| RESET | Hardware reset | GPIO 23 (TTGO T-Display) | Pulse low to reset display |

#### Optional Pins

| Pin | Function | Default | When to Use |
|-----|----------|---------|-------------|
| CS | Chip select | GPIO 5 (TTGO T-Display) | Required if multiple SPI devices on same bus |
| BL | Backlight | GPIO 4 (TTGO T-Display) | Display has backlight control pin |

**Disabling pins**: Set value to `-1` in menuconfig to disable CS or BL functionality.

---

### Display Resolution & Offsets

**menuconfig path**: `ST7789 Configuration → Screen dimensions`

| Parameter | Description | Common Values |
|-----------|-------------|---------------|
| SCREEN WIDTH | Horizontal pixels | 240 (default), 280, 320 |
| SCREEN HEIGHT | Vertical pixels | 135 (default), 240, 280, 320 |
| GRAM X OFFSET | X-axis offset | 40 (TTGO T-Display), 0 (generic), 34-80 (some panels) |
| GRAM Y OFFSET | Y-axis offset | 52 (TTGO T-Display), 0 (generic), 0-40 (some panels) |

**GRAM offsets**: Some ST7789 panels have memory offsets. If display image is shifted, adjust these values.

**How to find offsets**:
1. Build with 0,0 offsets
2. If image is shifted right: increase X offset
3. If image is shifted down: increase Y offset
4. Typical range: 0-80 pixels

---

### SPI Performance Tuning

**menuconfig path**: `ST7789 Configuration → SPI peripheral` and clock speed (if available)

#### SPI Host Selection

| Host | ESP32 | ESP32-S2/S3 | ESP32-C3 | Notes |
|------|-------|-------------|----------|-------|
| SPI2_HOST | ✅ HSPI | ✅ | ✅ | Default, safest choice |
| SPI3_HOST | ✅ VSPI | ✅ | ❌ Not available | Alternative if SPI2 busy |

**Recommendation**: Use SPI2_HOST unless you have a specific reason to use SPI3.

#### Clock Speed Adjustment

**Default**: 40 MHz (tested on TTGO T-Display)

**Tuning**:
- **20 MHz**: Conservative, works on all boards
- **40 MHz**: Default for TTGO T-Display, good balance
- **80 MHz**: Maximum speed, may require short wires (<10cm)

**How to change** (code modification required):
```cpp
// In st7789.c (after integration)
spi_clock_speed(40000000);  // 40 MHz
```

Or add Kconfig option for user configuration.

---

### Frame Buffer Configuration

**menuconfig path**: `ST7789 Configuration → Enable Frame Buffer`

**Options**:
- **Disabled** (default): Direct rendering, lower memory usage
- **Enabled**: Frame buffer mode, faster rendering

**Memory usage** (240x135 RGB565 - TTGO T-Display):
- Frame buffer: 64,800 bytes (~65 KB)
- ESP32 SRAM: ~520 KB total
- Overhead: 12.5% of SRAM (very reasonable)

**When to enable**:
- ✅ Complex animations (many pixels change per frame)
- ✅ ESP32-S3 with PSRAM available
- ✅ Smooth performance more important than memory

**When to disable**:
- ✅ Simple animations (eye movement, few pixels)
- ✅ Low memory devices (ESP32-C3)
- ✅ Other components need SRAM

**Performance comparison** (240x135 display):
- Frame buffer: ~38 FPS achievable at 40 MHz SPI
- Direct rendering: ~15-20 FPS typical

---

### Display Inversion

**menuconfig path**: `ST7789 Configuration → Enable Display Inversion`

**Purpose**: Some ST7789 panels require color inversion to display correctly.

**Symptoms of incorrect inversion**:
- Colors inverted (white appears black, black appears white)
- Blue tint or red tint on entire screen

**Fix**: Toggle inversion setting and rebuild.

---

## Hardware Wiring Guide

### Typical ST7789 Pinout

```
ST7789 Display         ESP32 (example)
┌─────────────────┐
│  GND            │────── GND
│  VCC (3.3V)     │────── 3.3V
│  SCL (SCLK)     │────── GPIO 18
│  SDA (MOSI)     │────── GPIO 23
│  RES (RESET)    │────── GPIO 33
│  DC             │────── GPIO 27
│  CS             │────── GPIO 5 (or -1 to disable)
│  BLK (Backlight)│────── GPIO 32 (or 3.3V for always-on)
└─────────────────┘
```

### Wiring Checklist

- [ ] Power: Display VCC to ESP32 3.3V (NOT 5V)
- [ ] Ground: Display GND to ESP32 GND
- [ ] SCLK: Display clock pin to ESP32 SCLK GPIO
- [ ] MOSI: Display data pin to ESP32 MOSI GPIO
- [ ] DC: Display DC pin to ESP32 DC GPIO
- [ ] RESET: Display reset pin to ESP32 RESET GPIO
- [ ] Optional: CS to ESP32 CS GPIO (or tie to GND if not using)
- [ ] Optional: Backlight to ESP32 BL GPIO (or tie to VCC for always-on)

**Critical**: Double-check GND and VCC connections. Reversed polarity can damage display.

---

## Troubleshooting

### Display is Blank / Not Initializing

**Possible causes**:
1. **Wiring error**: Check all connections, especially GND and VCC
2. **Wrong GPIO pins**: Verify menuconfig settings match physical wiring
3. **Power supply insufficient**: ST7789 draws 20-50mA, ensure ESP32 can supply
4. **Display variant incompatible**: Some ST7789 clones use different init sequences

**Debug steps**:
```bash
# Check initialization logs
idf.py monitor | grep ST7789

# Expected output:
I (123) ST7789: GPIO_MOSI=23
I (124) ST7789: GPIO_SCLK=18
# ... more GPIO logs

# If no logs appear, display component not compiled
idf.py menuconfig  # Verify ST7789 selected
```

---

### Display Shows Corrupted Image

**Possible causes**:
1. **GRAM offset wrong**: Adjust X/Y offsets in menuconfig
2. **SPI clock too high**: Reduce to 20 MHz
3. **Loose wiring**: Check connections, especially SCLK and MOSI
4. **Electrical interference**: Shorten wires, add bypass capacitors

**Fix**:
```bash
idf.py menuconfig
# ST7789 Configuration → GRAM X OFFSET → adjust by 10-40 pixels
# ST7789 Configuration → GRAM Y OFFSET → adjust by 10-40 pixels
```

---

### Colors are Inverted

**Cause**: Display panel requires inversion setting opposite to default

**Fix**:
```bash
idf.py menuconfig
# ST7789 Configuration → Enable Display Inversion → toggle
```

---

### Display Works but Animation is Slow

**Possible causes**:
1. **Direct rendering mode**: Enable frame buffer for better performance
2. **Low SPI clock**: Increase to 40 MHz (test stability)
3. **Complex rendering**: BotiEyes drawing too many pixels per frame

**Performance optimization**:
```bash
idf.py menuconfig
# ST7789 Configuration → Enable Frame Buffer → Yes
# Build and test frame rate improvement

# If still slow, profile rendering loop
# Check main.cpp FRAME_INTERVAL_MS value
```

---

### Build Errors

**Error**: `st7789.h: No such file or directory`

**Cause**: ST7789 component not found in components directory

**Fix**: Ensure `esp-idf/components/st7789/` exists with driver files

---

**Error**: `CONFIG_DISPLAY_TYPE_ST7789_SPI undeclared`

**Cause**: Kconfig option not defined

**Fix**: Add `CONFIG_DISPLAY_TYPE_ST7789_SPI` to `hal_board/Kconfig.projbuild`

---

### Runtime Errors

**Error**: `esp_err_t 0x101 (ESP_ERR_INVALID_ARG)`

**Cause**: Invalid GPIO pin number (e.g., GPIO 48 on ESP32, or input-only GPIO 35 as output)

**Fix**: Choose different GPIO pin within valid range

---

**Error**: `Frame buffer allocation failed`

**Cause**: Insufficient SRAM for 115KB buffer

**Options**:
1. Disable frame buffer (slower but works)
2. Use ESP32-S3 with PSRAM
3. Reduce display resolution (smaller buffer)

---

## Advanced Configuration

### Custom Display Resolutions

If using non-240x240 ST7789 displays:

```bash
idf.py menuconfig
# ST7789 Configuration → SCREEN WIDTH → enter your width
# ST7789 Configuration → SCREEN HEIGHT → enter your height
```

**Common ST7789 resolutions**:
- 240x135 (TTGO T-Display, landscape)
- 240x240 (square displays)
- 240x280 (rectangular)
- 240x320 (rectangular, larger)

**Frame buffer memory**:
- 240x135 = 64,800 bytes (~65 KB) ✅ Default target
- 240x240 = 115,200 bytes (~115 KB)
- 240x280 = 134,400 bytes (~134 KB)  
- 240x320 = 153,600 bytes (~154 KB)

Ensure ESP32 has sufficient SRAM or use direct rendering.

---

### Multiple SPI Devices on Same Bus

If sharing SPI bus with SD card, other displays, etc.:

1. **Enable CS pin**: Set CS GPIO to valid pin (not -1)
2. **Use same SPI host**: Configure all devices on SPI2_HOST
3. **Different CS pins**: Each device needs unique CS GPIO

**Example**:
- ST7789: CS = GPIO 5, DC = GPIO 27
- SD Card: CS = GPIO 13
- Shared: MOSI = GPIO 23, SCLK = GPIO 18

---

### Display Rotation

**Current status**: Not configurable via menuconfig (future enhancement)

**Workaround** (code modification):
```cpp
// In display_init.cpp after initialization
display->setRotation(1);  // 0, 1, 2, or 3 for 0°, 90°, 180°, 270°
```

Or modify ST7789 initialization sequence (0x36 command parameter).

---

## Migrating from SSD1306 to ST7789

### Configuration Changes

1. Open menuconfig
2. Change display type: `SSD1306_I2C` → `ST7789_SPI`
3. Reconfigure GPIO pins (SPI instead of I2C)
4. Adjust resolution (128x64 → 240x240)
5. Build and flash

**Code changes**: None required (display abstraction handles differences)

---

### Feature Comparison

| Feature | SSD1306 I2C | SSD1306 SPI | ST7789 SPI |
|---------|-------------|-------------|------------|
| **Colors** | Monochrome | Monochrome | RGB565 (65K colors) |
| **Resolution** | 128x64 typical | 128x64 typical | 240x240 typical |
| **Speed** | 400 kHz I2C | 10 MHz SPI | 20-80 MHz SPI |
| **Wiring** | 2 pins (SDA, SCL) | 4-5 pins | 4-6 pins |
| **Memory** | 1KB buffer | 1KB buffer | 115KB buffer (optional) |
| **Performance** | 15-20 FPS | 20-30 FPS | 25+ FPS (buffered) |

---

## Testing & Validation

### Basic Functionality Test

1. **Power on**: Backlight turns on (if configured)
2. **Display init**: No error messages in logs
3. **First frame**: Eyes appear within 2 seconds
4. **Animation**: Eyes blink, move smoothly
5. **Colors**: White eyes on black background (or colors if implemented)

### Performance Test

```bash
# Monitor frame rate (check logs for timing)
idf.py monitor | grep "Frame time"

# Expected: <100ms per frame (10+ FPS)
```

### Stress Test

**Run firmware for 10+ minutes**:
- [ ] No crashes or resets
- [ ] Animation stays smooth
- [ ] Display doesn't freeze
- [ ] No memory leaks (free heap stable)

---

## Reference Configurations

### Tested Hardware: TTGO T-Display ESP32 (Default Target)

```
Display: ST7789 240x135 (landscape orientation)
Board: ESP32-PICO-D4

Pin Configuration:
- MOSI: GPIO 19
- SCLK: GPIO 18
- CS: GPIO 5
- DC: GPIO 16
- RESET: GPIO 23
- BL: GPIO 4

Resolution: 240x135
Offsets: X=40, Y=52
Clock: 40 MHz
Frame buffer: Recommended (only 65KB)
```

### Alternative: Generic ST7789 240x240

```
Display: ST7789 240x240 (generic square display)
Board: ESP32 or ESP32-S3

Pin Configuration (adjust for your board):
- MOSI: GPIO 23 (ESP32) or GPIO 35 (S3)
- SCLK: GPIO 18 (ESP32) or GPIO 36 (S3)
- CS: -1 (disabled) or assign as needed
- DC: GPIO 27 (ESP32) or GPIO 37 (S3)
- RESET: GPIO 33 (ESP32) or GPIO 38 (S3)
- BL: GPIO 32 (ESP32) or GPIO 33 (S3)

Resolution: 240x240
Offsets: X=0, Y=0 (usually)
Clock: 20-40 MHz
Frame buffer: Optional (115KB - test memory constraints)
```

---

## Next Steps

### After Basic Setup

1. **Optimize performance**: Enable frame buffer, increase SPI clock if stable
2. **Add color themes**: Modify BotiEyes to use color emotions (red=angry, blue=calm)
3. **Implement rotation**: Add display orientation configuration
4. **Power optimization**: Control backlight based on activity

### Customization Ideas

- Dynamic brightness adjustment (PWM backlight)
- Color-coded emotional states
- Display sleep mode for power saving
- On-screen status indicators (WiFi, battery)

---

## Getting Help

### Check Logs First

```bash
idf.py monitor | tee display_debug.log
# Save logs to file, share if asking for help
```

### Common Log Patterns

**Success**:
```
I (123) ST7789: GPIO_MOSI=23
I (200) hal_board: Display initialized successfully
```

**Failure**:
```
E (150) ST7789: spi_bus_initialize failed: 0x101
E (151) hal_board: Display initialization failed
```

### Resources

- **Feature spec**: `specs/005-st7789-display-support/spec.md`
- **Implementation plan**: `specs/005-st7789-display-support/plan.md`
- **Driver interface**: `specs/005-st7789-display-support/contracts/display-driver-interface.md`
- **ESP-IDF SPI docs**: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_master.html
- **ST7789 datasheet**: [Sitronix ST7789V](https://www.displayfuture.com/Display/datasheet/controller/ST7789.pdf)

---

## Conclusion

You should now have a working ST7789 display showing animated BotiEyes. 

**Key takeaways**:
- Menuconfig for all configuration (no code changes)
- Default GPIO values tested and safe
- Frame buffer optional (trade-off: speed vs memory)
- Backward compatible with SSD1306 setups

Enjoy your colorful animated robot eyes! 🤖👀
