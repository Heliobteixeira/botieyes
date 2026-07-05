# Display Configuration Review

**Date**: 2026-07-03  
**Status**: ⚠️ **CRITICAL - Configuration Duplication & Inconsistency**

---

## Executive Summary

The display configuration is currently **duplicated across THREE different Kconfig files**, creating confusion, maintenance burden, and potential for mismatched settings. The system works but only by accident - different parts of code reference different config symbols.

---

## Current Configuration Sources

### 1. External SSD1306 Library Kconfig
**Location**: `/Users/helioteixeira/dev/esp-idf-ssd1306/components/ssd1306/Kconfig.projbuild`

**Symbols Defined**:
```kconfig
CONFIG_SPI_INTERFACE=y              # Protocol selection
CONFIG_I2C_INTERFACE=n

CONFIG_MOSI_GPIO=35                 # SPI pins
CONFIG_SCLK_GPIO=36
CONFIG_CS_GPIO=34
CONFIG_DC_GPIO=37
CONFIG_RESET_GPIO=38

CONFIG_SDA_GPIO=21                  # I2C pins
CONFIG_SCL_GPIO=22
```

**Used By**:
- `esp-idf/components/display/src/display_init.cpp` (lines 125-130)
- External nopnop2002/ssd1306 library itself

**Status**: ❌ **Defaults don't match ESP32-S3 wiring** (defaults are for ESP32-S2)

---

### 2. Main Project Kconfig (OLD)
**Location**: `esp-idf/main/Kconfig.projbuild`

**Symbols Defined**:
```kconfig
CONFIG_BOTIEYES_OLED_PROTOCOL_I2C=n
CONFIG_BOTIEYES_OLED_PROTOCOL_SPI=y

CONFIG_BOTIEYES_OLED_MOSI_PIN=11    # SPI pins
CONFIG_BOTIEYES_OLED_SCK_PIN=12
CONFIG_BOTIEYES_OLED_CS_PIN=10
CONFIG_BOTIEYES_OLED_DC_PIN=9       # ⚠️ WRONG! Should be 13
CONFIG_BOTIEYES_OLED_RST_PIN=16

CONFIG_BOTIEYES_OLED_SDA_PIN=4      # I2C pins
CONFIG_BOTIEYES_OLED_SCL_PIN=15
CONFIG_BOTIEYES_OLED_I2C_ADDRESS=0x3C

CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ=10000000
```

**Used By**:
- ❌ **NOTHING** - These symbols are defined but never referenced in code!
- Main has since moved to logging-only usage of protocol choice

**Status**: ❌ **OBSOLETE - Should be removed**

---

### 3. HAL Board Kconfig (NEW)
**Location**: `esp-idf/components/hal_board/Kconfig`

**Symbols Defined**:
```kconfig
CONFIG_BOTIEYES_BOARD_ESP32S3_SPI=y
CONFIG_BOTIEYES_BOARD_TTGO_LORA32=n

CONFIG_BOTIEYES_OLED_PROTOCOL_I2C=n
CONFIG_BOTIEYES_OLED_PROTOCOL_SPI=y

CONFIG_BOTIEYES_OLED_SPI_MOSI=11    # SPI pins
CONFIG_BOTIEYES_OLED_SPI_SCK=12
CONFIG_BOTIEYES_OLED_SPI_CS=10
CONFIG_BOTIEYES_OLED_SPI_DC=13      # ✅ CORRECT
CONFIG_BOTIEYES_OLED_SPI_RST=14
CONFIG_BOTIEYES_OLED_SPI_FREQ=10000000

CONFIG_BOTIEYES_OLED_I2C_SDA=4      # I2C pins
CONFIG_BOTIEYES_OLED_I2C_SCL=15
CONFIG_BOTIEYES_OLED_I2C_RST=16
CONFIG_BOTIEYES_OLED_I2C_FREQ=400000
CONFIG_BOTIEYES_OLED_I2C_ADDRESS=0x3C

CONFIG_BOTIEYES_LED_TYPE_WS2812=y
CONFIG_BOTIEYES_LED_WS2812_GPIO=38
```

**Used By**:
- `esp-idf/components/hal_board/src/hal_display_spi.c` (STUB - not yet implemented)
- `esp-idf/components/hal_board/src/hal_display_i2c.c` (STUB - not yet implemented)
- `esp-idf/main/main.cpp` for logging board type

**Status**: ⚠️ **DEFINED BUT NOT ACTIVELY USED** - HAL display implementation incomplete

---

## Active Configuration (sdkconfig)

Current ESP32-S3 build uses **ALL THREE simultaneously**:

```ini
# From External ssd1306 Library (ACTIVE)
CONFIG_SPI_INTERFACE=y
CONFIG_MOSI_GPIO=35      # ⚠️ WRONG! Defaults to ESP32-S2
CONFIG_SCLK_GPIO=36      # ⚠️ WRONG!
CONFIG_CS_GPIO=34        # ⚠️ WRONG!
CONFIG_DC_GPIO=37        # ⚠️ WRONG!
CONFIG_RESET_GPIO=38     # ⚠️ WRONG!

# From Main Kconfig (IGNORED)
CONFIG_BOTIEYES_OLED_PROTOCOL_SPI=y
CONFIG_BOTIEYES_OLED_MOSI_PIN=11
CONFIG_BOTIEYES_OLED_SCK_PIN=12
CONFIG_BOTIEYES_OLED_CS_PIN=10
CONFIG_BOTIEYES_OLED_DC_PIN=9    # ⚠️ WRONG!
CONFIG_BOTIEYES_OLED_RST_PIN=16  # ⚠️ WRONG!
CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ=10000000

# From HAL Board Kconfig (PARTIALLY USED)
CONFIG_BOTIEYES_BOARD_ESP32S3_SPI=y
CONFIG_BOTIEYES_OLED_SPI_MOSI=11
CONFIG_BOTIEYES_OLED_SPI_SCK=12
CONFIG_BOTIEYES_OLED_SPI_CS=10
CONFIG_BOTIEYES_OLED_SPI_DC=13   # ✅ CORRECT
CONFIG_BOTIEYES_OLED_SPI_RST=14  # ✅ CORRECT
CONFIG_BOTIEYES_OLED_SPI_FREQ=10000000
```

---

## Problems Identified

### 🔴 Problem 1: Configuration Duplication

**Three Kconfig files define overlapping display settings**:
- External library defines `CONFIG_MOSI_GPIO`
- Main defines `CONFIG_BOTIEYES_OLED_MOSI_PIN`
- HAL Board defines `CONFIG_BOTIEYES_OLED_SPI_MOSI`

**Impact**: Confusion about which config to use, maintenance burden

---

### 🔴 Problem 2: Wrong Pin Assignments in Active Config

The external ssd1306 library's Kconfig defaults to **ESP32-S2 pins**, but we're building for **ESP32-S3**:

| Pin | External Lib Default | Actual ESP32-S3 Wiring | Status |
|-----|---------------------|------------------------|--------|
| MOSI | GPIO35 | GPIO11 | ❌ WRONG |
| SCLK | GPIO36 | GPIO12 | ❌ WRONG |
| CS | GPIO34 | GPIO10 | ❌ WRONG |
| DC | GPIO37 | GPIO13 | ❌ WRONG |
| RST | GPIO38 | GPIO14 | ❌ WRONG |

**Why It Works**: The `display_init.cpp` reads from external lib configs but those happen to be overridden in sdkconfig (source unknown - possibly manual edit or menuconfig session).

---

### 🔴 Problem 3: Obsolete Main Kconfig

`main/Kconfig.projbuild` defines display configs that **no code references**. These should have been removed when HAL Board component was created.

**Obsolete symbols**:
- `CONFIG_BOTIEYES_OLED_MOSI_PIN` (nothing uses this)
- `CONFIG_BOTIEYES_OLED_SCK_PIN` (nothing uses this)
- `CONFIG_BOTIEYES_OLED_CS_PIN` (nothing uses this)
- `CONFIG_BOTIEYES_OLED_DC_PIN` (nothing uses this - and has WRONG value!)
- `CONFIG_BOTIEYES_OLED_RST_PIN` (nothing uses this)

---

### 🔴 Problem 4: Inconsistent Naming

Three different naming schemes exist:

| Concept | External Lib | Main (OLD) | HAL Board (NEW) |
|---------|-------------|-----------|----------------|
| SPI MOSI | `CONFIG_MOSI_GPIO` | `CONFIG_BOTIEYES_OLED_MOSI_PIN` | `CONFIG_BOTIEYES_OLED_SPI_MOSI` |
| SPI SCK | `CONFIG_SCLK_GPIO` | `CONFIG_BOTIEYES_OLED_SCK_PIN` | `CONFIG_BOTIEYES_OLED_SPI_SCK` |
| Protocol | `CONFIG_SPI_INTERFACE` | `CONFIG_BOTIEYES_OLED_PROTOCOL_SPI` | `CONFIG_BOTIEYES_OLED_PROTOCOL_SPI` |

---

## Correct ESP32-S3 Wiring

Based on `sdkconfig.defaults` and hardware documentation:

### SPI Display Pins
```
MOSI (Data) = GPIO11
SCK (Clock) = GPIO12
CS (Chip Select) = GPIO10
DC (Data/Command) = GPIO13
RST (Reset) = GPIO14
Frequency = 10 MHz
```

### Status LED
```
WS2812 LED = GPIO38
```

---

## Recommendations

### 🎯 **Immediate Fix: Update External Library Defaults**

Since we control the external ssd1306 library path, update its Kconfig defaults:

**File**: `/Users/helioteixeira/dev/esp-idf-ssd1306/components/ssd1306/Kconfig.projbuild`

```kconfig
config MOSI_GPIO
    depends on SPI_INTERFACE
    int "MOSI GPIO number"
    range 0 SOC_GPIO_OUT_RANGE_MAX
    default 23 if IDF_TARGET_ESP32
    default 11 if IDF_TARGET_ESP32S3  # ✅ ADD THIS
    default 35 if IDF_TARGET_ESP32S2
    default  1 # C3 and others

config SCLK_GPIO
    depends on SPI_INTERFACE
    int "SCLK GPIO number"
    range 0 SOC_GPIO_OUT_RANGE_MAX
    default 18 if IDF_TARGET_ESP32
    default 12 if IDF_TARGET_ESP32S3  # ✅ ADD THIS
    default 36 if IDF_TARGET_ESP32S2
    default  2 # C3 and others

config CS_GPIO
    depends on SPI_INTERFACE
    int "CS GPIO number"
    range 0 SOC_GPIO_OUT_RANGE_MAX
    default  5 if IDF_TARGET_ESP32
    default 10 if IDF_TARGET_ESP32S3  # ✅ ADD THIS
    default 34 if IDF_TARGET_ESP32S2
    default 10 # C3 and others

config DC_GPIO
    depends on SPI_INTERFACE
    int "DC GPIO number"
    range 0 SOC_GPIO_OUT_RANGE_MAX
    default  4 if IDF_TARGET_ESP32
    default 13 if IDF_TARGET_ESP32S3  # ✅ ADD THIS
    default 37 if IDF_TARGET_ESP32S2
    default  3 # C3 and others

config RESET_GPIO
    int "RESET GPIO number"
    range -1 SOC_GPIO_OUT_RANGE_MAX
    default 15 if IDF_TARGET_ESP32
    default 14 if IDF_TARGET_ESP32S3  # ✅ ADD THIS
    default 38 if IDF_TARGET_ESP32S2
    default  4 # C3 and others
```

---

### 🎯 **Short-Term Fix: Remove Obsolete Main Kconfig**

**File**: `esp-idf/main/Kconfig.projbuild`

**Remove lines 36-115** (all OLED-related config):
- Remove `CONFIG_BOTIEYES_OLED_PROTOCOL_*` choice
- Remove `CONFIG_BOTIEYES_OLED_*_PIN` definitions
- Remove `CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ`

**Keep**:
- WiFi config (lines 3-14)
- UDP port config (line 16-20)
- Status LED pin (lines 22-27)
- Frame interval (lines 29-34)

This eliminates duplicate/conflicting definitions.

---

### 🎯 **Long-Term Solution: Complete HAL Display Implementation**

**Current State**: HAL display drivers are STUBS:
- `hal_display_spi.c` - 42 lines, all TODOs
- `hal_display_i2c.c` - Similar stub status

**Target Architecture**:
```
Application (main.cpp)
    ↓
BotiEyes Library
    ↓
HAL Display Interface (hal_display.h)
    ↓
HAL Display SPI/I2C Implementation
    ↓
ESP-IDF SPI/I2C Drivers
    ↓
Hardware
```

**Remove**:
- `components/display/` (temporary adapter)
- Dependency on external nopnop2002/ssd1306

**Benefits**:
- Single source of truth for display config (HAL Board Kconfig)
- No external library pin configuration conflicts
- Proper abstraction layers per industrial architecture
- Easier to support multiple display types

---

## Migration Path

### Phase 1: Immediate (Current Sprint)
1. ✅ Document current configuration chaos (this file)
2. Update external ssd1306 library Kconfig defaults for ESP32-S3
3. Remove obsolete display config from `main/Kconfig.projbuild`
4. Update `sdkconfig.defaults` to remove redundant settings

### Phase 2: Feature 005 (Next Sprint)
1. Implement `hal_display_spi.c` properly
2. Implement `hal_display_i2c.c` properly
3. Remove `components/display/` temporary adapter
4. Update main.cpp to use HAL display instead of display_init

### Phase 3: Cleanup
1. Remove external ssd1306 library dependency
2. Single Kconfig source: `components/hal_board/Kconfig`
3. Document HAL display API in contracts/

---

## Current Workaround

**Until HAL is complete**, the system works because:

1. `sdkconfig` contains correct ESP32-S3 pin values (manually set at some point)
2. `display_init.cpp` reads from external lib configs (which happen to be correct in sdkconfig)
3. Main and HAL Board Kconfigs are ignored by display initialization
4. Boot logs show correct pins:
   ```
   I (402) APP:INIT: Display: SPI (MOSI=11, SCK=12, CS=10, DC=13)
   ```

**But this is fragile** - a `idf.py fullclean` or `menuconfig` reset would revert to wrong defaults!

---

## Action Items

- [ ] Update external ssd1306 Kconfig with ESP32-S3 defaults
- [ ] Remove obsolete display config from main/Kconfig.projbuild
- [ ] Document HAL display completion as Feature 005
- [ ] Add warning comment in display_init.cpp about temporary nature
- [ ] Update copilot-instructions.md with display config policy

---

## References

- External ssd1306: `/Users/helioteixeira/dev/esp-idf-ssd1306`
- HAL Board Spec: `specs/004-industrial-firmware-architecture/plan.md`
- Display Component: `esp-idf/components/display/README.md`
- Current Pin Wiring: `sdkconfig.defaults`
