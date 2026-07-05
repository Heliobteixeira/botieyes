# Quickstart: Industrial Firmware Architecture

**Feature**: 004-industrial-firmware-architecture  
**Date**: 2026-06-30  
**Audience**: Developers working with the refactored ESP-IDF firmware

---

## Overview

This quickstart guide helps you understand, build, and modify the refactored BotiEyes ESP-IDF firmware. The refactoring introduces professional industrial patterns: layered components, event-driven architecture, hardware abstraction, and comprehensive monitoring.

**What Changed**:
- ✅ Modular component structure (WiFi, state, config, health, HAL)
- ✅ Event-driven communication (ESP event loop + FreeRTOS queues)
- ✅ Hardware abstraction layer (supports multiple boards/displays)
- ✅ Configuration management (Kconfig + NVS)
- ✅ System health monitoring (watchdog + crash recovery)
- ✅ Structured logging (hierarchical tags, runtime control)

**What Stayed the Same**:
- ✅ BotiEyes emotion rendering core
- ✅ UDP network control protocol
- ✅ Display rendering (SSD1306 + Adafruit GFX)
- ✅ Python emulator (unchanged)

---

## Prerequisites

### Hardware
- **Primary**: TTGO LoRa32 (ESP32 + SSD1306 I2C OLED, GPIO2 LED)
- **Alternative**: ESP32-S3 with SSD1306 SPI OLED + WS2812 LED

### Software
- ESP-IDF v5.0 or later
- Activation script: `source ~/.espressif/tools/activate_idf_v6.0.1.sh` (or your version)
- Python 3.8+ (for idf.py)

---

## Project Structure (Refactored)

```
esp-idf/                          # ESP-IDF project root
├── main/                         # Application layer
│   ├── main.cpp                  # Entry point (app_main)
│   ├── app_task.cpp              # Application task logic
│   └── Kconfig.projbuild         # User-facing configuration
│
├── components/                   # Service & HAL components
│   ├── botieyes/                 # BotiEyes library wrapper
│   ├── wifi_manager/             # WiFi lifecycle service
│   ├── state_machine/            # Application state management
│   ├── config_manager/           # NVS configuration abstraction
│   ├── health_monitor/           # Watchdog & crash recovery
│   └── hal_board/                # Hardware abstraction layer
│
├── sdkconfig.defaults.ttgo_lora32   # TTGO LoRa32 board defaults
├── sdkconfig.defaults.esp32s3_spi   # ESP32-S3 SPI variant defaults
└── CMakeLists.txt                # Project build config
```

**Key Concept**: Each component is self-contained with:
- `CMakeLists.txt` - Build config, dependencies
- `include/` - Public API headers
- `src/` - Private implementation

---

## Quick Build & Flash

### 1. Activate ESP-IDF Environment

```bash
cd /Users/helioteixeira/dev/botieyes/esp-idf
source ~/.espressif/tools/activate_idf_v6.0.1.sh
```

### 2. Configure for Your Board

**TTGO LoRa32 (I2C display)**:
```bash
rm -f sdkconfig
cp sdkconfig.defaults.ttgo_lora32 sdkconfig.defaults
idf.py set-target esp32
idf.py menuconfig  # Optional: customize WiFi SSID/password
```

**ESP32-S3 (SPI display)**:
```bash
rm -f sdkconfig
cp sdkconfig.defaults.esp32s3_spi sdkconfig.defaults
idf.py set-target esp32s3
idf.py menuconfig
```

### 3. Build & Flash

```bash
idf.py build
idf.py -p /dev/cu.usbserial-XXXXXXXX flash monitor
```

**Monitor Output**: Press `Ctrl+]` to exit monitor.

### 4. Set WiFi Credentials (Runtime)

**Option A**: Via menuconfig (build-time):
```bash
idf.py menuconfig
# → BotiEyes Network Control → Wi-Fi SSID / Wi-Fi password
idf.py flash
```

**Option B**: Via NVS (runtime, future feature):
```bash
# Serial console command (future):
# > config_wifi "MySSID" "MyPassword"
```

---

## Architecture Overview

### Layered Design

```
┌─────────────────────────────────────────┐
│  Application Layer (main/)              │
│  - app_main() orchestration             │
│  - app_task (rendering, command proc)   │
└─────────────────────────────────────────┘
                ↓ uses
┌─────────────────────────────────────────┐
│  Service Layer (components/)            │
│  - wifi_manager (connectivity)          │
│  - state_machine (lifecycle)            │
│  - config_manager (NVS)                 │
│  - health_monitor (watchdog)            │
└─────────────────────────────────────────┘
                ↓ uses
┌─────────────────────────────────────────┐
│  HAL Layer (hal_board/)                 │
│  - Board initialization                 │
│  - Display abstraction (I2C/SPI)        │
│  - LED abstraction (GPIO/WS2812)        │
└─────────────────────────────────────────┘
                ↓ uses
┌─────────────────────────────────────────┐
│  Driver Layer                           │
│  - Adafruit GFX (rendering)             │
│  - led_strip (WS2812)                   │
│  - ESP-IDF drivers (SPI, I2C, GPIO)     │
└─────────────────────────────────────────┘
```

### Event-Driven Communication

**ESP Event Loop** (system-wide events):
- WiFi events (`WIFI_MGR_EVENT:CONNECTED`, `DISCONNECTED`, `FAILED`)
- State changes (`APP_STATE_EVENT:STATE_CHANGED`)

**FreeRTOS Queues** (inter-task data):
- Network commands: `network_task` → `app_task` via `g_network_cmd_queue`

**Mutexes** (shared resource protection):
- Display hardware: `g_display_mutex` (prevents concurrent SPI/I2C access)
- State machine: `g_state_mutex` (thread-safe state transitions)

---

## Common Development Tasks

### Adding a New Board Variant

1. **Create board config** (`hal_board/src/boards/my_board.c`):
   ```c
   const hal_board_config_t board_config = {
       .display = {
           .type = HAL_DISPLAY_I2C,
           .i2c = { .sda = GPIO_NUM_21, .scl = GPIO_NUM_22, .freq_hz = 400000, .address = 0x3C }
       },
       .led = {
           .type = HAL_LED_GPIO,
           .gpio = { .pin = GPIO_NUM_2, .active_high = true }
       }
   };
   ```

2. **Add Kconfig option** (`hal_board/Kconfig`):
   ```kconfig
   config HAL_BOARD_MY_BOARD
       bool "My Custom Board"
   ```

3. **Create sdkconfig defaults** (`sdkconfig.defaults.my_board`):
   ```ini
   CONFIG_HAL_BOARD_MY_BOARD=y
   CONFIG_BOTIEYES_OLED_PROTOCOL_I2C=y
   # ... pin configs
   ```

4. **Build**:
   ```bash
   cp sdkconfig.defaults.my_board sdkconfig.defaults
   idf.py build
   ```

### Handling WiFi Events in Your Code

```c
#include "wifi_manager.h"

static void my_wifi_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data) {
    if (event_id == WIFI_MGR_CONNECTED) {
        esp_netif_ip_info_t *ip_info = (esp_netif_ip_info_t *)event_data;
        ESP_LOGI(TAG, "Connected! IP: " IPSTR, IP2STR(&ip_info->ip));
    }
}

void app_main(void) {
    // ...
    wifi_manager_init();
    esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_CONNECTED, 
                                &my_wifi_handler, NULL);
    wifi_manager_connect();
}
```

### Adding Application State Change Logic

```c
#include "app_state.h"

static void my_state_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data) {
    app_state_info_t *info = (app_state_info_t *)event_data;
    ESP_LOGI(TAG, "State: %d → %d", info->previous, info->current);
    
    if (info->current == APP_STATE_RUNNING) {
        // Start network server
    }
}

void app_main(void) {
    // ...
    app_state_init();
    esp_event_handler_register(APP_STATE_EVENT, STATE_CHANGED,
                                &my_state_handler, NULL);
}
```

### Storing/Retrieving Configuration

```c
#include "config_manager.h"

void my_init(void) {
    config_manager_init();
    
    // Load WiFi config
    config_wifi_t wifi_cfg;
    if (config_get_wifi(&wifi_cfg) == ESP_OK) {
        ESP_LOGI(TAG, "SSID: %s", wifi_cfg.ssid);
    }
    
    // Load app config
    config_app_t app_cfg;
    if (config_get_app(&app_cfg) == ESP_OK) {
        ESP_LOGI(TAG, "Brightness: %d", app_cfg.brightness);
    }
}

void save_new_wifi(const char *ssid, const char *password) {
    config_wifi_t cfg = {
        .auto_reconnect = true
    };
    strncpy(cfg.ssid, ssid, sizeof(cfg.ssid) - 1);
    strncpy(cfg.password, password, sizeof(cfg.password) - 1);
    
    esp_err_t err = config_set_wifi(&cfg);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "WiFi config saved");
    }
}
```

### Creating a New Component

1. **Create directory structure**:
   ```bash
   mkdir -p components/my_component/{include,src}
   ```

2. **Write CMakeLists.txt** (`components/my_component/CMakeLists.txt`):
   ```cmake
   idf_component_register(
       SRCS "src/my_component.c"
       INCLUDE_DIRS "include"
       REQUIRES esp_event nvs_flash  # Dependencies
   )
   ```

3. **Define public API** (`include/my_component.h`):
   ```c
   #pragma once
   #include "esp_err.h"
   
   esp_err_t my_component_init(void);
   void my_component_do_something(void);
   ```

4. **Implement** (`src/my_component.c`):
   ```c
   #include "my_component.h"
   #include "esp_log.h"
   
   static const char *TAG = "SVC:MYCOMP";
   
   esp_err_t my_component_init(void) {
       ESP_LOGI(TAG, "Initialized");
       return ESP_OK;
   }
   ```

5. **Use in main** (`main/main.cpp`):
   ```cpp
   #include "my_component.h"
   
   extern "C" void app_main(void) {
       my_component_init();
   }
   ```

---

## Monitoring & Debugging

### Serial Console Output

**Hierarchical log tags** enable filtering:
```
I (1234) APP:INIT: Starting BotiEyes...
I (1250) HAL:BOARD: Initializing TTGO LoRa32
I (1280) HAL:DISPLAY: I2C display initialized
I (1300) SVC:WIFI:MGR: Connecting to "MyWiFi"...
I (3500) SVC:WIFI:MGR: Connected: IP=192.168.1.100
I (3520) SVC:STATE: CONNECTING → CONNECTED
```

### Runtime Log Level Control

Change log levels at runtime without recompiling:

```c
// Set specific component to DEBUG
esp_log_level_set("SVC:WIFI:MGR", ESP_LOG_DEBUG);

// Set all WiFi components to DEBUG using wildcard
esp_log_level_set("SVC:WIFI:*", ESP_LOG_DEBUG);

// Set all services to INFO
esp_log_level_set("SVC:*", ESP_LOG_INFO);

// Silence verbose HAL logs
esp_log_level_set("HAL:*", ESP_LOG_WARN);

// Set application layer to VERBOSE for detailed debugging
esp_log_level_set("APP:*", ESP_LOG_VERBOSE);

// Set network layer to DEBUG
esp_log_level_set("NET:*", ESP_LOG_DEBUG);
```

**Available log levels** (from most to least verbose):
- `ESP_LOG_VERBOSE` - Detailed trace information
- `ESP_LOG_DEBUG` - Debug messages
- `ESP_LOG_INFO` - Informational messages (default)
- `ESP_LOG_WARN` - Warnings
- `ESP_LOG_ERROR` - Errors only
- `ESP_LOG_NONE` - Disable all logs

**Common debugging scenarios**:

```c
// Debug WiFi connection issues
esp_log_level_set("SVC:WIFI:*", ESP_LOG_DEBUG);
esp_log_level_set("HAL:*", ESP_LOG_INFO);

// Debug network command processing
esp_log_level_set("NET:*", ESP_LOG_DEBUG);
esp_log_level_set("APP:TASK", ESP_LOG_DEBUG);

// Debug state machine transitions
esp_log_level_set("SVC:STATE", ESP_LOG_DEBUG);

// Debug hardware initialization
esp_log_level_set("HAL:*", ESP_LOG_DEBUG);
esp_log_level_set("APP:INIT", ESP_LOG_DEBUG);

// Reduce noise from verbose components
esp_log_level_set("HAL:DISPLAY", ESP_LOG_WARN);
esp_log_level_set("NET:UDP", ESP_LOG_INFO);
```

### Stack Monitoring

**Check task stack usage** (in `health_monitor`):
```c
UBaseType_t watermark = uxTaskGetStackHighWaterMark(task_handle);
ESP_LOGW(TAG, "Task %s: stack watermark = %u bytes", name, watermark);
```

**Warning threshold**: < 512 bytes free (too close to overflow)

### Heap Monitoring

```c
uint32_t free_heap = esp_get_free_heap_size();
uint32_t min_free = esp_get_minimum_free_heap_size();
ESP_LOGI(TAG, "Heap: %u free, min %u", free_heap, min_free);
```

### Crash Analysis

**After watchdog reset or panic**:
```bash
idf.py monitor
# Look for:
# E (123) SYS:HEALTH: Previous crash: reason=TASK_WDT, task=network, PC=0x40012345
```

**View stored crash log** (via config_manager):
```c
crash_log_t log;
if (config_get_crash_log(&log) == ESP_OK) {
    ESP_LOGE(TAG, "Last crash: %s at PC=0x%08x", log.description, log.pc);
}
```

---

## Testing

### On-Hardware Validation

**Build and flash**:
```bash
idf.py build flash monitor
```

**Test WiFi connectivity**:
1. Watch serial console for `SVC:WIFI:MGR: Connected`
2. Check IP address in logs
3. Test network control: `python controller/botieyes_client.py <ESP_IP>`

**Test state transitions**:
1. Watch logs for `SVC:STATE: INIT → CONNECTING → CONNECTED → RUNNING`
2. Disconnect WiFi AP, verify auto-reconnection
3. Verify state returns to `CONNECTING`, then `CONNECTED` after reconnection

**Test watchdog**:
1. Intentionally block a task (infinite loop without `vTaskDelay`)
2. Verify watchdog triggers reset after 30 seconds
3. Check logs on reboot for watchdog trigger message

### Network Control Testing

**Python client** (`controller/botieyes_client.py`):
```bash
cd controller
python botieyes_client.py 192.168.1.100  # ESP IP address

# Interactive commands:
# emotion 0.5 0.8    - Set emotion (valence, arousal)
# position 0.2 -0.1  - Set eye position
# idle               - Enter idle mode
# ping               - Send heartbeat
```

---

## Performance Profiling

### Frame Timing

**Measure frame duration**:
```cpp
uint64_t start = esp_timer_get_time();
eyes->update();
uint64_t duration_us = esp_timer_get_time() - start;

if (duration_us > 40000) {  // 40ms target
    ESP_LOGW(TAG, "Frame took %llu us (target: 40000)", duration_us);
}
```

### Event Latency

**Measure WiFi event handling**:
```c
static void wifi_event_handler(void *arg, esp_event_base_t base,
                                 int32_t id, void *data) {
    uint64_t now = esp_timer_get_time();
    // Compare 'now' with timestamp in event data
}
```

### Memory Profiling

```bash
idf.py size        # Binary size breakdown
idf.py size-components  # Per-component size
```

---

## Configuration Reference

### Kconfig Options

**Key settings** (`idf.py menuconfig` → BotiEyes Network Control):

| Option | Default | Description |
|--------|---------|-------------|
| `BOTIEYES_WIFI_SSID` | "" | WiFi SSID (can be set via NVS) |
| `BOTIEYES_WIFI_PASSWORD` | "" | WiFi password |
| `BOTIEYES_UDP_PORT` | 4210 | Network control UDP port |
| `BOTIEYES_STATUS_LED_PIN` | 38 | GPIO for status LED (-1 = disabled) |
| `BOTIEYES_FRAME_INTERVAL_MS` | 40 | Frame period (25 FPS) |
| `BOTIEYES_OLED_PROTOCOL` | I2C | Display interface (I2C or SPI) |

### NVS Namespaces

| Namespace | Keys | Purpose |
|-----------|------|---------|
| `botieyes_wifi` | `ssid`, `password`, `auto_reconnect` | WiFi credentials |
| `botieyes_app` | `brightness`, `idle_timeout_s`, `network_enabled` | App settings |
| `botieyes_sys` | `boot_count`, `last_crash` | System data |

**Factory reset** (erase user settings):
```c
config_factory_reset();  // Erases botieyes_wifi and botieyes_app
```

---

## Troubleshooting

### Build Errors

**"Component not found"**:
```bash
# Check CMakeLists.txt in main/ has:
# idf_component_register(REQUIRES wifi_manager state_machine ...)
```

**"Header not found"**:
```bash
# Ensure component has:
# idf_component_register(INCLUDE_DIRS "include" ...)
```

### Runtime Errors

**WiFi won't connect**:
1. Check SSID/password in `idf.py menuconfig`
2. Verify WiFi AP is WPA2 (WEP not supported)
3. Check logs: `SVC:WIFI:MGR: Failed to connect: reason=X`

**Display not working**:
1. Verify correct protocol selected (I2C vs SPI) in menuconfig
2. Check pin mappings match your board
3. Look for `HAL:DISPLAY: initialization failed` in logs

**Watchdog resets**:
1. Check logs for task name: `E (123) SYS:HEALTH: Task 'network' triggered watchdog`
2. Verify task calls `esp_task_wdt_reset()` regularly
3. Reduce task stack if watermark > 512 bytes free

---

## Next Steps

1. **Implement tasks**: See [tasks.md](./tasks.md) (generated by `/speckit.tasks`)
2. **Review architecture**: [plan.md](./plan.md), [data-model.md](./data-model.md)
3. **Component APIs**: [contracts/component-api.md](./contracts/component-api.md)
4. **Best practices**: [../docs/SYNCHRONIZATION_GUIDE.md](../../../docs/SYNCHRONIZATION_GUIDE.md)

---

## Getting Help

- **ESP-IDF Docs**: https://docs.espressif.com/projects/esp-idf/
- **BotiEyes Repository**: https://github.com/Heliobteixeira/botieyes
- **Issues**: GitHub Issues tab

---

**Happy refactoring!** 🤖👀
