# HAL Board Component Fix Plan

## Issue Summary

Build failures discovered after event base linker fixes:

### Issue 1: Config Variable Name Mismatch
**Location**: `esp-idf/components/hal_board/src/hal_board.c:45`
**Error**: 
```
error: 'CONFIG_BOTIEYES_LED_WS2812_PIN' undeclared; did you mean 'CONFIG_BOTIEYES_LED_WS2812_GPIO'?
```
**Root Cause**: Code uses `CONFIG_BOTIEYES_LED_WS2812_PIN` but Kconfig defines `CONFIG_BOTIEYES_LED_WS2812_GPIO`

### Issue 2: LED Strip API Incompatibility
**Location**: `esp-idf/components/hal_board/src/hal_led.cpp:54-56`
**Errors**:
```
error: 'struct led_strip_config_t' has no member named 'led_pixel_format'
error: 'LED_PIXEL_FORMAT_GRB' was not declared in this scope
```
**Root Cause**: Code uses deprecated API. The managed component (`espressif__led_strip`) has different structure:
- Old: `led_pixel_format` field (doesn't exist)
- New: `led_model` (enum) + `color_component_format` (union) fields

## API Migration Details

### Current (Incorrect) Code
```cpp
led_strip_config_t strip_config = {};
strip_config.strip_gpio_num = CONFIG_BOTIEYES_LED_WS2812_GPIO;
strip_config.max_leds = 1;
strip_config.led_pixel_format = LED_PIXEL_FORMAT_GRB;  // ❌ Field doesn't exist
strip_config.led_model = LED_MODEL_WS2812;             // ❌ Missing in current code
```

### Correct API (Per led_strip_types.h)
```cpp
led_strip_config_t strip_config = {
    .strip_gpio_num = CONFIG_BOTIEYES_LED_WS2812_GPIO,
    .max_leds = 1,
    .led_model = LED_MODEL_WS2812,
    .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB,
    .flags = {
        .invert_out = false
    }
};
```

### Available Macros (from led_strip_types.h)
- `LED_STRIP_COLOR_COMPONENT_FMT_GRB` - Standard GRB order (WS2812)
- `LED_STRIP_COLOR_COMPONENT_FMT_RGB` - RGB order
- `LED_STRIP_COLOR_COMPONENT_FMT_GRBW` - GRBW with white channel
- Enums: `LED_MODEL_WS2812`, `LED_MODEL_SK6812`, etc.

## Fix Plan

### Step 1: Fix Config Variable Name in hal_board.c ✅
**File**: `esp-idf/components/hal_board/src/hal_board.c`
**Line**: 45
**Change**: Replace `CONFIG_BOTIEYES_LED_WS2812_PIN` with `CONFIG_BOTIEYES_LED_WS2812_GPIO`
**Status**: COMPLETE

### Step 2: Fix LED Strip Config in hal_led.cpp ✅
**File**: `esp-idf/components/hal_board/src/hal_led.cpp`
**Lines**: 52-56
**Changes**:
1. Update struct initialization to use designated initializers
2. Add `led_model = LED_MODEL_WS2812`
3. Replace `led_pixel_format` with `color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB`
4. Add `flags.invert_out = false`
5. Update `strip_gpio_num` to use correct config variable
6. Add proper RMT config with `clk_src`, `mem_block_symbols`
**Status**: COMPLETE

### Step 3: Remove Legacy Display Code ⚠️ NEW
**File**: `esp-idf/main/CMakeLists.txt`
**Issue**: display_init.cpp has null pointer errors (lines 137, 151, 152)
**Root Cause**: Legacy code not fully migrated to hal_board
**Change**: Remove "display_init.cpp" and "wifi_init.cpp" from SRCS list
**Rationale**: 
- hal_board component now handles display initialization
- wifi_manager component handles WiFi initialization
- These files are no longer used (no references found in main.cpp or app_task.cpp)
- They're causing build errors with warnings-as-errors enabled

## Expected Results

**✅ COMPLETED**:
- hal_board.c config variable naming fixed
- hal_led.cpp LED strip API updated to ESP-IDF v6.0.1
- main.cpp configure_status_led() updated to new LED strip API
- I2C deprecation warnings suppressed

**⚠️ BLOCKED** - External Dependency Issue:
- nopnop2002/ssd1306 library incompatible with ESP-IDF v6.0.1
  - Uses deprecated I2C driver API (i2c_master_bus_handle_t not defined)
  - ESP_IDF_VERSION_VAL macro issue
  - Library location: /Users/helioteixeira/dev/esp-idf-ssd1306/components/ssd1306
  - Referenced in: esp-idf/main/idf_component.yml

**Remaining Work**:
1. **Option A** - Update external ssd1306 library to ESP-IDF v6.0.1 I2C API
2. **Option B** - Replace with compatible managed component
3. **Option C** - Complete hal_display_spi.c implementation (bypasses external dependency)

## Build Status

**Last successful build**: Commit 56a8718 (before ssd1306 integration)
- Binary: botieyes-esp-idf.bin (881 KB)
- Target: ESP32-S3
- Status: Event base fixes complete, firmware functional

**Current blocker**: External ssd1306 library incompatible with ESP-IDF v6.0.1

## Implementation Notes

### Files Modified
1. `components/hal_board/src/hal_board.c` - Fixed WS2812 config variable name
2. `components/hal_board/src/hal_led.cpp` - Updated LED strip API with designated initializers
3. `main/main.cpp` - Updated configure_status_led() to new API, added hal includes
4. `main/CMakeLists.txt` - Temporarily restored display_init.cpp and esp_ssd1306.cpp
5. `sdkconfig` - Added CONFIG_I2C_SUPPRESS_DEPRECATE_WARN=y, set CONFIG_SPI_INTERFACE=y

### Architecture State
- ✅ wifi_manager component: Complete and functional
- ✅ state_machine component: Complete and functional
- ✅ config_manager component: Complete and functional
- ✅ health_monitor component: Complete and functional
- ✅ hal_led: Complete and functional with WS2812 support
- ⚠️ hal_display: API defined, SPI implementation incomplete (stub only)
- ⚠️ Legacy display_init.cpp: Still in use due to hal_display_spi.c incompleteness
- ⚠️ External ssd1306: Incompatible with ESP-IDF v6.0.1

## Implementation Time Estimate

- Step 1: 2 minutes (simple text replacement)
- Step 2: 5 minutes (struct initialization update)
- Step 3: 1 minute (verify includes)
- Build validation: 3 minutes
- **Total**: ~11 minutes

## Validation Steps

1. Clean build: `rm -rf build`
2. Full rebuild: `idf.py build`
3. Verify binary size unchanged or minimal increase
4. Check for any new warnings
5. Mark T117 fully complete in tasks.md

## References

- LED Strip API: `/managed_components/espressif__led_strip/include/led_strip_types.h`
- Example usage: `~/.espressif/v6.0.1/esp-idf/examples/openthread/ot_common_components/ot_led/ot_led_strip.c`
- Kconfig definition: `esp-idf/components/hal_board/Kconfig` (line 137)
