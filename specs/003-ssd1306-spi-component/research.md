# Research: SSD1306 SPI Component Integration

**Feature**: [spec.md](spec.md) | **Plan**: [plan.md](plan.md)
**Date**: 2026-06-13
**Phase**: 0 (Research & Technical Decisions)

## Overview

This document consolidates technical research for migrating SSD1306 hardware communication to the nopnop2002/esp-idf-ssd1306 component for both I2C and SPI protocols, replacing the existing custom I2C driver. All [NEEDS CLARIFICATION] markers from the Technical Context have been resolved through targeted research.

---

## Decision 1: Unified Component Integration for Both Protocols

**Context**: How to integrate the nopnop2002/esp-idf-ssd1306 component into the ESP-IDF build system to replace the existing custom I2C driver (esp_ssd1306.cpp/h) while supporting both I2C and SPI protocols and keeping the existing Adafruit_GFX rendering layer unchanged.

**Decision**: Use direct component integration via `idf_component.yml` with internal buffer access pattern for both I2C and SPI protocols, removing the custom driver entirely.

**Rationale**:
- nopnop2002/esp-idf-ssd1306 provides complete I2C and SPI abstraction with DMA support (for SPI)
- Internal framebuffer is compatible with Adafruit_GFX byte-oriented rendering
- Component manages all I2C and SPI transactions internally (init, commands, data transfer)
- Buffer API is protocol-agnostic (same for I2C and SPI)
- Eliminates maintenance burden of custom driver
- Proven, well-tested component from the community

**Implementation**:

### Component Declaration (`esp-idf/main/idf_component.yml`)
```yaml
dependencies:
  nopnop2002/ssd1306:
    path: components/ssd1306/
    git: https://github.com/nopnop2002/esp-idf-ssd1306.git
```

Component is auto-fetched to `managed_components/nopnop2002__ssd1306/` during build.

### SPI Initialization Pattern (Recommended)
```cpp
#include "ssd1306.h"

SSD1306_t dev;

// Configure clock speed (default 1MHz, max 10MHz)
spi_clock_speed(10000000);

// Initialize SPI interface
spi_master_init(&dev, 
    CONFIG_BOTIEYES_OLED_MOSI_PIN,   // GPIO11
    CONFIG_BOTIEYES_OLED_SCK_PIN,    // GPIO12
    CONFIG_BOTIEYES_OLED_CS_PIN,     // GPIO10
    CONFIG_BOTIEYES_OLED_DC_PIN,     // GPIO9
    CONFIG_BOTIEYES_OLED_RST_PIN);   // GPIO8

// Initialize display geometry
esp_err_t ret = ssd1306_init(&dev, 128, 64);
```

### Buffer Management (Adafruit_GFX Integration)
```cpp
// 1. Allocate framebuffer (128x64 = 1024 bytes)
uint8_t framebuffer[1024];

// 2. Get current buffer state (optional, for read-modify-write)
ssd1306_get_buffer(&dev, framebuffer);

// 3. Draw using Adafruit_GFX (writes to framebuffer)
// ... Adafruit_GFX drawing operations ...

// 4. Upload to display via SPI
ssd1306_set_buffer(&dev, framebuffer);  // Copy to component's internal buffer
ssd1306_show_buffer(&dev);              // Transmit via SPI with DMA
```

### DMA Configuration
DMA is **automatically enabled** during SPI bus initialization:
```cpp
spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
```
- `SPI_DMA_CH_AUTO`: Automatic DMA channel selection
- Max transfer: 1024 bytes (entire framebuffer in one DMA operation)
- No manual DMA management required

**Alternatives Considered**:
1. **Direct SPI register access**: Rejected - no DMA, reinventing the wheel
2. **Espressif's SSD1306 component**: Not found in component registry
3. **Arduino SSD1306 library port**: Rejected - I/O abstraction incompatible with ESP-IDF

**References**:
- Component repo: https://github.com/nopnop2002/esp-idf-ssd1306
- API header: `components/ssd1306/ssd1306.h`
- SPI implementation: `components/ssd1306/ssd1306_spi.c`

---

## Decision 2: Kconfig-Based Build-Time Configuration

**Context**: How to expose SPI pin configuration to developers without requiring source code changes.

**Decision**: Use `Kconfig.projbuild` with choice menu for protocol selection and conditional pin configuration.

**Rationale**:
- Kconfig is the standard ESP-IDF configuration mechanism
- Build-time configuration prevents runtime protocol switching (per spec requirement FR-011)
- Conditional visibility (`depends on`) hides irrelevant options
- Range validation prevents invalid GPIO assignments at menuconfig time

**Implementation**:

### Protocol Selection (Choice Menu)
```kconfig
menu "BotiEyes Display Configuration"

choice BOTIEYES_OLED_PROTOCOL
    prompt "OLED communication protocol"
    default BOTIEYES_OLED_PROTOCOL_I2C
    help
        Select I2C or SPI interface for the OLED display.

config BOTIEYES_OLED_PROTOCOL_I2C
    bool "I2C"
    help
        Use I2C interface (default, typical for small OLEDs).

config BOTIEYES_OLED_PROTOCOL_SPI
    bool "Hardware SPI"
    help
        Use hardware SPI interface for faster updates (up to 10 MHz).

endchoice
```

### SPI Pin Configuration
```kconfig
config BOTIEYES_OLED_MOSI_PIN
    int "OLED SPI MOSI pin"
    range 0 48
    default 11
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI Master Out Slave In (data line).
        Default: GPIO11 (ESP32-S3 hardware layout)

config BOTIEYES_OLED_SCK_PIN
    int "OLED SPI SCK pin"
    range 0 48
    default 12
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI clock line.
        Default: GPIO12 (ESP32-S3 hardware layout)

config BOTIEYES_OLED_CS_PIN
    int "OLED SPI CS pin"
    range -1 48
    default 10
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI chip select (active low). Set to -1 if not used.
        Default: GPIO10

config BOTIEYES_OLED_DC_PIN
    int "OLED SPI DC pin"
    range 0 48
    default 9
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        Data/Command select pin (low=command, high=data).
        Default: GPIO9

config BOTIEYES_OLED_RST_PIN
    int "OLED SPI RST pin"
    range -1 48
    default 8
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        Reset pin (active low). Set to -1 if tied high.
        Default: GPIO8

config BOTIEYES_SPI_CLOCK_SPEED_HZ
    int "SPI clock speed (Hz)"
    range 100000 10000000
    default 10000000
    depends on BOTIEYES_OLED_PROTOCOL_SPI
    help
        SPI clock frequency in Hz. SSD1306 datasheet maximum: 10 MHz.
        Lower values may be needed for long wires or noisy environments.

endmenu
```

### Code Access Pattern
```cpp
#include "sdkconfig.h"

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
```

**Alternatives Considered**:
1. **Runtime configuration files**: Rejected - violates FR-011 (build-time only)
2. **#define in header files**: Rejected - requires code changes, no validation
3. **CMake variables**: Considered but Kconfig is more user-friendly (GUI via menuconfig)

**References**:
- ESP-IDF Kconfig guide: `esp-idf/docs/en/api-guides/kconfig/project-configuration-guide.rst`
- Existing BotiEyes Kconfig: `esp-idf/main/Kconfig.projbuild` (follows all conventions)

---

## Decision 3: Multi-Layer Error Handling with Status LED

**Context**: How to handle initialization failures (timeout, DMA allocation, invalid pins) with clear diagnostics and user feedback.

**Decision**: Propagate `esp_err_t` codes from low-level functions, log detailed diagnostics at each layer, set status LED to red, and halt execution by returning from `app_main()`.

**Rationale**:
- ESP-IDF convention: Low-level functions return error codes, top-level decides handling
- Display is critical for BotiEyes operation (spec FR-008: must halt on failure)
- Status LED provides visual feedback when display is unavailable
- Returning from `app_main()` is clean (no boot loop risk, LED remains red)
- Detailed logging aids debugging (pin conflicts, wiring issues, timeout scenarios)

**Implementation**:

### 2-Second Timeout Pattern
```cpp
#include "esp_timer.h"

#define SPI_INIT_TIMEOUT_US (2 * 1000 * 1000)  // 2 seconds

bool init_ssd1306_with_timeout(spi_device_handle_t spi, gpio_num_t dc, gpio_num_t rst) {
    int64_t start_time = esp_timer_get_time();
    
    // Hardware reset
    gpio_set_level(rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // Send init commands with timeout checking
    const uint8_t init_cmds[] = { /* SSD1306 init sequence */ };
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        if ((esp_timer_get_time() - start_time) > SPI_INIT_TIMEOUT_US) {
            ESP_LOGE(TAG, "SSD1306 init timeout after %lld ms",
                     (esp_timer_get_time() - start_time) / 1000);
            return false;
        }
        
        esp_err_t err = ssd1306_write_command(spi, dc, init_cmds[i]);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Command 0x%02X failed: %s", init_cmds[i], esp_err_to_name(err));
            return false;
        }
    }
    
    ESP_LOGI(TAG, "SSD1306 init completed in %lld ms",
             (esp_timer_get_time() - start_time) / 1000);
    return true;
}
```

### Error Propagation with Cleanup
```cpp
esp_err_t ssd1306_spi_init(spi_device_handle_t *out_spi, const ssd1306_config_t *cfg) {
    // Step 1: SPI bus init
    spi_bus_config_t bus_cfg = { /* config */ };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    
    if (err == ESP_ERR_INVALID_ARG) {
        ESP_LOGE(TAG, "Invalid pin config: MOSI=%d, SCK=%d", cfg->mosi, cfg->sck);
        return err;
    } else if (err == ESP_ERR_NO_MEM) {
        ESP_LOGE(TAG, "DMA allocation failed - insufficient memory");
        return err;
    } else if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(err));
        return err;
    }
    
    // Step 2: Add device
    spi_device_interface_config_t dev_cfg = { /* config */ };
    err = spi_bus_add_device(SPI2_HOST, &dev_cfg, out_spi);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SPI add device failed: %s", esp_err_to_name(err));
        spi_bus_free(SPI2_HOST);  // Cleanup
        return err;
    }
    
    // Step 3: GPIO config for DC/RST
    gpio_config_t io_conf = { /* config */ };
    err = gpio_config(&io_conf);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %s", esp_err_to_name(err));
        spi_bus_remove_device(*out_spi);
        spi_bus_free(SPI2_HOST);
        return err;
    }
    
    // Step 4: Initialize display with timeout
    if (!init_ssd1306_with_timeout(*out_spi, cfg->dc_pin, cfg->rst_pin)) {
        ESP_LOGE(TAG, "Display not responding (timeout or hardware failure)");
        gpio_reset_pin(cfg->dc_pin);
        gpio_reset_pin(cfg->rst_pin);
        spi_bus_remove_device(*out_spi);
        spi_bus_free(SPI2_HOST);
        return ESP_ERR_TIMEOUT;
    }
    
    ESP_LOGI(TAG, "SSD1306 SPI initialization successful");
    return ESP_OK;
}
```

### Top-Level Halt Strategy
```cpp
extern "C" void app_main(void) {
    ESP_LOGI(TAG, "=== BotiEyes Startup ===");
    
    // Initialize status LED (blue = starting)
    if (init_status_led(CONFIG_BOTIEYES_STATUS_LED_PIN)) {
        set_status_led(0, 0, 255);  // Blue
    }
    
    // Initialize display
    spi_device_handle_t spi_display;
    ssd1306_config_t cfg = {
        .mosi_pin = CONFIG_BOTIEYES_OLED_MOSI_PIN,
        .sck_pin = CONFIG_BOTIEYES_OLED_SCK_PIN,
        .cs_pin = CONFIG_BOTIEYES_OLED_CS_PIN,
        .dc_pin = CONFIG_BOTIEYES_OLED_DC_PIN,
        .rst_pin = CONFIG_BOTIEYES_OLED_RST_PIN,
        .clock_speed_hz = CONFIG_BOTIEYES_SPI_CLOCK_SPEED_HZ,
    };
    
    esp_err_t err = ssd1306_spi_init(&spi_display, &cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "FATAL: Display initialization failed");
        ESP_LOGE(TAG, "Error: %s (0x%x)", esp_err_to_name(err), err);
        ESP_LOGE(TAG, "System halted - check wiring and pin config");
        set_status_led(255, 0, 0);  // Red
        
        return;  // Halt execution (scheduler stops)
    }
    
    set_status_led(0, 255, 0);  // Green: success
    ESP_LOGI(TAG, "Initialization complete");
    
    // Continue with normal operation...
}
```

### Status LED Handling (Non-Fatal)
```cpp
static led_strip_handle_t s_status_led = NULL;

bool init_status_led(int gpio_pin) {
    if (gpio_pin < 0) {
        ESP_LOGI(TAG, "Status LED disabled");
        return false;  // Not an error, just optional feature
    }
    
    led_strip_config_t strip_config = {
        .strip_gpio_num = gpio_pin,
        .max_leds = 1,
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000,
    };
    
    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_status_led);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "LED init failed: %s", esp_err_to_name(err));
        return false;  // Continue without LED (display is primary)
    }
    
    led_strip_clear(s_status_led);
    return true;
}

void set_status_led(uint8_t r, uint8_t g, uint8_t b) {
    if (s_status_led == NULL) return;  // Silent if LED unavailable
    
    led_strip_set_pixel(s_status_led, 0, r, g, b);
    led_strip_refresh(s_status_led);
}
```

**Error Scenario Handling**:
| Scenario | Detection | Log Message | Action |
|----------|-----------|-------------|--------|
| **Timeout** | `esp_timer_get_time()` | "SSD1306 init timeout after 2000 ms" | Return `ESP_ERR_TIMEOUT`, halt |
| **DMA failure** | `ESP_ERR_NO_MEM` from `spi_bus_initialize()` | "DMA allocation failed - insufficient memory" | Return error, halt |
| **Invalid pins** | `ESP_ERR_INVALID_ARG` from GPIO/SPI | "Invalid pin config: MOSI=X, SCK=Y" | Return error, halt |
| **LED conflict** | `ESP_ERR_*` from `led_strip_new_rmt_device()` | "LED init failed: [error]" | Log warning, continue |

**Alternatives Considered**:
1. **`ESP_ERROR_CHECK()` (abort)**: Rejected - doesn't allow LED indication before abort
2. **`esp_restart()`**: Rejected - may cause boot loops with persistent hardware faults
3. **Continue without display**: Rejected - display is mandatory per spec FR-008

**References**:
- ESP-IDF Error Handling: `esp-idf/docs/en/api-guides/error-handling.html`
- ESP-IDF Logging: `esp-idf/docs/en/api-reference/system/log.html`
- SPI Master Driver: `esp-idf/docs/en/api-reference/peripherals/spi_master.html`

---

## Research Summary

### All Technical Unknowns Resolved

| Area | Status | Key Decision |
|------|--------|-------------|
| Component integration | ✅ Resolved | `idf_component.yml` + buffer API pattern |
| SPI initialization | ✅ Resolved | `spi_master_init()` + `spi_clock_speed(10MHz)` |
| DMA configuration | ✅ Resolved | Automatic via `SPI_DMA_CH_AUTO` |
| Pin configuration | ✅ Resolved | Kconfig choice menu + conditional visibility |
| Buffer management | ✅ Resolved | `ssd1306_set_buffer()` + `ssd1306_show_buffer()` |
| Error handling | ✅ Resolved | Multi-layer propagation + status LED + halt |
| Timeout strategy | ✅ Resolved | 2s using `esp_timer_get_time()` |

### Ready for Phase 1 (Design)

All research findings provide sufficient detail to:
1. Define data model (SPI config structures, display device handle)
2. Document API contracts (Kconfig interface, initialization functions)
3. Create quickstart guide for developers

**No blockers remaining.** Proceed to Phase 1.
