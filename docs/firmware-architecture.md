# BotiEyes Firmware Architecture

**Version**: Feature 004 (Industrial ESP-IDF Architecture)  
**Target**: ESP32-S3 with SSD1306 OLED (SPI) and WS2812 LED  
**Framework**: ESP-IDF v6.0.1

---

## Table of Contents

1. [Overview](#overview)
2. [Layered Architecture](#layered-architecture)
3. [Task Architecture](#task-architecture)
4. [Component System](#component-system)
5. [Communication Patterns](#communication-patterns)
6. [Configuration System](#configuration-system)
7. [Build & Deployment](#build--deployment)

---

## Overview

BotiEyes firmware follows **professional industrial ESP-IDF patterns** with strict component boundaries, event-driven communication, and layered architecture. The design prioritizes maintainability, testability, and scalability for production robotics applications.

### Design Principles

1. **Component isolation** - Each component has clear API boundaries
2. **Event-driven** - Use ESP event loop for system-wide events
3. **Queue-based** - Pass data between tasks via FreeRTOS queues
4. **Hardware abstraction** - HAL layer isolates hardware dependencies
5. **Configuration-driven** - Kconfig system for hardware variants
6. **Industrial robustness** - Watchdog monitoring, error recovery, logging

### Key Features

- Dual-core task architecture (network + application)
- UDP network control service (port 4210)
- Emotion-driven eye animations on OLED display
- WiFi connection management with auto-reconnect
- Task health monitoring with watchdog
- Configuration persistence (NVS)
- Status indication via WS2812 RGB LED

---

## Layered Architecture

The firmware is organized in four layers:

```
┌─────────────────────────────────────────────────────┐
│  Application Layer (main.cpp, app_task.cpp)       │
│  - Initialization orchestration                     │
│  - BotiEyes eyes rendering loop                     │
│  - Network command processing                       │
└─────────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────┐
│  Service Layer (Components)                         │
│  - wifi_manager    : WiFi connectivity lifecycle    │
│  - state_machine   : Application state management   │
│  - config_manager  : NVS configuration persistence  │
│  - health_monitor  : Task watchdog & registry       │
└─────────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────┐
│  HAL Layer (hal_board)                              │
│  - hal_led         : LED abstraction (GPIO/WS2812) │
│  - hal_display     : Display abstraction (I2C/SPI) │
│  - Board variants  : TTGO LoRa32 / ESP32-S3 SPI    │
└─────────────────────────────────────────────────────┘
                      ↓
┌─────────────────────────────────────────────────────┐
│  Driver Layer (ESP-IDF + External)                  │
│  - ESP-IDF drivers : WiFi, SPI, I2C, GPIO, NVS     │
│  - led_strip       : WS2812 RGB LED driver          │
│  - ssd1306         : OLED display driver (temp)     │
│  - BotiEyes lib    : Eye rendering (C++)            │
└─────────────────────────────────────────────────────┘
```

### Layer Responsibilities

#### Application Layer
- **main.cpp**: System initialization, event loop, WiFi handlers
- **app_task.cpp**: Rendering loop (25 FPS), command processing, eye updates
- **Network control**: UDP server in BotiEyes library (net/)

#### Service Layer
- **wifi_manager**: Connection lifecycle, retry logic, event posting
- **state_machine**: APP_STATE transitions (IDLE→CONNECTING→CONNECTED→ERROR)
- **config_manager**: Load/save WiFi credentials, persistent settings
- **health_monitor**: Task watchdog registration, health reporting

#### HAL Layer
- **hal_board**: Board-specific pin mappings and initialization
- **hal_led**: Unified API for simple GPIO LED or WS2812 addressable LED
- **hal_display**: Unified API for I2C or SPI OLED displays (stub, Feature 005)

#### Driver Layer
- ESP-IDF native drivers for peripherals
- External component wrappers (led_strip for WS2812)
- Temporary external ssd1306 driver (to be replaced by HAL in Feature 005)

---

## Task Architecture

### FreeRTOS Tasks

| Task | Core | Priority | Stack | Responsibility |
|------|------|----------|-------|----------------|
| **network_task** | 0 | 23 (HIGH) | 4 KB | WiFi events, UDP server, network I/O |
| **app_task** | 1 | 12 (NORMAL) | 4 KB | Eye rendering (25 FPS), command processing |
| **idle_0** | 0 | 0 (IDLE) | - | ESP-IDF idle task |
| **idle_1** | 1 | 0 (IDLE) | - | ESP-IDF idle task |

### Task Communication

```
┌─────────────────┐
│  network_task   │  Core 0 (HIGH priority)
│   (UDP server)  │
└────────┬────────┘
         │ g_network_cmd_queue (FreeRTOS queue)
         │ Commands: { type, emotion, brightness }
         ↓
┌─────────────────┐
│   app_task      │  Core 1 (NORMAL priority)
│  (Eye renderer) │
└─────────────────┘

External Events:
┌──────────────┐
│ ESP Event    │  WiFi events, state changes
│ Loop         │  → Handlers in main.cpp
└──────────────┘
```

### Watchdog Protection

**ESP-IDF v6.0+** auto-initializes Task Watchdog Timer (TWDT) before `app_main()`.

- **Timeout**: 30 seconds
- **Monitored tasks**: network_task, app_task
- **Grace period**: 100ms (network) / 80ms (app) before first reset call
- **Recovery**: System reset on timeout (unrecoverable hang detected)

**See**: [docs/bugfix-twdt-initialization.md](bugfix-twdt-initialization.md) for v6.0 migration details.

---

## Component System

### Component Structure

Each ESP-IDF component follows strict boundaries:

```
components/{component}/
├── CMakeLists.txt          # Build configuration
├── Kconfig                 # Configuration options (optional)
├── include/
│   └── {component}.h       # Public API (ONLY this is visible to others)
└── src/
    ├── {component}.c       # Implementation
    └── internal.h          # Private headers (not visible outside)
```

### Component Dependencies

Components declare dependencies in `CMakeLists.txt`:

```cmake
idf_component_register(
    SRCS "src/wifi_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_netif nvs_flash config_manager  # Dependencies
)
```

**Rule**: Components can ONLY include headers from their declared `REQUIRES`.

### Component Registry

| Component | Public API | Dependencies | Purpose |
|-----------|------------|--------------|---------|
| **wifi_manager** | wifi_manager.h | esp_wifi, esp_netif, config_manager | WiFi lifecycle management |
| **state_machine** | app_state.h | esp_event | Application state transitions |
| **config_manager** | config_manager.h | nvs_flash | NVS configuration storage |
| **health_monitor** | health_monitor.h | esp_task_wdt | Watchdog & task registry |
| **hal_board** | hal_board.h, hal_led.h, hal_display.h | driver, led_strip | Hardware abstraction |
| **display** | display_init.h | ssd1306, adafruit_gfx, botieyes | Display adapter (temporary) |

---

## Communication Patterns

### 1. System-Wide Events (ESP Event Loop)

**Use for**: WiFi events, state changes, events with multiple observers

```cpp
// Define event base
ESP_EVENT_DECLARE_BASE(WIFI_MGR_EVENT);
enum {
    WIFI_MGR_EVENT_CONNECTED,    // value 0
    WIFI_MGR_EVENT_DISCONNECTED, // value 1
    WIFI_MGR_EVENT_FAILED        // value 2
};

// Post event (from wifi_manager)
esp_event_post(WIFI_MGR_EVENT, WIFI_MGR_EVENT_CONNECTED, 
               &ip_info, sizeof(ip_info), portMAX_DELAY);

// Register handler (in main.cpp)
esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_EVENT_CONNECTED,
                           &wifi_connected_handler, NULL);
```

**⚠️ Critical**: Use EVENT enum constants (WIFI_MGR_EVENT_CONNECTED), NOT status enum values!

### 2. Inter-Task Data Transfer (FreeRTOS Queues)

**Use for**: Passing commands/data between tasks

```cpp
// Network task sends commands to app task
typedef struct {
    uint8_t type;        // CMD_SET_EMOTION, CMD_SET_BRIGHTNESS
    uint8_t emotion;     // EMOTION_HAPPY, EMOTION_SAD, etc.
    uint8_t brightness;
} network_command_t;

// Create queue (in main.cpp)
QueueHandle_t g_network_cmd_queue = xQueueCreate(10, sizeof(network_command_t));

// Send (network_task)
xQueueSend(g_network_cmd_queue, &cmd, pdMS_TO_TICKS(100));

// Receive (app_task)
while (xQueueReceive(g_network_cmd_queue, &cmd, 0) == pdTRUE) {
    process_command(&cmd);
}
```

### 3. Shared Resource Protection (Mutexes)

**Use for**: Display hardware, state machine, shared data structures

```cpp
// Protect display access
static SemaphoreHandle_t s_display_mutex = xSemaphoreCreateMutex();

void safe_draw(void) {
    if (xSemaphoreTake(s_display_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        draw_to_display();
        xSemaphoreGive(s_display_mutex);
    }
}
```

### 4. Simple Signaling (Task Notifications)

**Use for**: Lightweight 1:1 task signaling (lighter than semaphores)

```cpp
// Wake up worker task
xTaskNotifyGive(worker_task_handle);

// Worker waits for signal
ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
```

### 5. Direct Calls (Within Component)

**Use for**: Same component, no task synchronization needed

```cpp
// Internal helpers in same component
static void helper_function(void) { }

esp_err_t public_api(void) {
    helper_function();  // Direct call - no overhead
}
```

**Reference**: See [docs/SYNCHRONIZATION_GUIDE.md](SYNCHRONIZATION_GUIDE.md) for detailed decision guide.

---

## Configuration System

### Kconfig Hierarchy

```
Project Configuration (idf.py menuconfig)
│
├── BotiEyes Network Control (main/Kconfig.projbuild)
│   ├── WiFi SSID/Password
│   ├── UDP Port (default: 4210)
│   ├── Status LED Pin
│   └── Frame Interval (default: 40ms = 25 FPS)
│
├── Board Configuration (hal_board/Kconfig)
│   ├── Board Selection: TTGO LoRa32 / ESP32-S3 SPI
│   ├── Display Protocol: I2C / SPI (auto-selected by board)
│   ├── Display Pins (conditional on protocol)
│   └── LED Type: None / GPIO / WS2812 (auto-selected by board)
│
└── SSD1306 Configuration (external lib - temporary)
    ├── Interface: I2C / SPI
    └── Pin Assignments (ESP32-S3 defaults: MOSI=11, SCK=12, etc.)
```

### Board Variants

#### TTGO LoRa32 (ESP32 + I2C SSD1306)
```
Display: I2C @ GPIO4 (SDA), GPIO15 (SCL), GPIO16 (RST)
LED: Simple GPIO @ GPIO2
```

#### ESP32-S3 + SPI SSD1306 (Current Target)
```
Display: SPI @ GPIO11 (MOSI), GPIO12 (SCK), GPIO10 (CS), 
               GPIO13 (DC), GPIO14 (RST), 10 MHz
LED: WS2812 addressable @ GPIO38
```

### Configuration Access

```cpp
// In C/C++ code
#include "sdkconfig.h"

int port = CONFIG_BOTIEYES_UDP_PORT;           // 4210
int mosi = CONFIG_BOTIEYES_OLED_SPI_MOSI;      // 11
int freq = CONFIG_BOTIEYES_OLED_SPI_FREQ;      // 10000000
```

### Configuration Files

- **sdkconfig**: Active build configuration (gitignored, local)
- **sdkconfig.defaults**: Default for ESP32-S3 SPI board (tracked)
- **sdkconfig.defaults.ttgo_lora32**: Default for TTGO LoRa32 (tracked)

**Security**: WiFi credentials in Kconfig defaults should be EMPTY. Local sdkconfig contains user credentials (gitignored).

**Reference**: See [docs/display-config-review.md](display-config-review.md) for configuration audit.

---

## Build & Deployment

### Build System

```bash
# Configure (interactive)
idf.py menuconfig

# Build firmware
idf.py build

# Flash to ESP32-S3
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor

# Combined flash + monitor
idf.py -p /dev/ttyUSB0 flash monitor
```

### Build Output

```
build/
├── botieyes-esp-idf.bin       # Main application (862 KB)
├── bootloader/
│   └── bootloader.bin         # ESP-IDF bootloader
├── partition_table/
│   └── partition-table.bin    # Partition layout
└── compile_commands.json      # For IDE/clangd
```

### Memory Layout

| Partition | Size | Usage | Free |
|-----------|------|-------|------|
| **app** | 1024 KB | 862 KB | 162 KB (16%) |
| bootloader | 32 KB | - | - |
| partition table | 4 KB | - | - |

### Project Structure

```
botieyes/
├── BotiEyes/                    # Arduino library (eyes rendering)
│   ├── src/                     # Core library + net/
│   └── examples/                # Arduino examples
├── esp-idf/                     # ESP-IDF firmware project
│   ├── main/                    # Application entry point
│   ├── components/              # Service + HAL components
│   ├── managed_components/      # External deps (gitignored)
│   └── sdkconfig.defaults       # Default configuration
├── controller/                  # Python UDP client
├── emulator/                    # Python/Pygame desktop emulator
├── specs/                       # Feature specifications
└── docs/                        # Architecture documentation
```

### ESP-IDF Environment

```bash
# Activate ESP-IDF v6.0.1
source ~/.espressif/tools/activate_idf_v6.0.1.sh

# Verify
idf.py --version  # ESP-IDF v6.0.1

# Change directory to project
cd esp-idf/
```

---

## Error Handling

### ESP-IDF Patterns

```cpp
// Always check error returns
esp_err_t ret = nvs_open("storage", NVS_READWRITE, &handle);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS: %s (state=%d)", 
             esp_err_to_name(ret), app_state);
    return ret;
}

// Use ESP_ERROR_CHECK only for unrecoverable errors
ESP_ERROR_CHECK(esp_event_loop_create_default());  // System failure if this fails
```

### Hierarchical Logging

```cpp
// Layer:Component:Module format
static const char *TAG = "SVC:WIFI:MGR";    // Service layer, wifi_manager
static const char *TAG = "HAL:BOARD:LED";   // HAL layer, hal_board, LED module
static const char *TAG = "APP:INIT";        // Application, initialization
static const char *TAG = "APP:TASK";        // Application, main task loop
```

**Log Levels**: ERROR (critical), WARN (recoverable), INFO (normal), DEBUG (verbose)

---

## State Machine

### Application States

```cpp
typedef enum {
    APP_STATE_IDLE = 0,       // Initial state, not initialized
    APP_STATE_CONNECTING,     // WiFi connection in progress
    APP_STATE_CONNECTED,      // WiFi connected, operational
    APP_STATE_DISCONNECTED,   // WiFi lost, retrying
    APP_STATE_ERROR           // Unrecoverable error
} app_state_t;
```

### State Transitions

```
IDLE → CONNECTING → CONNECTED → [operational]
  ↓         ↓            ↓
ERROR ← ─ ─ ┴ ─ ─ ─ ─ ─ ┴ ─ → DISCONNECTED → CONNECTING
                                    ↓
                                  ERROR (after max retries)
```

### Event Posting

```cpp
// Transition with reason
esp_err_t ret = app_state_transition(APP_STATE_CONNECTED, 
                                     "WiFi connected");

// Posts APP_STATE_EVENT to ESP event loop
// All registered handlers are notified
```

---

## Testing & Validation

### Hardware Testing Checklist

Before committing changes to embedded code (`BotiEyes/src/**`, `esp-idf/**`):

1. **Build**: `idf.py build` succeeds with no errors
2. **Flash**: `idf.py flash` uploads successfully
3. **Monitor**: Verify boot logs show no errors:
   - ✓ All components initialize
   - ✓ WiFi connects (if credentials configured)
   - ✓ Display initializes
   - ✓ Eye rendering starts (25 FPS)
   - ✓ No TWDT timeouts or task crashes
4. **Visual verification**: Eyes render correctly on OLED
5. **Network testing**: Controller can send commands via UDP

### Python Emulator Testing

```bash
cd emulator/
pip install -r requirements.txt
python botieyes_emulator.py
```

Use for rapid iteration on eye rendering logic without hardware.

---

## Future Work

### Feature 005: Complete HAL Display Implementation

**Goal**: Eliminate external ssd1306 library dependency

**Tasks**:
1. Implement `hal_display_spi.c` (currently stub)
2. Implement `hal_display_i2c.c` (currently stub)
3. Remove `components/display/` temporary adapter
4. Update `main.cpp` to use HAL display API
5. Single Kconfig source: `hal_board/Kconfig` only

### Other Planned Features

- OTA firmware updates
- Bluetooth Low Energy control
- SD card logging
- Multi-language support
- Animation scripting

---

## References

- **Feature Specs**: [specs/004-industrial-firmware-architecture/](../specs/004-industrial-firmware-architecture/)
- **Synchronization Guide**: [docs/SYNCHRONIZATION_GUIDE.md](SYNCHRONIZATION_GUIDE.md)
- **Architecture Enforcement**: [docs/ARCHITECTURE_ENFORCEMENT.md](ARCHITECTURE_ENFORCEMENT.md)
- **TWDT Bugfix**: [docs/bugfix-twdt-initialization.md](bugfix-twdt-initialization.md)
- **Display Config Audit**: [docs/display-config-review.md](display-config-review.md)
- **ESP-IDF Documentation**: https://docs.espressif.com/projects/esp-idf/en/v6.0.1/

---

**Last Updated**: 2026-07-03  
**Feature**: 004-industrial-firmware-architecture  
**Maintainer**: BotiEyes Team
