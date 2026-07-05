# Research: Industrial Firmware Architecture Patterns

**Feature**: 004-industrial-firmware-architecture  
**Date**: 2026-06-30  
**Purpose**: Document best practices and design decisions for ESP-IDF industrial firmware refactoring

---

## Table of Contents

1. [ESP-IDF Component Architecture](#1-esp-idf-component-architecture)
2. [Event-Driven Architecture](#2-event-driven-architecture)
3. [FreeRTOS Task Management](#3-freertos-task-management)
4. [Hardware Abstraction Patterns](#4-hardware-abstraction-patterns)
5. [Configuration Management](#5-configuration-management)
6. [System Health & Recovery](#6-system-health--recovery)
7. [Logging Strategies](#7-logging-strategies)
8. [Performance Considerations](#8-performance-considerations)

---

## 1. ESP-IDF Component Architecture

### Decision: Layered Component Structure

**Chosen Approach**: Application → Services → HAL → Drivers → ESP-IDF

**Rationale**:
- **Industry Standard**: ESP-IDF component system is designed for modularity
- **Independent Testing**: Each component can be built/tested in isolation
- **Reusability**: Service components (WiFi manager, state machine) can be reused across projects
- **Dependency Control**: CMakeLists.txt explicitly declares component dependencies, preventing circular references

**Component Structure Pattern**:
```
components/{component_name}/
├── CMakeLists.txt          # Declares SRCS, INCLUDE_DIRS, REQUIRES
├── Kconfig                 # Component-specific build configuration
├── include/                # Public API headers (installed to build include path)
│   └── {component}.h
└── src/                    # Private implementation (not exposed)
    └── *.c
```

**Key Rules**:
- Public API: `include/{component}/*.h` - visible to dependent components
- Private implementation: `src/*.c` - internal only
- Dependencies: Declared via `REQUIRES` in CMakeLists.txt
- No direct includes of other component's `src/` files

**Alternatives Considered**:
- **Flat directory structure**: Rejected because it doesn't scale; everything depends on everything
- **Git submodules per component**: Overkill for single-project refactoring; adds version management complexity
- **Header-only libraries**: Not suitable for embedded C/C++ with limited flash; want separate compilation

**References**:
- ESP-IDF Component System: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#component-cmakelists-files
- Component Architecture Best Practices: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#component-requirements

---

## 2. Event-Driven Architecture

### Decision: ESP Event Loop + FreeRTOS Queues

**Chosen Approach**: 
- **ESP event loop** (`esp_event`) for system-wide events with multiple observers
- **FreeRTOS queues** for inter-task data transfer
- **Mutexes** for shared resource protection
- **Task notifications** for lightweight 1:1 signaling

**Rationale**:
- **Decoupling**: Publishers don't need to know about subscribers
- **Standard Pattern**: ESP-IDF uses esp_event for WiFi, IP, system events
- **Performance**: Queues provide bounded-latency message passing between cores
- **Simplicity**: Task notifications are lighter than semaphores for simple signaling

**Event Loop Usage**:
```cpp
// WiFi Manager posts events:
ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT);
esp_event_post(WIFI_MGR_EVENT, WIFI_MGR_CONNECTED, &ip_info, sizeof(ip_info), portMAX_DELAY);

// Multiple components register handlers:
esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_CONNECTED, &app_on_wifi_connected, NULL);
esp_event_handler_register(WIFI_MGR_EVENT, WIFI_MGR_CONNECTED, &display_show_status, NULL);
```

**Queue Usage**:
```cpp
// Network task sends commands:
QueueHandle_t cmd_queue = xQueueCreate(10, sizeof(network_cmd_t));
xQueueSend(cmd_queue, &cmd, pdMS_TO_TICKS(100));

// Application task receives:
network_cmd_t cmd;
if (xQueueReceive(cmd_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
    process_command(&cmd);
}
```

**When to Use What** (per SYNCHRONIZATION_GUIDE.md):
| Mechanism | Use Case | Example |
|-----------|----------|---------|
| ESP event loop | System events, multiple observers | WiFi connected, state change |
| Queue | Inter-task data transfer | Network commands, sensor data |
| Mutex | Shared resource protection | Display hardware, state machine |
| Task notification | Lightweight 1:1 signaling | "Work ready", "Frame complete" |
| Event group | Multiple condition wait | Initialization (WiFi + Display + Config ready) |

**Alternatives Considered**:
- **Direct function calls across components**: Creates tight coupling, doesn't work across cores/tasks
- **Callback registration**: Possible but reinvents esp_event; non-standard pattern for ESP-IDF
- **Global shared state with flags**: Race conditions, requires complex locking, hard to debug

**References**:
- ESP Event Loop: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_event.html
- FreeRTOS Queues: https://www.freertos.org/Embedded-RTOS-Queues.html
- Project's SYNCHRONIZATION_GUIDE.md

---

## 3. FreeRTOS Task Management

### Decision: Priority-Based Task Hierarchy with Core Pinning

**Chosen Approach**:
- **4 priority levels**: CRITICAL (configMAX_PRIORITIES-1), HIGH, NORMAL, LOW
- **3 stack sizes**: LARGE (8KB), MEDIUM (4KB), SMALL (2KB)
- **Core pinning**: Network/WiFi → Core 0, Rendering/Compute → Core 1

**Rationale**:
- **Real-Time Guarantees**: Higher-priority tasks preempt lower-priority tasks
- **Core Affinity**: Prevents cache thrashing; keeps related work on same core
- **Memory Efficiency**: Right-sized stacks prevent waste; monitored via watermarks
- **Predictability**: Fixed priorities make scheduling behavior determinable

**Task Design Pattern**:
```cpp
// Task configuration
#define TASK_PRIORITY_CRITICAL  (configMAX_PRIORITIES - 1)
#define TASK_PRIORITY_HIGH      (configMAX_PRIORITIES - 2)
#define TASK_PRIORITY_NORMAL    (configMAX_PRIORITIES / 2)
#define TASK_PRIORITY_LOW       (tskIDLE_PRIORITY + 1)

#define TASK_STACK_LARGE   (8192)
#define TASK_STACK_MEDIUM  (4096)
#define TASK_STACK_SMALL   (2048)

// Task creation with core pinning
xTaskCreatePinnedToCore(
    network_task,          // Task function
    "network",             // Name
    TASK_STACK_MEDIUM,     // Stack size
    NULL,                  // Parameters
    TASK_PRIORITY_HIGH,    // Priority
    &network_handle,       // Handle
    0                      // Core 0 (network)
);

xTaskCreatePinnedToCore(
    render_task,
    "render",
    TASK_STACK_LARGE,      // Rendering needs more stack
    NULL,
    TASK_PRIORITY_NORMAL,
    &render_handle,
    1                      // Core 1 (compute)
);
```

**Proposed Task Structure**:
| Task | Priority | Stack | Core | Purpose |
|------|----------|-------|------|---------|
| `network_task` | HIGH | 4KB | 0 | UDP receive, queue commands |
| `app_task` | NORMAL | 4KB | 1 | Process commands, update BotiEyes |
| `wifi_monitor` | NORMAL | 2KB | 0 | WiFi event handling |
| `health_monitor` | LOW | 2KB | any | Watchdog feeding, crash detection |
| `idle_task` (built-in) | IDLE | min | both | CPU idle, watchdog feeding |

**Stack Sizing Strategy**:
1. Start with conservative estimate based on call depth and local variables
2. Add 25% margin for safety
3. Monitor via `uxTaskGetStackHighWaterMark()` in production
4. Log warnings if watermark < 512 bytes (too close to overflow)

**Alternatives Considered**:
- **Single task with cooperative scheduling**: Doesn't leverage dual-core, harder to meet real-time deadlines
- **Dynamic priorities**: Unnecessary complexity; static priorities sufficient for this application
- **No core pinning**: Causes cache thrashing, unpredictable performance on SMP

**References**:
- FreeRTOS Task Management: https://www.freertos.org/taskandcr.html
- ESP-IDF Task Watchdog: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html

---

## 4. Hardware Abstraction Patterns

### Decision: Compile-Time HAL with Runtime Board Detection

**Chosen Approach**:
- **Kconfig-based board selection** at build time (e.g., `CONFIG_HAL_BOARD_TTGO_LORA32`)
- **HAL API** abstracts display (I2C/SPI), LED (GPIO/WS2812), pins
- **Board config files** define pin mappings and initialization sequences

**Rationale**:
- **Zero Runtime Overhead**: Board selection at compile time; no runtime conditionals in hot paths
- **Type Safety**: Incorrect configurations caught at build time
- **Multi-Board Support**: Same application code works on different boards
- **Testing**: Can build for multiple boards without hardware; catch portability issues early

**HAL Structure**:
```
hal_board/
├── include/
│   ├── hal_board.h         // Board initialization
│   ├── hal_display.h       // Display abstraction (I2C/SPI agnostic)
│   └── hal_led.h           // LED abstraction (GPIO/WS2812)
├── src/
│   ├── boards/
│   │   ├── ttgo_lora32.c      // TTGO LoRa32 pin config
│   │   └── esp32s3_spi.c      // ESP32-S3 SPI variant
│   ├── hal_display_i2c.c      // I2C implementation
│   ├── hal_display_spi.c      // SPI implementation
│   └── hal_led.cpp            // LED wrapper
└── Kconfig                 // Board selection menu
```

**HAL API Example**:
```cpp
// hal_display.h - Abstract display interface
esp_err_t hal_display_init(void);
void hal_display_clear(void);
void hal_display_draw_pixel(int16_t x, int16_t y, uint16_t color);
void hal_display_refresh(void);
Adafruit_GFX* hal_display_get_gfx(void); // For BotiEyes rendering

// Implementation selected at compile time:
#if CONFIG_BOTIEYES_OLED_PROTOCOL_I2C
    #include "hal_display_i2c.c"
#elif CONFIG_BOTIEYES_OLED_PROTOCOL_SPI
    #include "hal_display_spi.c"
#endif
```

**Board Configuration Pattern**:
```cpp
// boards/ttgo_lora32.c
const hal_board_config_t board_config = {
    .display = {
        .type = HAL_DISPLAY_I2C,
        .i2c = {
            .sda = GPIO_NUM_4,
            .scl = GPIO_NUM_15,
            .freq = 400000,
            .address = 0x3C
        }
    },
    .led = {
        .type = HAL_LED_GPIO,
        .gpio = {
            .pin = GPIO_NUM_2,
            .active_high = true
        }
    }
};
```

**Alternatives Considered**:
- **Runtime board detection**: Adds code size, slower, harder to debug; compile-time is sufficient
- **Preprocessor macros everywhere**: Unmaintainable; HAL centralizes board differences
- **Separate firmware per board**: Duplicates code; HAL enables code sharing

**References**:
- ESP-IDF Kconfig: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html
- Hardware Abstraction Best Practices: https://embeddedartistry.com/blog/2017/02/17/hardware-abstraction-layers/

---

## 5. Configuration Management

### Decision: Kconfig (Build-Time) + NVS (Runtime)

**Chosen Approach**:
- **Kconfig**: Board type, display interface, optional features, log levels (immutable after build)
- **NVS**: WiFi credentials, user preferences, operational parameters (changeable at runtime)

**Rationale**:
- **Separation of Concerns**: Hardware config (build-time) vs user config (runtime)
- **Flexibility**: Same firmware binary can be configured for different deployments
- **Persistence**: NVS survives reboots, power loss; stored in flash
- **Atomicity**: NVS handles flash wear leveling and power-fail safety

**Kconfig Usage** (build-time):
```kconfig
config BOTIEYES_OLED_PROTOCOL_I2C
    bool "I2C"
    help
        Use I2C interface for OLED display.

config BOTIEYES_FRAME_INTERVAL_MS
    int "Render frame interval (ms)"
    range 10 1000
    default 40
```

**NVS Usage** (runtime):
```cpp
// config_manager component API
esp_err_t config_get_wifi_ssid(char *ssid, size_t len);
esp_err_t config_set_wifi_ssid(const char *ssid);
esp_err_t config_factory_reset(void); // Clear all user settings
```

**NVS Namespace Strategy**:
- `botieyes_wifi`: WiFi credentials (SSID, password)
- `botieyes_app`: Application settings (brightness, idle timeout)
- `botieyes_sys`: System data (crash logs, boot count)

**Error Handling**:
- NVS corruption: Erase and reinitialize with factory defaults
- NVS full: Log error, fall back to built-in defaults (from Kconfig)
- Missing keys: Return default values, don't error

**Alternatives Considered**:
- **All runtime (NVS only)**: Too flexible; board-specific config should be immutable
- **All build-time (Kconfig only)**: Requires reflashing to change WiFi credentials; unacceptable
- **Configuration files on filesystem**: Adds complexity (FatFS/LittleFS), NVS simpler for key-value

**References**:
- ESP-IDF NVS: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/storage/nvs_flash.html
- Kconfig Configuration: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/kconfig.html

---

## 6. System Health & Recovery

### Decision: Task Watchdog + Crash Logging + Boot Loop Detection

**Chosen Approach**:
- **Task Watchdog**: 30-second timeout for critical tasks; triggers reset on hang
- **Crash Logging**: Store panic info in NVS for post-mortem analysis
- **Boot Loop Detection**: 3+ crashes in 60 seconds → enter safe mode

**Rationale**:
- **Unattended Operation**: Device must self-recover from software faults
- **Debuggability**: Crash logs enable remote diagnosis without JTAG
- **Fail-Safe**: Boot loop detection prevents infinite restart cycles

**Watchdog Pattern**:
```cpp
// health_monitor component
esp_err_t health_monitor_register_task(TaskHandle_t task, const char *name);

// In each critical task:
void critical_task(void *arg) {
    health_monitor_register_task(xTaskGetCurrentTaskHandle(), "critical");
    
    while (1) {
        // Do work
        process_something();
        
        // Feed watchdog (proves liveness)
        esp_task_wdt_reset();
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

**Crash Logging Strategy**:
```cpp
// On boot, check for previous crash
esp_reset_reason_t reason = esp_reset_reason();
if (reason == ESP_RST_PANIC || reason == ESP_RST_TASK_WDT) {
    // Read crash info from coredump/RTC memory
    crash_log_t log;
    crash_log_read(&log);
    ESP_LOGE(TAG, "Previous crash: reason=%d, task=%s, PC=0x%08x", 
             log.reason, log.task_name, log.pc);
    
    // Store in NVS for later retrieval
    nvs_set_blob(nvs_handle, "last_crash", &log, sizeof(log));
}
```

**Boot Loop Detection**:
```cpp
// Increment boot counter in RTC memory (survives reboot)
uint32_t boot_count = rtc_get_boot_count();
uint32_t boot_time = esp_timer_get_time() / 1000000; // seconds

if (boot_count >= 3 && boot_time < 60) {
    // 3+ boots in 60 seconds = boot loop
    ESP_LOGE(TAG, "Boot loop detected! Entering safe mode.");
    enter_safe_mode(); // Minimal services, no WiFi, no app
}

// Reset counter after 60 seconds of stable operation
if (boot_time > 60) {
    rtc_set_boot_count(0);
}
```

**Safe Mode Behavior**:
- Display shows "SAFE MODE" message
- WiFi disabled
- Network control disabled
- Serial console accepts factory reset command

**Alternatives Considered**:
- **No watchdog**: Unacceptable for production; device can hang indefinitely
- **External hardware watchdog**: Adds BOM cost; task watchdog sufficient for ESP32
- **Remote monitoring only**: Requires network; watchdog provides local recovery

**References**:
- ESP-IDF Task Watchdog: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/wdts.html
- Core Dump: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/core_dump.html

---

## 7. Logging Strategies

### Decision: Hierarchical Tags + Runtime Level Control

**Chosen Approach**:
- **Hierarchical tags**: `"LAYER:COMPONENT:MODULE"` (e.g., `"SVC:WIFI:MGR"`)
- **Runtime control**: `esp_log_level_set("SVC:WIFI:*", ESP_LOG_DEBUG)`
- **Compile-time defaults**: Kconfig sets default levels per component

**Rationale**:
- **Filtering**: Enable verbose logs for one subsystem without noise from others
- **Production**: Reduce log verbosity without recompiling
- **Debugging**: Quickly zoom in on problematic areas
- **Performance**: Compile-time level removes log calls from binary when disabled

**Tag Hierarchy**:
```
APP:*              Application layer
  APP:MAIN         Main task
  APP:INIT         Initialization
SVC:*              Service layer
  SVC:WIFI:MGR     WiFi manager
  SVC:WIFI:EVT     WiFi events
  SVC:STATE        State machine
  SVC:CONFIG       Config manager
  SVC:HEALTH       Health monitor
HAL:*              Hardware abstraction
  HAL:BOARD        Board init
  HAL:DISPLAY      Display driver
  HAL:LED          LED control
NET:*              Network layer
  NET:UDP          UDP server
  NET:CMD          Command processing
```

**Logging Best Practices**:
```cpp
static const char *TAG = "SVC:WIFI:MGR";

// ERROR: Always logged in production; includes context
ESP_LOGE(TAG, "Failed to connect: ssid=%s, retries=%d, reason=%d", 
         ssid, retry_count, reason);

// WARN: Recoverable issues
ESP_LOGW(TAG, "Signal weak: rssi=%d dBm", rssi);

// INFO: Important state changes
ESP_LOGI(TAG, "Connected: ip=%s, gateway=%s", ip, gateway);

// DEBUG: Detailed execution flow (disabled in production)
ESP_LOGD(TAG, "Scanning for networks: channels=%d", num_channels);

// VERBOSE: Very detailed (development only)
ESP_LOGV(TAG, "RX packet: len=%d, src=%s:%d", len, ip, port);
```

**Runtime Control**:
```cpp
// Enable debug logs for WiFi only
esp_log_level_set("SVC:WIFI:*", ESP_LOG_DEBUG);

// Silence verbose HAL logs
esp_log_level_set("HAL:*", ESP_LOG_INFO);

// Or via serial console command:
// > log_level SVC:WIFI:MGR DEBUG
```

**Alternatives Considered**:
- **Flat tags**: Hard to filter; "wifi_manager" vs "wifi_event_handler" not hierarchical
- **Numerical levels only**: Less intuitive than ERROR/WARN/INFO/DEBUG/VERBOSE
- **Printf debugging**: No filtering, no runtime control, clutters serial output

**References**:
- ESP-IDF Logging: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html

---

## 8. Performance Considerations

### Decision: Profile-Guided Optimization with Monitoring

**Chosen Approach**:
- **Baseline measurement**: Profile current firmware (frame time, latency, memory)
- **Accept ≤10% regression**: Architectural improvements worth modest performance cost
- **Optimize hot paths**: Keep rendering on dedicated core; minimize queue/mutex latency
- **Monitor production**: Log stack watermarks, heap usage, task CPU time

**Key Metrics**:
| Metric | Target | Measurement |
|--------|--------|-------------|
| Frame time | 40ms (25 FPS) | `esp_timer_get_time()` before/after `eyes->update()` |
| WiFi event latency | <20ms | Timestamp in event handler vs processing |
| Network command latency | <10ms | UDP receive timestamp vs BotiEyes apply |
| Heap usage | <80% | `esp_get_free_heap_size()` / total heap |
| Stack watermark | >512 bytes free | `uxTaskGetStackHighWaterMark()` |

**Optimization Strategies**:
1. **Queue sizing**: Small queues (5-10 items) for bounded latency; monitor overflow
2. **Mutex timeout**: Short timeouts (100ms) to prevent deadlocks; log failures
3. **Task priorities**: Critical tasks preempt lower-priority; no priority inversion
4. **Core pinning**: Rendering on core 1; network on core 0 (cache locality)
5. **Task notifications over semaphores**: Lower overhead for 1:1 signaling

**Profiling Tools**:
- `esp_timer_get_time()`: Microsecond-resolution timing
- `uxTaskGetStackHighWaterMark()`: Stack usage per task
- `heap_caps_print_heap_info()`: Heap fragmentation analysis
- FreeRTOS trace hooks: Task execution time (if needed)

**Acceptance Criteria** (from Constitution Check):
- Frame rate: ≥22.5 FPS (≤10% regression from 25 FPS)
- WiFi event latency: <20ms (P95)
- Network command latency: <10ms (P95)
- Memory usage: <80% heap, all tasks >512 bytes stack free

**Alternatives Considered**:
- **Ignore performance**: Risks breaking real-time requirements; animation smoothness critical
- **Premature optimization**: Don't optimize before measuring; profile first
- **Custom RTOS**: FreeRTOS well-tuned for ESP32; custom RTOS unnecessary

**References**:
- ESP-IDF Performance: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/performance/index.html
- FreeRTOS Optimization: https://www.freertos.org/FAQPerf.html

---

## Summary of Key Decisions

| Area | Decision | Rationale |
|------|----------|-----------|
| **Architecture** | Layered ESP-IDF components | Modularity, testability, industry standard |
| **Communication** | ESP event loop + FreeRTOS queues | Decoupling, performance, standard pattern |
| **Task Management** | Priority + core pinning | Real-time guarantees, cache locality |
| **Hardware Abstraction** | Compile-time HAL | Zero overhead, type safety, multi-board |
| **Configuration** | Kconfig (build) + NVS (runtime) | Flexibility, persistence, separation |
| **Health** | Watchdog + crash logging + safe mode | Self-recovery, debuggability, fail-safe |
| **Logging** | Hierarchical tags + runtime control | Filtering, production-ready, debuggable |
| **Performance** | Profile-guided with monitoring | Data-driven, acceptance criteria |

---

## Open Questions / Future Research

None identified. All technical context from specification has been researched and resolved.

---

**Research Complete**: Ready to proceed to Phase 1 (Design).
