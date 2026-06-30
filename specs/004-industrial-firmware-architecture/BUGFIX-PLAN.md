# Build Integration Fix Plan

**Feature**: 004-industrial-firmware-architecture  
**Issue**: Linker errors preventing firmware build  
**Status**: ❌ Build fails with multiple definition errors  
**Created**: 2026-06-30

---

## 🔴 Critical Issues Identified

### Issue #1: Multiple Definition of Event Bases (LINKER ERROR)

**Error Message**:
```
multiple definition of `APP_STATE_EVENT'
multiple definition of `WIFI_MGR_EVENT'
```

**Root Cause**:
Event bases are defined in **two locations**:
1. ✅ **Correct**: Component implementation files
   - `components/state_machine/src/app_state.c:25` → `ESP_EVENT_DEFINE_BASE(APP_STATE_EVENT)`
   - `components/wifi_manager/src/wifi_manager.c:53` → `ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT)`

2. ❌ **Incorrect**: Main application file (duplicate)
   - `main/main.cpp:57` → `ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT)`
   - `main/main.cpp:58` → `ESP_EVENT_DEFINE_BASE(APP_STATE_EVENT)`

**ESP-IDF Event Base Rules**:
- `ESP_EVENT_DEFINE_BASE(X)` → **Define once** in the owning component's `.c` file
- `ESP_EVENT_DECLARE_BASE(X)` → **Declare** in public headers (already done)
- Other files → Just `#include` the header (declaration is sufficient)

**Impact**: Build completely blocked (linker fails)

---

## 📋 Correction Plan

### Phase 1: Fix Event Base Definitions (CRITICAL)

**Task 1.1**: Remove duplicate event base definitions from main.cpp
- **File**: `esp-idf/main/main.cpp` lines 57-58
- **Action**: Remove `ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT)` and `ESP_EVENT_DEFINE_BASE(APP_STATE_EVENT)`
- **Reason**: Already defined in component files (wifi_manager.c, app_state.c)
- **Alternative**: Since main.cpp already includes the headers, the declarations are available

**Task 1.2**: Verify header includes in main.cpp
- **File**: `esp-idf/main/main.cpp`
- **Check**: Ensure `#include "wifi_manager.h"` and `#include "app_state.h"` are present
- **Verify**: Declarations `ESP_EVENT_DECLARE_BASE(...)` exist in headers

**Expected Result**: Build progresses past linker stage

---

### Phase 2: Verify Component Registration

**Task 2.1**: Check botieyes component CMakeLists.txt
- **File**: `esp-idf/components/botieyes/CMakeLists.txt`
- **Status**: ✅ **VERIFIED** - Component properly registered
- **Sources**: 12 BotiEyes library files (BotiEyes.cpp, EmotionMapper.cpp, network layer, etc.)
- **Path**: References `../../../BotiEyes/src` (original library location)
- **Note**: Component wrapper pattern used (no file duplication)

**Task 2.2**: Verify main CMakeLists.txt dependencies
- **File**: `esp-idf/main/CMakeLists.txt`
- **Status**: ✅ **VERIFIED** - All dependencies declared
- **PRIV_REQUIRES**: botieyes, wifi_manager, state_machine, config_manager, health_monitor, hal_board
- **Additional**: led_strip, driver, esp_driver_gpio, esp_wifi, nvs_flash, esp_event, esp_netif
- **Note**: Dependency chain is complete

---

### Phase 3: Build System Validation

**Task 3.1**: Clean build for TTGO LoRa32 (ESP32)
```bash
cd esp-idf
rm -rf build sdkconfig
cp sdkconfig.defaults.ttgo_lora32 sdkconfig.defaults
idf.py set-target esp32
idf.py build
```

**Task 3.2**: Clean build for ESP32-S3 SPI variant
```bash
cd esp-idf
rm -rf build sdkconfig
cp sdkconfig.defaults.esp32s3_spi sdkconfig.defaults
idf.py set-target esp32s3
idf.py build
```

**Success Criteria**:
- ✅ No compilation errors
- ✅ No linker errors
- ✅ Firmware binaries generated: `build/botieyes-esp-idf.bin`
- ✅ Size within flash constraints

---

### Phase 4: Code Quality Checks (Post-Build)

**Task 4.1**: Remove any remaining legacy code
- Search for `BotiEyes::wifi` namespace references (should be none after cleanup)
- Search for old display API calls (should be none after cleanup)
- Verify all TODOs are properly documented

**Task 4.2**: Run static analysis
```bash
cd esp-idf
idf.py app
# Check warnings in build log
```

---

## 🔧 Implementation Steps (Ordered)

### Step 1: Fix Event Base Definitions (5 minutes)
1. Open `esp-idf/main/main.cpp`
2. Locate lines 57-58 (event base definitions)
3. Remove both `ESP_EVENT_DEFINE_BASE` lines
4. Add comment: `// Event bases defined in component files (wifi_manager.c, app_state.c)`
5. Save file

### Step 2: Verify Component Structure (5 minutes)
1. Check `esp-idf/components/botieyes/CMakeLists.txt` exists
2. Verify it contains `idf_component_register(SRCS ...)`
3. Check `esp-idf/main/CMakeLists.txt` REQUIRES list

### Step 3: Test Build (10 minutes)
1. Clean build directory: `rm -rf esp-idf/build`
2. Run build: `cd esp-idf && idf.py build`
3. Monitor for errors
4. Check binary generation

### Step 4: Validate Both Targets (10 minutes)
1. Test ESP32 (TTGO LoRa32) build
2. Test ESP32-S3 (SPI variant) build
3. Compare binary sizes
4. Document results

---

## ✅ Success Criteria

- [ ] No linker errors (multiple definition)
- [ ] No compilation errors
- [ ] Firmware builds for ESP32 target
- [ ] Firmware builds for ESP32-S3 target
- [ ] Binary size reasonable (< 1MB)
- [ ] All component dependencies resolved
- [ ] Build completes in < 2 minutes

---

## 🚨 Rollback Plan

If fixes introduce new issues:
1. Revert to commit `7360ef1`
2. Apply fixes incrementally
3. Test after each change

---

## 📊 Estimated Time

- **Phase 1** (Event Base Fix): 5 minutes
- **Phase 2** (Component Verification): 5 minutes  
- **Phase 3** (Build Validation): 20 minutes
- **Phase 4** (Code Quality): 10 minutes

**Total**: ~40 minutes

---

## 🎯 Priority

**P0 (Immediate)**: Phase 1 - Blocks all builds  
**P1 (High)**: Phase 2 - May reveal additional issues  
**P1 (High)**: Phase 3 - Validates both targets  
**P2 (Medium)**: Phase 4 - Quality improvements

---

## 📝 Notes

- Event base definitions follow ESP-IDF conventions (define once, declare in headers)
- Components are the source of truth for their event bases
- Main application only consumes events (uses declarations from headers)
- This is a textbook multiple definition linker error - straightforward to fix

---

## 🔗 References

- ESP-IDF Event Loop: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html
- Component Build System: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html
- Task T117: specs/004-industrial-firmware-architecture/tasks.md
