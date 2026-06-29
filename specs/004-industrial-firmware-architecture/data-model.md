# Data Model: Industrial Firmware Architecture

**Feature**: 004-industrial-firmware-architecture  
**Date**: 2026-06-30  
**Purpose**: Define key entities, data structures, and relationships for refactored ESP-IDF firmware

---

## Overview

This document describes the data structures, state machines, and relationships between components in the refactored BotiEyes ESP-IDF firmware. The architecture follows industrial embedded patterns with clear separation between configuration data (build-time vs runtime), operational state, and synchronization primitives.

---

## 1. WiFi Manager Service

### Purpose
Manages WiFi connectivity lifecycle: connection, reconnection, status monitoring, event propagation.

### Data Structures

#### `wifi_manager_config_t`
**Stored**: NVS (`botieyes_wifi` namespace)  
**Lifecycle**: Runtime-configurable, survives reboots

```c
typedef struct {
    char ssid[33];              // WiFi SSID (32 chars + null)
    char password[65];          // WPA2 password (64 chars + null)
    uint8_t max_retry;          // Max connection retries (default: 5)
    uint32_t retry_delay_ms;    // Initial retry delay (default: 1000)
    bool auto_reconnect;        // Enable auto-reconnection (default: true)
} wifi_manager_config_t;
```

#### `wifi_manager_state_t`
**Stored**: RAM (volatile)  
**Lifecycle**: Runtime state, reset on reboot

```c
typedef enum {
    WIFI_MGR_IDLE,              // Not initialized
    WIFI_MGR_CONNECTING,        // Connection in progress
    WIFI_MGR_CONNECTED,         // Connected, have IP
    WIFI_MGR_DISCONNECTED,      // Disconnected, will retry
    WIFI_MGR_FAILED             // Failed after max retries
} wifi_manager_status_t;

typedef struct {
    wifi_manager_status_t status;
    esp_netif_ip_info_t ip_info;    // IP, netmask, gateway
    uint8_t retry_count;            // Current retry attempt
    int8_t rssi;                    // Signal strength (dBm)
    uint32_t connected_time_sec;    // Uptime since connection
} wifi_manager_state_t;
```

### Events

**Posted to ESP Event Loop**:
- `WIFI_MGR_EVENT:WIFI_MGR_CONNECTED` - Connection successful, IP acquired
- `WIFI_MGR_EVENT:WIFI_MGR_DISCONNECTED` - Connection lost
- `WIFI_MGR_EVENT:WIFI_MGR_FAILED` - Max retries exceeded

**Subscribed**:
- `WIFI_EVENT:*` - ESP-IDF WiFi events (connect, disconnect, scan done)
- `IP_EVENT:IP_EVENT_STA_GOT_IP` - IP address acquired

### API

```c
// Initialization
esp_err_t wifi_manager_init(void);

// Configuration
esp_err_t wifi_manager_set_credentials(const char *ssid, const char *password);
esp_err_t wifi_manager_get_credentials(char *ssid, size_t ssid_len, 
                                        char *password, size_t password_len);

// Control
esp_err_t wifi_manager_connect(void);
esp_err_t wifi_manager_disconnect(void);

// Status query
wifi_manager_state_t wifi_manager_get_state(void);
const char* wifi_manager_get_ip_address(void);  // Returns string or NULL
```

### Dependencies
- **NVS**: Credential storage
- **ESP WiFi**: Network connectivity
- **ESP Event Loop**: Event handling
- **State Machine**: Optional integration for app-level state tracking

---

## 2. Application State Machine

### Purpose
Centralized management of application lifecycle states with validated transitions and event notification.

### Data Structures

#### `app_state_t`
**Stored**: RAM (protected by mutex)  
**Lifecycle**: Runtime state

```c
typedef enum {
    APP_STATE_INIT,             // Initializing components
    APP_STATE_CONNECTING,       // Waiting for WiFi
    APP_STATE_CONNECTED,        // WiFi connected, no network control
    APP_STATE_RUNNING,          // Network control active
    APP_STATE_ERROR,            // Recoverable error state
    APP_STATE_SAFE_MODE,        // Boot loop detected, minimal services
    APP_STATE_SHUTDOWN          // Graceful shutdown in progress
} app_state_t;

typedef struct {
    app_state_t current;
    app_state_t previous;
    uint32_t transition_time_ms;    // Timestamp of last transition
    uint32_t state_duration_ms;     // Time in current state
    char error_message[128];        // Error context (if in ERROR state)
} app_state_info_t;
```

### State Transition Rules

Valid transitions (enforced by state machine):
```
INIT → CONNECTING | ERROR
CONNECTING → CONNECTED | ERROR | CONNECTING (retry)
CONNECTED → RUNNING | CONNECTING (disconnect) | ERROR
RUNNING → CONNECTED (network stopped) | CONNECTING | ERROR
ERROR → CONNECTING (recovery) | SAFE_MODE | SHUTDOWN
SAFE_MODE → INIT (manual reset) | SHUTDOWN
SHUTDOWN → (reboot)
```

### Events

**Posted to ESP Event Loop**:
- `APP_STATE_EVENT:STATE_CHANGED` - State transition occurred (payload: `app_state_info_t`)

### API

```c
// Initialization
esp_err_t app_state_init(void);

// State transitions (validated)
esp_err_t app_state_transition(app_state_t new_state, const char *reason);

// State query (thread-safe)
app_state_t app_state_get_current(void);
app_state_info_t app_state_get_info(void);

// Error handling
esp_err_t app_state_set_error(const char *error_message);
```

### Synchronization
- **Mutex**: Protects `app_state_info_t` from concurrent access
- **Timeout**: 100ms (state transitions are fast)

---

## 3. Hardware Abstraction Layer (HAL)

### Purpose
Abstract board-specific hardware differences (display interface, LED type, pin mappings).

### Data Structures

#### `hal_board_config_t`
**Stored**: Compile-time constant (selected via Kconfig)  
**Lifecycle**: Immutable after build

```c
typedef enum {
    HAL_DISPLAY_I2C,
    HAL_DISPLAY_SPI
} hal_display_type_t;

typedef enum {
    HAL_LED_NONE,       // No LED
    HAL_LED_GPIO,       // Simple GPIO LED
    HAL_LED_WS2812      // Addressable RGB LED (via led_strip)
} hal_led_type_t;

typedef struct {
    struct {
        hal_display_type_t type;
        union {
            struct {
                gpio_num_t sda;
                gpio_num_t scl;
                uint32_t freq_hz;
                uint8_t address;
            } i2c;
            struct {
                gpio_num_t mosi;
                gpio_num_t sck;
                gpio_num_t cs;
                gpio_num_t dc;
                gpio_num_t rst;
                uint32_t freq_hz;
            } spi;
        };
    } display;
    
    struct {
        hal_led_type_t type;
        union {
            struct {
                gpio_num_t pin;
                bool active_high;
            } gpio;
            struct {
                gpio_num_t pin;
                uint8_t brightness;
            } ws2812;
        };
    } led;
} hal_board_config_t;
```

### Board Variants

**TTGO LoRa32**:
- Display: I2C (SDA=GPIO4, SCL=GPIO15, addr=0x3C)
- LED: GPIO (pin=GPIO2, active_high=true)

**ESP32-S3 SPI**:
- Display: SPI (MOSI=GPIO11, SCK=GPIO12, CS=GPIO10, DC=GPIO13, RST=GPIO14)
- LED: WS2812 (pin=GPIO38)

### API

```c
// Board initialization (calls display + LED init)
esp_err_t hal_board_init(void);

// Display abstraction
esp_err_t hal_display_init(void);
void hal_display_clear(void);
void hal_display_refresh(void);
Adafruit_GFX* hal_display_get_gfx(void);  // For BotiEyes rendering

// LED abstraction
esp_err_t hal_led_init(void);
void hal_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void hal_led_off(void);
```

---

## 4. Configuration Manager

### Purpose
Abstraction over NVS for storing/retrieving runtime configuration with error handling.

### NVS Namespace Organization

| Namespace | Keys | Purpose |
|-----------|------|---------|
| `botieyes_wifi` | `ssid`, `password`, `auto_reconnect` | WiFi credentials |
| `botieyes_app` | `brightness`, `idle_timeout_s`, `network_enabled` | App settings |
| `botieyes_sys` | `boot_count`, `last_crash`, `factory_reset_count` | System data |

### Data Structures

#### `config_wifi_t`
```c
typedef struct {
    char ssid[33];
    char password[65];
    bool auto_reconnect;
} config_wifi_t;
```

#### `config_app_t`
```c
typedef struct {
    uint8_t brightness;         // Display brightness (0-255)
    uint32_t idle_timeout_s;    // Network idle timeout (seconds)
    bool network_enabled;       // Enable network control
} config_app_t;
```

#### `crash_log_t`
```c
typedef struct {
    esp_reset_reason_t reason;
    char task_name[16];
    uint32_t pc;                // Program counter at crash
    uint32_t timestamp;         // Unix timestamp
    char description[64];
} crash_log_t;
```

### API

```c
// Initialization
esp_err_t config_manager_init(void);

// WiFi configuration
esp_err_t config_get_wifi(config_wifi_t *cfg);
esp_err_t config_set_wifi(const config_wifi_t *cfg);

// Application configuration
esp_err_t config_get_app(config_app_t *cfg);
esp_err_t config_set_app(const config_app_t *cfg);

// System data
esp_err_t config_get_crash_log(crash_log_t *log);
esp_err_t config_set_crash_log(const crash_log_t *log);

// Factory reset
esp_err_t config_factory_reset(void);  // Erase all user data
```

### Error Handling
- **NVS corruption**: Erase namespace, return `ESP_ERR_NVS_INVALID_STATE`
- **Key not found**: Return default values (defined in Kconfig)
- **NVS full**: Log error, attempt compaction

---

## 5. Task Registry

### Purpose
Track created tasks for monitoring (stack watermarks, CPU usage, watchdog subscription).

### Data Structures

#### `task_info_t`
```c
typedef struct {
    TaskHandle_t handle;
    char name[16];
    UBaseType_t priority;
    uint32_t stack_size;
    BaseType_t core_id;         // Core affinity (-1 = any)
    UBaseType_t stack_watermark; // Minimum free stack (bytes)
    bool watchdog_subscribed;
} task_info_t;
```

#### `task_registry_t`
```c
#define MAX_TASKS 10

typedef struct {
    task_info_t tasks[MAX_TASKS];
    uint8_t count;
    SemaphoreHandle_t mutex;    // Protect concurrent access
} task_registry_t;
```

### API

```c
// Internal use by health_monitor component
esp_err_t task_registry_init(void);
esp_err_t task_registry_add(TaskHandle_t handle, const char *name, 
                             UBaseType_t priority, uint32_t stack_size, 
                             BaseType_t core_id);
esp_err_t task_registry_update_watermark(TaskHandle_t handle, UBaseType_t watermark);
task_info_t* task_registry_find(TaskHandle_t handle);
void task_registry_print_all(void);  // Debug output
```

---

## 6. Health Monitor

### Purpose
Watchdog management, crash detection, boot loop prevention, task monitoring.

### Data Structures

#### `health_status_t`
```c
typedef struct {
    uint32_t boot_count;            // Boots since power-on (RTC memory)
    uint32_t uptime_sec;            // Seconds since boot
    uint32_t last_crash_time;       // Unix timestamp of last crash
    bool safe_mode;                 // True if in safe mode
    uint8_t registered_tasks;       // Count of watchdog-monitored tasks
} health_status_t;
```

### API

```c
// Initialization
esp_err_t health_monitor_init(void);

// Watchdog subscription
esp_err_t health_monitor_register_task(TaskHandle_t task, const char *name);

// Crash handling
void health_monitor_on_boot(void);  // Check for boot loop, log previous crash

// Status
health_status_t health_monitor_get_status(void);
bool health_monitor_is_safe_mode(void);
```

### Boot Loop Detection Logic

```c
// Stored in RTC memory (survives reboot, cleared on power cycle)
typedef struct {
    uint32_t boot_count;
    uint64_t first_boot_time_us;
} rtc_boot_data_t;

// On boot:
// 1. Increment boot_count
// 2. If boot_count == 1, store first_boot_time
// 3. If boot_count >= 3 && (current_time - first_boot_time) < 60s:
//    → Enter safe mode
// 4. After 60s of stable operation, reset boot_count to 0
```

---

## 7. Display Driver (HAL)

### Purpose
Unified API for I2C/SPI SSD1306 display with mutex-protected access.

### Data Structures

#### Display Handle (opaque)
```c
// Internal to hal_display component
typedef struct {
    Adafruit_SSD1306 *gfx;      // Adafruit GFX instance
    SemaphoreHandle_t mutex;    // Protect hardware access
    hal_display_type_t type;    // I2C or SPI
} display_handle_t;
```

### Synchronization
- **Mutex**: Prevents concurrent display access (SPI/I2C hardware constraint)
- **Timeout**: 100ms (frame rendering ~10-20ms; longer timeout for safety)

### API

```c
// Initialization (called by hal_board_init)
esp_err_t hal_display_init(void);

// Thread-safe drawing (acquires mutex)
esp_err_t hal_display_clear(void);
esp_err_t hal_display_refresh(void);

// Get GFX handle for batch operations (caller must lock/unlock)
Adafruit_GFX* hal_display_get_gfx(void);
esp_err_t hal_display_lock(TickType_t timeout);
void hal_display_unlock(void);
```

---

## 8. Network Command Queue

### Purpose
Pass UDP control commands from network task (core 0) to application task (core 1).

### Data Structures

#### `network_cmd_t`
```c
#define NETWORK_CMD_DATA_SIZE 64

typedef enum {
    CMD_SET_EMOTION,        // Set emotion (valence, arousal)
    CMD_SET_POSITION,       // Set eye position (x, y)
    CMD_IDLE_MODE,          // Enter idle animation
    CMD_PING,               // Heartbeat/keepalive
    CMD_RESET               // Reset to defaults
} network_cmd_type_t;

typedef struct {
    network_cmd_type_t type;
    uint8_t data[NETWORK_CMD_DATA_SIZE];  // Command-specific payload
    uint32_t timestamp_ms;                // Receive timestamp
} network_cmd_t;
```

#### Queue Handle
```c
// Global queue (created in main)
#define NETWORK_CMD_QUEUE_SIZE 10
QueueHandle_t g_network_cmd_queue;
```

### Usage Pattern

**Network Task (Producer)**:
```c
void network_task(void *arg) {
    network_cmd_t cmd;
    // Receive UDP packet, decode to cmd
    if (xQueueSend(g_network_cmd_queue, &cmd, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "Command queue full, dropping packet");
    }
}
```

**Application Task (Consumer)**:
```c
void app_task(void *arg) {
    network_cmd_t cmd;
    while (1) {
        if (xQueueReceive(g_network_cmd_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            process_command(&cmd);
        }
    }
}
```

---

## 9. Synchronization Primitives

### Summary of Usage

| Primitive | Type | Protects | Timeout | Created By |
|-----------|------|----------|---------|------------|
| `g_display_mutex` | Mutex | Display hardware (SPI/I2C) | 100ms | `hal_display_init()` |
| `g_state_mutex` | Mutex | `app_state_info_t` structure | 100ms | `app_state_init()` |
| `g_task_registry_mutex` | Mutex | Task registry array | 50ms | `task_registry_init()` |
| `g_network_cmd_queue` | Queue | Network command transfer | 100ms send, 100ms receive | `app_main()` |

### Design Guidelines

1. **Mutexes**: Use for protecting shared data structures (display, state, registry)
2. **Queues**: Use for inter-task data transfer (network commands)
3. **Task Notifications**: Use for simple signaling (future optimization)
4. **Event Groups**: Use for multi-condition wait (initialization only)

---

## 10. System Initialization Sequence

### Boot Flow

```
app_main() [main task]
  ├─ nvs_flash_init()
  ├─ esp_event_loop_create_default()
  │
  ├─ config_manager_init()       // Load NVS config
  ├─ app_state_init()             // Initialize state machine → INIT
  ├─ health_monitor_on_boot()     // Check for boot loop / previous crash
  │
  ├─ hal_board_init()
  │   ├─ hal_display_init()       // Initialize display + mutex
  │   └─ hal_led_init()
  │
  ├─ wifi_manager_init()          // Load WiFi config from NVS
  ├─ app_state_transition(CONNECTING)
  ├─ wifi_manager_connect()       // Start WiFi connection
  │
  ├─ Create network_cmd_queue
  ├─ xTaskCreatePinnedToCore(network_task, ..., core=0)
  ├─ xTaskCreatePinnedToCore(app_task, ..., core=1)
  │
  └─ health_monitor_init()        // Start watchdog monitoring
  
[Event-driven operation begins]

WiFi Connected → WIFI_MGR_EVENT:CONNECTED
  ├─ app_state_transition(CONNECTED)
  └─ Start network server (UDP)

Network Control Active
  └─ app_state_transition(RUNNING)
```

---

## 11. Component Dependency Graph

```
Application Layer (main/)
    ↓ uses
Service Layer (components/)
    ├─ wifi_manager ──→ esp_wifi, esp_event, config_manager
    ├─ state_machine ──→ esp_event
    ├─ config_manager ──→ nvs_flash
    └─ health_monitor ──→ esp_system, nvs_flash, task_registry
    ↓ uses
HAL Layer (hal_board/)
    ├─ hal_display ──→ Adafruit_GFX, esp_spi/i2c
    └─ hal_led ──→ led_strip, gpio
    ↓ uses
Driver Layer
    ├─ Adafruit_GFX (managed component)
    ├─ led_strip (managed component)
    └─ ESP-IDF drivers (built-in)
```

---

## Summary

This data model defines:
1. **Configuration Data**: Kconfig (build-time) + NVS (runtime)
2. **Operational State**: WiFi status, app state, task registry, health status
3. **Communication**: Queues for commands, ESP event loop for system events
4. **Synchronization**: Mutexes for display, state machine, registry
5. **Hardware Abstraction**: Board configs, display/LED APIs
6. **Lifecycle**: Boot sequence, state transitions, error recovery

All entities follow ESP-IDF conventions and industrial embedded patterns for reliability, maintainability, and testability.
