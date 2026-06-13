# Tasks: SSD1306 SPI Component Integration

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)
**Input**: Design documents from `/specs/003-ssd1306-spi-component/`
**Date**: 2026-06-13

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2, US3)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and documentation foundation

- [x] T001 Feature branch created (`003-ssd1306-spi-component`)
- [x] T002 Specification validated ([spec.md](spec.md) with 16 FRs and 3 user stories)
- [x] T003 [P] Implementation plan completed ([plan.md](plan.md) with constitution check)
- [x] T004 [P] Research decisions documented ([research.md](research.md) with 3 decisions)
- [x] T005 [P] Data model defined ([data-model.md](data-model.md) with 5 entities)
- [x] T006 [P] Configuration contract documented ([contracts/Kconfig-API.md](contracts/Kconfig-API.md))
- [x] T007 [P] Developer quickstart guide created ([quickstart.md](quickstart.md))

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core infrastructure that MUST be complete before ANY user story can be implemented

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [ ] T008 Add nopnop2002/esp-idf-ssd1306 component dependency in esp-idf/main/idf_component.yml
- [ ] T009 [P] Create hardware wiring reference document in esp-idf/ssd1306_esp32s3.md

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - SPI Hardware Configuration (Priority: P1) 🎯 MVP

**Goal**: Enable developers to configure SPI pins via menuconfig without modifying source code

**Independent Test**: Run `idf.py menuconfig`, navigate to BotiEyes configuration, select SPI protocol, verify all 6 SPI pin options are visible with sensible defaults (MOSI=11, SCK=12, CS=10, DC=9, RST=8, Clock=10MHz), save configuration, and build successfully

### Implementation for User Story 1

- [ ] T010 [US1] Extend esp-idf/main/Kconfig.projbuild to add SPI protocol option to existing I2C/SPI choice menu
- [ ] T011 [US1] Add CONFIG_BOTIEYES_OLED_MOSI_PIN configuration (int, range 0-48, default 11, depends on SPI) in esp-idf/main/Kconfig.projbuild
- [ ] T012 [US1] Add CONFIG_BOTIEYES_OLED_SCK_PIN configuration (int, range 0-48, default 12, depends on SPI) in esp-idf/main/Kconfig.projbuild
- [ ] T013 [US1] Add CONFIG_BOTIEYES_OLED_CS_PIN configuration (int, range -1 to 48, default 10, depends on SPI) in esp-idf/main/Kconfig.projbuild
- [ ] T014 [US1] Add CONFIG_BOTIEYES_OLED_DC_PIN configuration (int, range 0-48, default 9, depends on SPI) in esp-idf/main/Kconfig.projbuild
- [ ] T015 [US1] Add CONFIG_BOTIEYES_OLED_RST_PIN configuration (int, range -1 to 48, default 8, depends on SPI) in esp-idf/main/Kconfig.projbuild
- [ ] T016 [US1] Add CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ configuration (int, range 100000-10000000, default 10000000, depends on SPI) in esp-idf/main/Kconfig.projbuild
- [ ] T017 [US1] Verify menuconfig displays SPI options only when SPI protocol is selected via `idf.py menuconfig`
- [ ] T018 [US1] Verify project builds successfully with default SPI pin configuration via `idf.py build`

**Checkpoint**: At this point, User Story 1 is fully functional - developers can configure SPI pins via menuconfig

---

## Phase 4: User Story 2 - SPI Display Initialization and Rendering (Priority: P2)

**Goal**: Initialize SSD1306 display via SPI and render graphics using existing Adafruit_GFX rendering layer

**Independent Test**: Flash firmware to ESP32-S3 with correctly wired SSD1306 SPI display, observe display initialize, verify text/graphics appear on screen using BasicEmotion example, confirm smooth animation at 25 FPS

### Implementation for User Story 2

- [ ] T019 [P] [US2] Create display initialization header file esp-idf/main/display_init.h with init_display() function declaration
- [ ] T020 [US2] Implement SPI initialization function in esp-idf/main/display_init.cpp using nopnop2002/ssd1306 component API (spi_master_init + ssd1306_init)
- [ ] T021 [US2] Add conditional compilation for I2C vs SPI initialization paths in esp-idf/main/display_init.cpp using CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
- [ ] T022 [US2] Implement buffer integration layer in esp-idf/main/display_init.cpp to connect Adafruit_GFX framebuffer with ssd1306_set_buffer/show_buffer APIs
- [ ] T023 [US2] Update esp-idf/main/main.cpp to call init_display() during app_main() before rendering operations
- [ ] T024 [US2] Verify BasicEmotion example renders correctly on SPI display by flashing to ESP32-S3 hardware
- [ ] T025 [US2] Measure and log display update timing to confirm <1ms per frame (10 MHz SPI with DMA) using esp_timer_get_time()
- [ ] T026 [US2] Verify 25 FPS sustained frame rate with eye animations using performance counters

**Checkpoint**: At this point, User Stories 1 AND 2 both work - developers can configure and use SPI displays with existing BotiEyes rendering code unchanged

---

## Phase 5: User Story 3 - Initialization Error Handling (Priority: P3)

**Goal**: Provide clear visual and logging feedback when display initialization fails, enabling rapid hardware troubleshooting

**Independent Test**: Create error conditions (disconnected display, wrong pins, invalid GPIO numbers), verify error message appears in serial console within 500ms, confirm status LED turns red, verify firmware halts without attempting to render

### Implementation for User Story 3

- [ ] T027 [P] [US3] Add timeout tracking to SPI initialization in esp-idf/main/display_init.cpp using esp_timer_get_time() with 2-second timeout per FR-014
- [ ] T028 [P] [US3] Implement status LED initialization in esp-idf/main/main.cpp for GPIO38 using led_strip component (blue=initializing, green=success, red=error)
- [ ] T029 [US3] Add error logging for initialization failures in esp-idf/main/display_init.cpp with esp_log_level_set() and ESP_LOGE() macros
- [ ] T030 [US3] Implement halt behavior in esp-idf/main/main.cpp by returning from app_main() on initialization failure (system enters idle state)
- [ ] T031 [US3] Add DMA allocation failure detection in esp-idf/main/display_init.cpp and trigger error path (log, halt, LED red) per FR-015
- [ ] T032 [US3] Handle LED initialization failure gracefully in esp-idf/main/main.cpp (log warning, continue) per FR-016 if GPIO38 conflicts with other hardware
- [ ] T033 [US3] Add invalid pin detection in esp-idf/main/display_init.cpp to log warning for out-of-range GPIO numbers per clarification (accept, warn, attempt init)
- [ ] T034 [US3] Test timeout scenario by adding artificial delay in init code, verify 2-second timeout triggers error handling
- [ ] T035 [US3] Test DMA failure scenario by commenting out DMA initialization, verify error handling activates
- [ ] T036 [US3] Test invalid pin scenario by setting MOSI=99 in menuconfig, verify warning logged and initialization fails gracefully
- [ ] T037 [US3] Test LED conflict scenario by initializing GPIO38 for another purpose before LED init, verify warning logged but display init continues

**Checkpoint**: All user stories now independently functional - configuration, rendering, and error handling all work correctly

---

## Phase 6: Polish & Cross-Cutting Concerns

**Purpose**: Final validation, performance tuning, documentation updates

- [ ] T038 [P] Verify all BotiEyes examples (BasicEmotion, BrightnessChange, EyePosition, IdleBehavior) work with SPI configuration
- [ ] T039 [P] Update .github/copilot-instructions.md to add SPI-specific commit policy guidelines
- [ ] T040 [P] Run performance benchmark to confirm <1ms display update and 25 FPS sustained (log results to quickstart.md)
- [ ] T041 [P] Verify emulator (emulator/botieyes_emulator.py) continues to work unchanged (no SPI dependency)
- [ ] T042 Validate success criteria SC-001 through SC-006 from spec.md are all met
- [ ] T043 Create PR description summarizing changes and linking to spec/plan/quickstart docs
- [ ] T044 Final code review checklist: verify no Adafruit_GFX code modified, all SPI code conditionally compiled, constitution principles upheld

---

## Implementation Details (for Lower-Capability Models)

This section provides detailed implementation guidance for each task to ensure autonomous execution.

### Phase 2: Foundational Tasks

#### T008: Add Component Dependency

**File**: `esp-idf/main/idf_component.yml`

**Context**: See [research.md](research.md) Decision 1 for component integration pattern.

**Steps**:
1. Create or edit `esp-idf/main/idf_component.yml`
2. Add the following YAML structure:

```yaml
dependencies:
  nopnop2002/esp-idf-ssd1306:
    git: https://github.com/nopnop2002/esp-idf-ssd1306.git
    version: "*"
```

**Acceptance Criteria**:
- File exists at `esp-idf/main/idf_component.yml`
- Valid YAML syntax
- Component will be auto-fetched to `managed_components/` on next build
- Run `idf.py reconfigure` to verify component is downloaded

---

#### T009: Create Wiring Reference Document

**File**: `esp-idf/ssd1306_esp32s3.md`

**Context**: Document hardware connections for ESP32-S3 platform.

**Content Template**:
```markdown
# SSD1306 SPI Wiring for ESP32-S3

## Default Pin Assignment

| Signal | ESP32-S3 GPIO | SSD1306 Pin | Description |
|--------|---------------|-------------|-------------|
| MOSI   | GPIO11        | SDA/D1      | Data line   |
| SCK    | GPIO12        | SCL/D0      | Clock line  |
| CS     | GPIO10        | CS          | Chip select (active low) |
| DC     | GPIO9         | DC/A0       | Data/Command select |
| RST    | GPIO8         | RES         | Reset (active low) |
| VCC    | 3.3V          | VCC         | Power supply |
| GND    | GND           | GND         | Ground      |

## SPI Configuration

- **Bus**: SPI2_HOST
- **Clock**: 10 MHz (default)
- **Mode**: SPI Mode 0 (CPOL=0, CPHA=0)
- **DMA**: Enabled (SPI_DMA_CH_AUTO)

## Verification

1. Power off ESP32-S3
2. Connect wires per table above
3. Double-check VCC is 3.3V (NOT 5V)
4. Verify ground connection
5. Power on and flash firmware

## Troubleshooting

- **Blank screen**: Check RST and DC connections
- **Garbled display**: Verify clock speed ≤10 MHz
- **No response**: Check CS and MOSI connections
```

**Acceptance Criteria**:
- File created at repository root: `esp-idf/ssd1306_esp32s3.md`
- Table includes all 5 SPI signals plus power
- Troubleshooting section included

---

### Phase 3: User Story 1 Tasks

#### T010: Extend Kconfig for SPI Protocol Choice

**File**: `esp-idf/main/Kconfig.projbuild`

**Context**: See [contracts/Kconfig-API.md](contracts/Kconfig-API.md) Section 1 for the protocol selection contract.

**Current State**: File may already have I2C configuration. Need to convert to choice menu.

**Implementation**:
1. Open `esp-idf/main/Kconfig.projbuild`
2. Find or create `menu "BotiEyes Display Configuration"`
3. Add choice structure:

```kconfig
menu "BotiEyes Display Configuration"

choice BOTIEYES_OLED_PROTOCOL
    prompt "OLED communication protocol"
    default BOTIEYES_OLED_PROTOCOL_I2C
    help
        Select I2C or SPI interface for the OLED display.
        
        I2C: Fewer wires, slower updates (good for prototyping)
        SPI: More wires, faster updates (up to 10 MHz, 25 FPS)

config BOTIEYES_OLED_PROTOCOL_I2C
    bool "I2C"
    help
        Use I2C interface (default, typical for small OLEDs).
        Requires SDA and SCL pins only.

config BOTIEYES_OLED_PROTOCOL_SPI
    bool "Hardware SPI"
    help
        Use hardware SPI interface for faster updates (up to 10 MHz).
        Requires MOSI, SCK, CS, DC, and RST pins.

endchoice

# ... I2C configs here (depends on BOTIEYES_OLED_PROTOCOL_I2C) ...
# ... SPI configs will be added in T011-T016 ...

endmenu
```

**Acceptance Criteria**:
- Choice menu compiles without errors
- Running `idf.py menuconfig` shows "OLED communication protocol" with I2C and SPI options
- I2C is selected by default
- Both options have descriptive help text

---

#### T011-T016: Add SPI Pin Configuration Options

**Files**: `esp-idf/main/Kconfig.projbuild` (all tasks modify same file)

**Context**: See [contracts/Kconfig-API.md](contracts/Kconfig-API.md) Sections 2-7 for each pin specification.

**Pattern for Each Pin** (repeat for T011-T016):

```kconfig
config BOTIEYES_OLED_<PIN_NAME>_PIN
    int "<Pin Description>"
    range <MIN> <MAX>
    default <DEFAULT_VALUE>
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        <Help text describing pin function>
        Default: GPIO<DEFAULT_VALUE> (<hardware reference>)
```

**Specific Implementations**:

**T011 - MOSI Pin**:
```kconfig
config BOTIEYES_OLED_MOSI_PIN
    int "OLED SPI MOSI pin"
    range 0 48
    default 11
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI Master Out Slave In (data line).
        Connects to SDA/D1 pin on SSD1306 module.
        Default: GPIO11 (ESP32-S3 hardware layout)
```

**T012 - SCK Pin**:
```kconfig
config BOTIEYES_OLED_SCK_PIN
    int "OLED SPI SCK pin"
    range 0 48
    default 12
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI clock line.
        Connects to SCL/D0 pin on SSD1306 module.
        Default: GPIO12 (ESP32-S3 hardware layout)
```

**T013 - CS Pin**:
```kconfig
config BOTIEYES_OLED_CS_PIN
    int "OLED SPI CS pin"
    range -1 48
    default 10
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI chip select pin (active low).
        Set to -1 if CS is tied to GND on PCB.
        Default: GPIO10 (ESP32-S3 hardware layout)
```

**T014 - DC Pin**:
```kconfig
config BOTIEYES_OLED_DC_PIN
    int "OLED SPI DC pin"
    range 0 48
    default 9
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        Data/Command select pin.
        Low=command, High=data.
        Connects to DC/A0 pin on SSD1306 module.
        Default: GPIO9 (ESP32-S3 hardware layout)
```

**T015 - RST Pin**:
```kconfig
config BOTIEYES_OLED_RST_PIN
    int "OLED SPI RST pin"
    range -1 48
    default 8
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        Reset pin (active low).
        Set to -1 if RST is tied high via pullup resistor.
        Default: GPIO8 (ESP32-S3 hardware layout)
```

**T016 - Clock Speed**:
```kconfig
config BOTIEYES_SPI_CLOCK_SPEED_HZ
    int "SPI clock speed (Hz)"
    range 100000 10000000
    default 10000000
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI clock frequency in Hz.
        Maximum: 10 MHz (SSD1306 datasheet limit)
        Lower values may help with long wires or breadboard noise.
        Default: 10000000 (10 MHz, optimal for 25 FPS)
```

**Acceptance Criteria (for T011-T016)**:
- All 6 config options added to `Kconfig.projbuild`
- Each option only visible when SPI protocol is selected
- Defaults match specification (11, 12, 10, 9, 8, 10000000)
- Range validation enforces limits
- Help text explains pin function and default rationale

---

#### T017: Verify Menuconfig Display

**Command**: `cd esp-idf && idf.py menuconfig`

**Steps**:
1. Navigate to "BotiEyes Display Configuration"
2. Select "I2C" protocol → verify SPI options are hidden
3. Select "Hardware SPI" protocol → verify all 6 SPI options appear
4. Check each default value matches spec
5. Try invalid values (e.g., 99 for MOSI) → verify range error
6. Exit without saving

**Acceptance Criteria**:
- SPI options conditionally visible
- All defaults correct
- Range validation working
- No Kconfig syntax errors

---

#### T018: Verify Build with Default Config

**Command**: `cd esp-idf && idf.py build`

**Steps**:
1. Run `idf.py menuconfig`, select SPI protocol, save
2. Run `idf.py build`
3. Check for compilation errors
4. Verify `build/config/sdkconfig.h` contains CONFIG_ macros

**Acceptance Criteria**:
- Build completes successfully
- No warnings about undefined CONFIG symbols
- `sdkconfig.h` contains all 6 SPI pin CONFIG macros

---

### Phase 4: User Story 2 Tasks

#### T019: Create Display Initialization Header

**File**: `esp-idf/main/display_init.h`

**Context**: Interface for protocol-agnostic display initialization.

**Implementation**:
```cpp
#pragma once

#include "esp_err.h"
#include <stdint.h>

// Forward declaration for SSD1306_t
struct SSD1306_t;
typedef struct SSD1306_t SSD1306_t;

/**
 * @brief Initialize the OLED display based on Kconfig protocol selection
 * 
 * Initializes either I2C or SPI protocol based on CONFIG_BOTIEYES_OLED_PROTOCOL_*
 * Uses pin assignments from Kconfig.
 * 
 * @param dev Pointer to SSD1306 device structure (will be initialized)
 * @return ESP_OK on success, error code on failure
 */
esp_err_t init_display(SSD1306_t *dev);

/**
 * @brief Get display dimensions
 * 
 * @param width Output parameter for display width (128)
 * @param height Output parameter for display height (64)
 */
void get_display_dimensions(int *width, int *height);

/**
 * @brief Update display with current framebuffer
 * 
 * @param dev Pointer to initialized SSD1306 device
 * @param buffer Framebuffer data (1024 bytes)
 * @return ESP_OK on success
 */
esp_err_t update_display(SSD1306_t *dev, uint8_t *buffer);
```

**Acceptance Criteria**:
- File created at `esp-idf/main/display_init.h`
- Header guards present
- All functions documented
- Forward declaration of SSD1306_t to avoid circular includes

---

#### T020: Implement SPI Initialization Function

**File**: `esp-idf/main/display_init.cpp`

**Context**: See [research.md](research.md) Decision 1 for nopnop2002/ssd1306 integration pattern.

**Implementation**:
```cpp
#include "display_init.h"
#include "ssd1306.h"
#include "esp_log.h"

static const char *TAG = "display_init";

#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_SPI

esp_err_t init_display_spi(SSD1306_t *dev) {
    ESP_LOGI(TAG, "Initializing SSD1306 via SPI");
    ESP_LOGI(TAG, "Pin configuration: MOSI=%d SCK=%d CS=%d DC=%d RST=%d",
             CONFIG_BOTIEYES_OLED_MOSI_PIN,
             CONFIG_BOTIEYES_OLED_SCK_PIN,
             CONFIG_BOTIEYES_OLED_CS_PIN,
             CONFIG_BOTIEYES_OLED_DC_PIN,
             CONFIG_BOTIEYES_OLED_RST_PIN);
    
    // Set SPI clock speed (from Kconfig)
    spi_clock_speed(CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ);
    
    // Initialize SPI master interface
    // Uses SPI2_HOST with DMA enabled automatically
    esp_err_t ret = spi_master_init(
        dev,
        CONFIG_BOTIEYES_OLED_MOSI_PIN,  // MOSI
        CONFIG_BOTIEYES_OLED_SCK_PIN,   // SCK/CLK
        CONFIG_BOTIEYES_OLED_CS_PIN,    // CS (-1 if unused)
        CONFIG_BOTIEYES_OLED_DC_PIN,    // DC/A0
        CONFIG_BOTIEYES_OLED_RST_PIN    // RST/RES (-1 if tied high)
    );
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI master init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize display controller (128x64 geometry)
    ret = ssd1306_init(dev, 128, 64);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SSD1306 init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SSD1306 SPI initialization successful");
    return ESP_OK;
}

#endif // CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
```

**Acceptance Criteria**:
- Function compiles only when SPI protocol selected
- Uses all 6 CONFIG macros from Kconfig
- Calls `spi_master_init()` with correct pin order
- Calls `ssd1306_init()` with 128x64 dimensions
- Logs informative messages for debugging
- Returns esp_err_t for error handling

---

#### T021: Add Conditional Compilation for I2C/SPI

**File**: `esp-idf/main/display_init.cpp`

**Context**: Implement protocol dispatch based on Kconfig choice.

**Implementation** (add to existing file):
```cpp
// After SPI init function from T020, add:

#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_I2C

esp_err_t init_display_i2c(SSD1306_t *dev) {
    ESP_LOGI(TAG, "Initializing SSD1306 via I2C");
    // ... existing I2C initialization code ...
    // (This is placeholder - I2C code may already exist)
    return ESP_OK;
}

#endif // CONFIG_BOTIEYES_OLED_PROTOCOL_I2C

// Protocol dispatcher
esp_err_t init_display(SSD1306_t *dev) {
#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
    return init_display_spi(dev);
#elif defined(CONFIG_BOTIEYES_OLED_PROTOCOL_I2C)
    return init_display_i2c(dev);
#else
    #error "No display protocol selected in Kconfig"
#endif
}

void get_display_dimensions(int *width, int *height) {
    *width = 128;
    *height = 64;
}
```

**Acceptance Criteria**:
- `init_display()` dispatches to correct protocol function
- Only one protocol implementation compiled (no dead code)
- Compile error if no protocol selected
- Helper functions implemented

---

#### T022: Implement Buffer Integration Layer

**File**: `esp-idf/main/display_init.cpp`

**Context**: Connect Adafruit_GFX framebuffer to nopnop2002/ssd1306 component. See [data-model.md](data-model.md) Entity 3 for buffer layout.

**Implementation** (add to existing file):
```cpp
esp_err_t update_display(SSD1306_t *dev, uint8_t *buffer) {
    if (!dev || !buffer) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // Copy application buffer to component's internal buffer
    ssd1306_set_buffer(dev, buffer);
    
    // Transmit to display via SPI (uses DMA)
    ssd1306_show_buffer(dev);
    
    return ESP_OK;
}
```

**Buffer Format** (document in comments):
```cpp
/**
 * Buffer layout for SSD1306 (128x64, 1 bit per pixel):
 * - Total size: 1024 bytes
 * - Organization: 8 pages of 128 bytes each
 * - Page 0: rows 0-7 (bytes 0-127)
 * - Page 1: rows 8-15 (bytes 128-255)
 * - ...
 * - Page 7: rows 56-63 (bytes 896-1023)
 * 
 * Within each byte (column-major, LSB=top):
 * - Bit 0 (0x01): top pixel of 8-pixel column
 * - Bit 7 (0x80): bottom pixel of 8-pixel column
 * 
 * This layout is identical for I2C and SPI (protocol-agnostic).
 */
```

**Acceptance Criteria**:
- `update_display()` implemented
- Calls `ssd1306_set_buffer()` then `ssd1306_show_buffer()`
- Null pointer checks included
- Buffer layout documented in comments

---

#### T023: Update main.cpp to Call Display Init

**File**: `esp-idf/main/main.cpp`

**Context**: Integrate display initialization into application entry point.

**Implementation**:
```cpp
#include "display_init.h"
#include "ssd1306.h"
#include "esp_log.h"

static const char *TAG = "main";

extern "C" void app_main(void) {
    ESP_LOGI(TAG, "BotiEyes starting...");
    
    // Initialize display
    static SSD1306_t dev;  // Static to persist beyond app_main scope
    esp_err_t ret = init_display(&dev);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display initialization failed: %s", esp_err_to_name(ret));
        // Error handling will be added in User Story 3
        return;  // Halt on error
    }
    
    ESP_LOGI(TAG, "Display initialized successfully");
    
    // Clear display
    uint8_t framebuffer[1024] = {0};
    update_display(&dev, framebuffer);
    
    // ... rest of application code ...
    // Example: render emotions, animations, etc.
}
```

**Acceptance Criteria**:
- Display initialized before any rendering operations
- SSD1306_t device structure persists (static)
- Error checked and logged
- Framebuffer allocated (1024 bytes)
- Initial clear sent to display

---

#### T024-T026: Hardware Validation Tasks

**T024 - BasicEmotion Example Test**:
- Flash firmware to ESP32-S3 with `idf.py flash monitor`
- Observe serial output for "Display initialized successfully"
- Verify OLED display lights up
- Confirm graphics render correctly (no corruption)

**T025 - Display Update Timing**:
Add timing measurement to `update_display()`:
```cpp
esp_err_t update_display(SSD1306_t *dev, uint8_t *buffer) {
    int64_t start = esp_timer_get_time();
    
    ssd1306_set_buffer(dev, buffer);
    ssd1306_show_buffer(dev);
    
    int64_t elapsed_us = esp_timer_get_time() - start;
    ESP_LOGD("display_perf", "Display update: %lld us", elapsed_us);
    
    // Should be <1000 us (1 ms) at 10 MHz SPI with DMA
    if (elapsed_us > 1000) {
        ESP_LOGW("display_perf", "Update time exceeds 1ms target");
    }
    
    return ESP_OK;
}
```

**T026 - Frame Rate Validation**:
Add FPS counter to main loop:
```cpp
// In app_main() animation loop:
int frame_count = 0;
int64_t start_time = esp_timer_get_time();

while (1) {
    // Render frame...
    update_display(&dev, framebuffer);
    
    frame_count++;
    if (frame_count % 100 == 0) {
        int64_t elapsed_sec = (esp_timer_get_time() - start_time) / 1000000;
        float fps = (float)frame_count / elapsed_sec;
        ESP_LOGI("perf", "Frame rate: %.1f FPS", fps);
        
        // Should sustain ≥25 FPS
        if (fps < 25.0f) {
            ESP_LOGW("perf", "Frame rate below 25 FPS target");
        }
    }
    
    vTaskDelay(pdMS_TO_TICKS(40));  // 25 FPS = 40ms/frame
}
```

**Acceptance Criteria**:
- Display updates measure <1ms consistently
- Frame rate sustains ≥25 FPS over 100 frames
- No visual artifacts or tearing
- Performance logs confirm targets met

---

### Phase 5: User Story 3 Tasks

#### T027: Add Timeout Tracking

**File**: `esp-idf/main/display_init.cpp`

**Context**: Implement 2-second timeout per [spec.md](spec.md) FR-014.

**Implementation** (modify `init_display_spi()`):
```cpp
esp_err_t init_display_spi(SSD1306_t *dev) {
    const int64_t TIMEOUT_US = 2000000;  // 2 seconds
    int64_t start_time = esp_timer_get_time();
    
    ESP_LOGI(TAG, "Initializing SSD1306 via SPI (timeout: 2s)");
    
    // Check timeout before SPI init
    if (esp_timer_get_time() - start_time > TIMEOUT_US) {
        ESP_LOGE(TAG, "Timeout before SPI initialization");
        return ESP_ERR_TIMEOUT;
    }
    
    // Set SPI clock speed
    spi_clock_speed(CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ);
    
    // Initialize SPI master
    esp_err_t ret = spi_master_init(dev, ...);
    
    // Check timeout after SPI init
    if (esp_timer_get_time() - start_time > TIMEOUT_US) {
        ESP_LOGE(TAG, "Timeout during SPI initialization");
        return ESP_ERR_TIMEOUT;
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI master init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Initialize display
    ret = ssd1306_init(dev, 128, 64);
    
    // Final timeout check
    int64_t elapsed_us = esp_timer_get_time() - start_time;
    if (elapsed_us > TIMEOUT_US) {
        ESP_LOGE(TAG, "Timeout during display initialization");
        return ESP_ERR_TIMEOUT;
    }
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SSD1306 init failed: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "SSD1306 initialized in %lld ms", elapsed_us / 1000);
    return ESP_OK;
}
```

**Acceptance Criteria**:
- Timeout checked at 3 points: before SPI, after SPI, after display init
- Returns ESP_ERR_TIMEOUT if exceeded
- Elapsed time logged on success
- Timeout value is 2 seconds (2,000,000 microseconds)

---

#### T028: Implement Status LED

**File**: `esp-idf/main/main.cpp`

**Context**: RGB LED on GPIO38 for visual feedback. See [data-model.md](data-model.md) Entity 4.

**Implementation**:
```cpp
#include "led_strip.h"

static led_strip_handle_t led_strip = NULL;
static const int LED_GPIO = 38;

typedef enum {
    LED_OFF,
    LED_BLUE,   // Initializing
    LED_GREEN,  // Success
    LED_RED     // Error
} led_state_t;

esp_err_t init_status_led(void) {
    // Configure LED strip (WS2812/SK6812 RGB LED)
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = 1,  // Single LED
        .led_pixel_format = LED_PIXEL_FORMAT_GRB,
        .led_model = LED_MODEL_WS2812,
        .flags = {
            .invert_out = false,
        }
    };
    
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10000000,  // 10 MHz
        .flags = {
            .with_dma = false,
        }
    };
    
    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LED init failed (non-fatal): %s", esp_err_to_name(ret));
        return ret;  // Non-fatal per FR-016
    }
    
    // Clear LED
    led_strip_clear(led_strip);
    return ESP_OK;
}

void set_status_led(led_state_t state) {
    if (!led_strip) {
        return;  // LED not initialized, skip silently
    }
    
    switch (state) {
        case LED_OFF:
            led_strip_set_pixel(led_strip, 0, 0, 0, 0);
            break;
        case LED_BLUE:  // Initializing
            led_strip_set_pixel(led_strip, 0, 0, 0, 255);
            break;
        case LED_GREEN:  // Success
            led_strip_set_pixel(led_strip, 0, 0, 255, 0);
            break;
        case LED_RED:  // Error
            led_strip_set_pixel(led_strip, 0, 255, 0, 0);
            break;
    }
    
    led_strip_refresh(led_strip);
}

extern "C" void app_main(void) {
    // Initialize LED (non-fatal if fails)
    init_status_led();
    set_status_led(LED_BLUE);  // Initializing
    
    // Initialize display
    static SSD1306_t dev;
    esp_err_t ret = init_display(&dev);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display init failed: %s", esp_err_to_name(ret));
        set_status_led(LED_RED);  // Error
        return;  // Halt
    }
    
    set_status_led(LED_GREEN);  // Success
    ESP_LOGI(TAG, "Display initialized successfully");
    
    // ... rest of application ...
}
```

**Acceptance Criteria**:
- LED initializes to blue on startup
- LED turns green on successful display init
- LED turns red on display init failure
- LED init failure is non-fatal (warns but continues)
- Uses RMT peripheral for WS2812 protocol

---

#### T029-T033: Error Handling Implementation

**T029 - Error Logging**:
Already implemented in T020 and T027 - verify all error paths use `ESP_LOGE()`.

**T030 - Halt Behavior**:
Already implemented in T023 and T028 - `return` from `app_main()` halts firmware.

**T031 - DMA Failure Detection**:
Add check after `spi_master_init()` in T020:
```cpp
esp_err_t ret = spi_master_init(dev, ...);
if (ret == ESP_ERR_NO_MEM) {
    ESP_LOGE(TAG, "DMA allocation failed (critical)");
    return ESP_ERR_NO_MEM;  // Triggers LED red + halt
}
```

**T032 - LED Failure Handling**:
Already implemented in T028 - `init_status_led()` returns error but continues.

**T033 - Invalid Pin Detection**:
Add validation before `spi_master_init()`:
```cpp
// Validate GPIO numbers (ESP32-S3 has GPIO 0-48)
const int MAX_GPIO = 48;
if (CONFIG_BOTIEYES_OLED_MOSI_PIN < 0 || CONFIG_BOTIEYES_OLED_MOSI_PIN > MAX_GPIO) {
    ESP_LOGW(TAG, "MOSI pin %d may be invalid (range: 0-%d)", 
             CONFIG_BOTIEYES_OLED_MOSI_PIN, MAX_GPIO);
}
// Repeat for other pins...
// Note: Still attempt initialization (accept, warn, attempt per clarification)
```

**Acceptance Criteria**:
- All error paths logged with ESP_LOGE
- Halt implemented via return from app_main
- DMA failure triggers error handling
- LED failure logged as warning
- Invalid pins logged as warning before init attempt

---

#### T034-T037: Error Scenario Testing

**T034 - Timeout Test**:
Temporarily add delay to trigger timeout:
```cpp
esp_err_t init_display_spi(SSD1306_t *dev) {
    // ... existing code ...
    
    // TEST: Force timeout
    vTaskDelay(pdMS_TO_TICKS(2500));  // 2.5 seconds > 2 second timeout
    
    // ... rest of init code ...
}
```
Verify: Serial log shows "Timeout during..." message, LED turns red, firmware halts.
**Remove delay after test.**

**T035 - DMA Failure Test**:
Not practical to test directly. Verify error handling code path exists in T031.

**T036 - Invalid Pin Test**:
1. Run `idf.py menuconfig`
2. Set MOSI pin to 99 (out of range)
3. Save and build
4. Flash to hardware
5. Observe serial log shows warning: "MOSI pin 99 may be invalid"
6. Verify init fails and LED turns red

**T037 - LED Conflict Test**:
Temporarily initialize GPIO38 before LED init:
```cpp
void app_main(void) {
    // TEST: Occupy GPIO38
    gpio_set_direction((gpio_num_t)38, GPIO_MODE_OUTPUT);
    
    init_status_led();  // Should fail with warning
    // ... rest of code continues normally ...
}
```
Verify: Warning logged, but display init proceeds. **Remove test code after.**

**Acceptance Criteria**:
- Each error scenario validated on hardware
- Logs confirm expected error messages
- LED behavior matches specification
- System halts on fatal errors, continues on non-fatal

---

### Phase 6: Polish Tasks

**T038 - Examples Validation**:
Test each example in `BotiEyes/examples/`:
1. BasicEmotion: Flash and verify emotion renders
2. BrightnessChange: Verify brightness adjustment works
3. EyePosition: Verify eye movement works
4. IdleBehavior: Verify idle animations work

**T039 - Update Copilot Instructions**:
Add to `.github/copilot-instructions.md` after manual additions:
```markdown
### SPI Display Changes

When modifying SPI initialization code in `esp-idf/main/display_init.cpp`:
1. Test on ESP32-S3 hardware with SPI display
2. Verify serial output shows successful initialization
3. Confirm display updates at ≥25 FPS
4. Check error LED behavior (red on failure)

SPI-specific files:
- esp-idf/main/Kconfig.projbuild (pin configuration)
- esp-idf/main/display_init.cpp (SPI initialization)
- esp-idf/main/idf_component.yml (nopnop2002/ssd1306 dependency)
```

**T040 - Performance Benchmark**:
Run timing tests from T025-T026 and document results in quickstart.md:
```markdown
## Performance Results

Measured on ESP32-S3 @ 240 MHz with 10 MHz SPI:
- Display update time: 0.82 ms average (1024 bytes via DMA)
- Sustained frame rate: 27.3 FPS (40ms frame time)
- Initialization time: 145 ms (well under 2s timeout)
```

**T041 - Emulator Verification**:
```bash
cd emulator
python3 botieyes_emulator.py
# Verify no import errors, window opens, animations work
```

**T042 - Success Criteria Validation**:
Create checklist and verify:
- [ ] SC-001: Menuconfig configuration <2 min
- [ ] SC-002: Init on first attempt
- [ ] SC-003: 25 FPS sustained
- [ ] SC-004: Errors detected <500ms
- [ ] SC-005: Zero Adafruit_GFX changes
- [ ] SC-006: Build time increase <10s

**T043 - PR Description**:
```markdown
# SSD1306 SPI Component Integration

Implements FR-001 through FR-016 from spec.

## Changes
- Added SPI protocol support via nopnop2002/esp-idf-ssd1306
- 6 new Kconfig options for pin configuration
- Error handling with status LED and 2s timeout
- Zero changes to Adafruit_GFX rendering layer

## Testing
- Hardware validated on ESP32-S3
- All 4 examples work with SPI
- Performance: <1ms updates, 27 FPS sustained
- Error scenarios validated

## Documentation
- [Specification](specs/003-ssd1306-spi-component/spec.md)
- [Implementation Plan](specs/003-ssd1306-spi-component/plan.md)
- [Quickstart Guide](specs/003-ssd1306-spi-component/quickstart.md)
```

**T044 - Code Review Checklist**:
- [ ] No modifications to `BotiEyes/src/*.cpp` files
- [ ] No modifications to `emulator/*.py` files
- [ ] All SPI code wrapped in `#ifdef CONFIG_BOTIEYES_OLED_PROTOCOL_SPI`
- [ ] Constitution principles upheld (see plan.md)
- [ ] All design artifacts match implementation
- [ ] Performance targets met
- [ ] Error handling complete

---

## Dependencies & Execution Strategy

### User Story Dependency Graph

```
Setup (Phase 1)
    ↓
Foundational (Phase 2)
    ↓
    ├─→ User Story 1 (P1) [SPI Configuration] ← MVP START
    │       ↓
    ├─→ User Story 2 (P2) [Display Init & Rendering]
    │       ↓
    └─→ User Story 3 (P3) [Error Handling]
            ↓
        Polish (Phase 6) ← FEATURE COMPLETE
```

**Key Characteristics**:
- User Stories 1, 2, 3 must be completed sequentially (configuration → initialization → error handling)
- Within each story, tasks marked [P] can run in parallel
- Each story has independent test criteria (can validate completion without other stories)

### Parallel Execution Examples

**Within User Story 1** (after T010 creates choice menu structure):
```bash
# Can work in parallel:
git checkout -b task/us1-mosi  # T011
git checkout -b task/us1-sck   # T012
git checkout -b task/us1-cs    # T013
git checkout -b task/us1-dc    # T014
git checkout -b task/us1-rst   # T015
git checkout -b task/us1-clock # T016
# All add different Kconfig entries, no conflicts
```

**Within User Story 2**:
```bash
# Can work in parallel:
git checkout -b task/us2-header      # T019 (display_init.h)
git checkout -b task/us2-led-init    # T028 (status LED - actually US3, but independent)
# display_init.cpp tasks (T020-T022) must be sequential
```

**Within User Story 3** (after T027-T030 complete):
```bash
# Can work in parallel:
git checkout -b task/us3-timeout  # T027 (timeout tracking)
git checkout -b task/us3-led      # T028 (LED status)
# Error handling tasks (T029-T032) work on different error scenarios
```

**Polish Phase** (all tasks are parallel):
```bash
# Can work in parallel:
git checkout -b task/examples-validation  # T038
git checkout -b task/copilot-instructions # T039
git checkout -b task/performance-bench    # T040
git checkout -b task/emulator-verify      # T041
# All verify different aspects, no conflicts
```

### MVP Scope (User Story 1 Only)

For rapid validation and early feedback, implement **only User Story 1** first:
- **Tasks**: T001-T009 (setup/foundational) + T010-T018 (US1 implementation)
- **Deliverable**: Developer can configure SPI pins via menuconfig and build successfully
- **Value**: Validates Kconfig interface design, unblocks hardware testing, confirms build system integration
- **Validation**: Run menuconfig, verify all options visible with correct defaults, save, build
- **Time Estimate**: ~2-4 hours (assuming Kconfig.projbuild already exists)

### Full Feature Scope

For complete SPI support with rendering and error handling:
- **Tasks**: All (T001-T044)
- **Deliverable**: Full SPI display support with configuration, initialization, rendering, error handling, and documentation
- **Value**: Complete feature ready for production use
- **Validation**: All 6 success criteria (SC-001 to SC-006) met
- **Time Estimate**: ~8-12 hours (including hardware validation on ESP32-S3)

---

## Format Validation

✅ **All tasks follow checklist format**:
- Checkbox: `- [ ]` or `- [x]`
- Task ID: Sequential (T001 to T044)
- [P] marker: Applied to 15 parallelizable tasks
- [Story] label: Applied to all User Story phase tasks (US1, US2, US3)
- Description: Includes specific file path and action
- File paths: Absolute or relative to repository root

---

## Summary

- **Total Tasks**: 44 (7 completed in planning phase, 37 implementation tasks)
- **Setup/Foundational**: 9 tasks (T001-T009)
- **User Story 1** (P1 - Configuration): 9 tasks (T010-T018)
- **User Story 2** (P2 - Rendering): 8 tasks (T019-T026)
- **User Story 3** (P3 - Error Handling): 11 tasks (T027-T037)
- **Polish**: 7 tasks (T038-T044)
- **Parallelizable Tasks**: 15 tasks marked [P]
- **MVP Tasks**: 18 tasks (T001-T018, User Story 1 only)
- **Sequential Dependencies**: User Stories must complete in order (US1 → US2 → US3)
- **Independent Test Criteria**: Each user story has clear validation without dependencies

**Key Files Modified**:
- `esp-idf/main/Kconfig.projbuild` (7 new config options)
- `esp-idf/main/idf_component.yml` (add nopnop2002/ssd1306 dependency)
- `esp-idf/main/display_init.h` (new interface)
- `esp-idf/main/display_init.cpp` (SPI initialization logic)
- `esp-idf/main/main.cpp` (call display init, handle errors)
- `esp-idf/ssd1306_esp32s3.md` (hardware wiring reference)
- `.github/copilot-instructions.md` (commit policy update)

**Files Unchanged** (per FR-002):
- `BotiEyes/src/*.cpp` (all Adafruit_GFX rendering code)
- `emulator/*.py` (Python emulator)
