# Contract: Kconfig Configuration API

**Feature**: [spec.md](../spec.md) | **Data Model**: [data-model.md](../data-model.md)
**Date**: 2026-06-13
**Version**: 1.0.0
**Phase**: 1 (Design)

## Overview

This contract defines the build-time configuration interface for SPI display integration via ESP-IDF Kconfig/menuconfig. Developers configure SPI pins and protocol selection through this interface without modifying source code.

**Contract Type**: Build-time configuration interface
**Audience**: Embedded developers using BotiEyes on ESP32-S3
**Stability**: Stable (pins may change per hardware, but interface is fixed)

---

## Configuration Menu Structure

### Menu Hierarchy
```
BotiEyes Display Configuration
├── OLED communication protocol (choice)
│   ├── ○ I2C (default)
│   └── ○ Hardware SPI
├── [I2C pins] (visible only if I2C selected)
│   ├── I2C SDA pin
│   ├── I2C SCL pin
│   └── I2C address
└── [SPI pins] (visible only if SPI selected)
    ├── SPI MOSI pin
    ├── SPI SCK pin
    ├── SPI CS pin
    ├── SPI DC pin
    ├── SPI RST pin
    └── SPI clock speed
```

---

## Configuration Options

### 1. Protocol Selection (Choice)

**Symbol**: `BOTIEYES_OLED_PROTOCOL`
**Type**: `choice` (mutually exclusive radio buttons)
**Required**: Yes (must select exactly one)

#### Option A: I2C (Default)
- **Symbol**: `CONFIG_BOTIEYES_OLED_PROTOCOL_I2C`
- **Enabled by default**: Yes
- **Description**: "Use I2C interface (typical for small OLEDs)"
- **Use case**: Hobbyist projects, breadboard prototyping, fewer wires

#### Option B: Hardware SPI
- **Symbol**: `CONFIG_BOTIEYES_OLED_PROTOCOL_SPI`
- **Enabled by default**: No
- **Description**: "Use hardware SPI interface for faster updates"
- **Use case**: Custom PCBs, performance-critical applications, 25 FPS+ rendering

**Access in code**:
```cpp
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
    // SPI initialization path
#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_I2C)
    // I2C initialization path
#endif
```

---

### 2. SPI MOSI Pin

**Symbol**: `CONFIG_BOTIEYES_OLED_MOSI_PIN`
**Type**: `int`
**Range**: `0` to `48` (ESP32-S3 GPIO count)
**Default**: `11`
**Visibility**: Only when `BOTIEYES_OLED_PROTOCOL_SPI` selected
**Required**: Yes (for SPI mode)

**Description**: "SPI Master Out Slave In (data line)"

**Hardware mapping**: GPIO11 corresponds to pin D1/MOSI/SDA on typical ESP32-S3 dev boards

**Validation**:
- ✅ Value must be in range [0, 48]
- ⚠️ Avoid GPIO6-11 on ESP32-S3 (flash/PSRAM) - not enforced, runtime warning
- ⚠️ Must not conflict with other SPI pins (CS, SCK, DC, RST) - runtime check

**Example menuconfig entry**:
```
(11) OLED SPI MOSI pin
```

---

### 3. SPI SCK Pin

**Symbol**: `CONFIG_BOTIEYES_OLED_SCK_PIN`
**Type**: `int`
**Range**: `0` to `48`
**Default**: `12`
**Visibility**: Only when `BOTIEYES_OLED_PROTOCOL_SPI` selected
**Required**: Yes (for SPI mode)

**Description**: "SPI clock line"

**Hardware mapping**: GPIO12 corresponds to pin D0/SCK/CLK on typical ESP32-S3 dev boards

**Validation**: Same rules as MOSI pin

**Example menuconfig entry**:
```
(12) OLED SPI SCK pin
```

---

### 4. SPI CS Pin

**Symbol**: `CONFIG_BOTIEYES_OLED_CS_PIN`
**Type**: `int`
**Range**: `-1` to `48` (allows `-1` for disabled)
**Default**: `10`
**Visibility**: Only when `BOTIEYES_OLED_PROTOCOL_SPI` selected
**Required**: Optional (can be `-1` if CS is tied low in hardware)

**Description**: "SPI chip select pin (active low). Set to -1 if not used."

**Hardware mapping**: GPIO10 on ESP32-S3

**Special value**: `-1` = CS pin not controlled by firmware (tied to GND on PCB)

**Validation**:
- ✅ Value must be -1 or in range [0, 48]
- ⚠️ If >= 0, must not conflict with other SPI pins

**Example menuconfig entry**:
```
(10) OLED SPI CS pin
```

---

### 5. SPI DC Pin

**Symbol**: `CONFIG_BOTIEYES_OLED_DC_PIN`
**Type**: `int`
**Range**: `0` to `48`
**Default**: `9`
**Visibility**: Only when `BOTIEYES_OLED_PROTOCOL_SPI` selected
**Required**: Yes (for SPI mode)

**Description**: "Data/Command select pin (low=command, high=data)"

**Hardware mapping**: GPIO9 (A0/DC pin on SSD1306 modules)

**Electrical behavior**:
- `LOW (0V)`: Next SPI byte is a command
- `HIGH (3.3V)`: Next SPI byte is data

**Validation**: Same rules as MOSI pin

**Example menuconfig entry**:
```
(9) OLED SPI DC pin
```

---

### 6. SPI RST Pin

**Symbol**: `CONFIG_BOTIEYES_OLED_RST_PIN`
**Type**: `int`
**Range**: `-1` to `48`
**Default**: `8`
**Visibility**: Only when `BOTIEYES_OLED_PROTOCOL_SPI` selected
**Required**: Optional (can be `-1` if RST is tied high)

**Description**: "Reset pin (active low). Set to -1 if tied high in hardware."

**Hardware mapping**: GPIO8 (RES/RST pin on SSD1306 modules)

**Electrical behavior**:
- Pulled `LOW` for 10ms during init to reset display
- Held `HIGH` during normal operation
- If `-1`, firmware skips reset pulse (assumes external pullup)

**Validation**:
- ✅ Value must be -1 or in range [0, 48]
- ⚠️ If >= 0, must not conflict with other SPI pins

**Example menuconfig entry**:
```
(8) OLED SPI RST pin
```

---

### 7. SPI Clock Speed

**Symbol**: `CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ`
**Type**: `int`
**Range**: `100000` to `10000000` (100 kHz to 10 MHz)
**Default**: `10000000` (10 MHz)
**Visibility**: Only when `BOTIEYES_OLED_PROTOCOL_SPI` selected
**Required**: Yes (for SPI mode)

**Description**: "SPI clock frequency in Hz. SSD1306 datasheet maximum: 10 MHz."

**Performance impact**:
| Clock Speed | Frame Transfer Time | Notes |
|-------------|---------------------|-------|
| 10 MHz | 0.82 ms | Maximum performance (recommended) |
| 5 MHz | 1.64 ms | Use for long wires or breadboards |
| 1 MHz | 8.2 ms | Conservative, high noise immunity |
| 100 kHz | 82 ms | Minimum, for debugging only |

**Validation**:
- ✅ Value must be in range [100000, 10000000]
- ⚠️ Values > 10 MHz may violate SSD1306 datasheet specs
- ⚠️ Low values (<1 MHz) may cause visible lag in animations

**Example menuconfig entry**:
```
(10000000) SPI clock speed (Hz)
```

---

## Code Access Pattern

### Reading Configuration Values

**Include header**:
```cpp
#include "sdkconfig.h"
```

**Access macros**:
```cpp
// Protocol selection (boolean)
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
    printf("SPI mode selected\n");
#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_I2C)
    printf("I2C mode selected\n");
#endif

// SPI pin values (integers)
int mosi = CONFIG_BOTIEYES_OLED_MOSI_PIN;    // 11
int sck = CONFIG_BOTIEYES_OLED_SCK_PIN;      // 12
int cs = CONFIG_BOTIEYES_OLED_CS_PIN;        // 10 or -1
int dc = CONFIG_BOTIEYES_OLED_DC_PIN;        // 9
int rst = CONFIG_BOTIEYES_OLED_RST_PIN;      // 8 or -1
int clock = CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ;  // 10000000
```

### Conditional Compilation Example

```cpp
void init_display() {
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
    spi_clock_speed(CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ);
    spi_master_init(&dev,
        CONFIG_BOTIEYES_OLED_MOSI_PIN,
        CONFIG_BOTIEYES_OLED_SCK_PIN,
        CONFIG_BOTIEYES_OLED_CS_PIN,
        CONFIG_BOTIEYES_OLED_DC_PIN,
        CONFIG_BOTIEYES_OLED_RST_PIN);
#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_I2C)
    i2c_master_init(&dev,
        CONFIG_BOTIEYES_OLED_SDA_PIN,
        CONFIG_BOTIEYES_OLED_SCL_PIN,
        CONFIG_BOTIEYES_OLED_I2C_ADDRESS);
#else
    #error "OLED protocol not configured"
#endif
    
    esp_err_t err = ssd1306_init(&dev, 128, 64);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Display init failed: %s", esp_err_to_name(err));
    }
}
```

---

## Developer Workflow

### 1. Configure via menuconfig

```bash
cd esp-idf
idf.py menuconfig
```

**Navigation**:
1. Arrow down to "BotiEyes Display Configuration"
2. Press Enter
3. Select "OLED communication protocol" → choose "Hardware SPI"
4. Verify/adjust pin numbers (use defaults if matching hardware)
5. Adjust "SPI clock speed" if needed (default 10 MHz is recommended)
6. Press `Q` to quit, `Y` to save

### 2. Build with new configuration

```bash
idf.py build
```

Configuration is baked into firmware via `sdkconfig.h`.

### 3. Flash and monitor

```bash
idf.py flash monitor
```

Initialization logs will show:
```
I (123) display_init: Initializing SSD1306 SPI: MOSI=11, SCK=12, CS=10, DC=9
I (145) display_init: SSD1306 init completed in 18 ms
I (150) display_init: SSD1306 SPI initialization successful
```

### 4. Error scenarios

**Invalid pin configuration**:
```
E (123) display_init: SPI bus init failed: Invalid pin configuration
E (125) display_init:   Check: MOSI=11, SCK=12 are valid GPIO pins
```

**Timeout**:
```
E (2123) display_init: SSD1306 init timeout after 2000 ms
E (2125) display_init: FATAL: Display initialization failed
```

---

## Validation & Testing

### Pre-Build Validation
- ✅ Kconfig range constraints prevent invalid values at menuconfig time
- ✅ Conditional visibility prevents showing SPI options when I2C selected
- ⚠️ Pin conflict detection NOT enforced at build time (runtime only)

### Runtime Validation

**Implemented checks**:
1. GPIO pin conflicts detected by `gpio_config()` → `ESP_ERR_INVALID_ARG`
2. SPI bus initialization validates pin assignments → `ESP_ERR_INVALID_ARG`
3. Timeout detection if display doesn't respond within 2 seconds
4. DMA allocation failure detected → `ESP_ERR_NO_MEM`

**Not validated** (developer responsibility):
- Electrical compatibility (3.3V logic levels)
- Physical wiring correctness
- Display power supply adequacy

### Contract Compliance Test Cases

| Test Case | Expected Behavior |
|-----------|-------------------|
| Default SPI config | Builds successfully, initializes on matching hardware |
| MOSI=50 (out of range) | Rejected at menuconfig, cannot save |
| CS=-1 (disabled) | Builds successfully, SPI works without CS control |
| Clock=100 kHz | Builds, slow but functional (82ms/frame) |
| Clock=20 MHz (exceeds 10 MHz limit) | Builds, likely causes display corruption |
| MOSI=SCK (conflict) | Builds, fails at runtime with `ESP_ERR_INVALID_ARG` |

---

## Versioning & Compatibility

**Current Version**: 1.0.0

**Backward Compatibility**:
- Adding new pins (e.g., BUSY signal): Minor version bump (1.1.0)
- Changing default values: Patch version bump (1.0.1)
- Removing pins or changing types: Major version bump (2.0.0)

**ESP-IDF Compatibility**:
- Minimum version: ESP-IDF 5.0 (idf_component.yml support)
- Tested version: ESP-IDF 6.0.1
- Maximum version: No known upper limit

**Hardware Compatibility**:
- Primary target: ESP32-S3 (GPIO0-48)
- Secondary targets: ESP32, ESP32-C3, ESP32-S2 (adjust GPIO ranges in Kconfig)

---

## Contract Summary

**Stability**: ✅ **STABLE** - Interface is frozen for 1.x versions

**Required Actions for Developers**:
1. Run `idf.py menuconfig`
2. Select SPI protocol
3. Verify/adjust pin assignments
4. Build and flash

**Guaranteed Behaviors**:
1. Configuration changes require rebuild (no runtime switching)
2. Invalid menuconfig values are rejected at save time
3. Pin conflicts detected at runtime with clear error messages
4. Initialization timeout triggers after exactly 2 seconds
5. Display buffer format identical for I2C and SPI (rendering code unchanged)

**Breaking Changes Prohibited** (until 2.0.0):
- Cannot remove existing CONFIG_* symbols
- Cannot change Kconfig symbol names
- Cannot change integer ranges (can only expand)
- Cannot make optional pins mandatory

**Contract Compliance**: All requirements FR-001 to FR-016 are met through this configuration interface.
