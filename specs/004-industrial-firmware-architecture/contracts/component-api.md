# Component API Contracts

**Feature**: 004-industrial-firmware-architecture  
**Date**: 2026-06-30  
**Purpose**: Define public API contracts for ESP-IDF components

---

## Overview

This document specifies the public interfaces (contracts) for each ESP-IDF component in the refactored firmware. These are the stable APIs that other components and application code depend on. Changes to these interfaces require careful consideration and versioning.

---

## 1. WiFi Manager (`wifi_manager`)

### Component Information
- **Path**: `esp-idf/components/wifi_manager/`
- **Header**: `<wifi_manager.h>`
- **Dependencies**: `esp_wifi`, `esp_event`, `esp_netif`, `nvs_flash`, `config_manager`

### Public Types

```c
// WiFi connection status
typedef enum {
    WIFI_MGR_IDLE,          // Not initialized
    WIFI_MGR_CONNECTING,    // Connection in progress
    WIFI_MGR_CONNECTED,     // Connected and have IP
    WIFI_MGR_DISCONNECTED,  // Lost connection, will retry
    WIFI_MGR_FAILED         // Failed after max retries
} wifi_manager_status_t;

// WiFi state information
typedef struct {
    wifi_manager_status_t status;
    esp_netif_ip_info_t ip_info;    // IP address, netmask, gateway
    uint8_t retry_count;             // Current retry attempt
    int8_t rssi;                     // Signal strength (dBm)
    uint32_t connected_time_sec;     // Duration of current connection
} wifi_manager_state_t;
```

### Custom Events

Event base: `WIFI_MGR_EVENT`

| Event ID | Payload | Description |
|----------|---------|-------------|
| `WIFI_MGR_CONNECTED` | `esp_netif_ip_info_t*` | Connection successful, IP acquired |
| `WIFI_MGR_DISCONNECTED` | `NULL` | Connection lost |
| `WIFI_MGR_FAILED` | `NULL` | Max retries exceeded |

**Usage**:
```c
ESP_ERROR_CHECK(esp_event_handler_register(
    WIFI_MGR_EVENT, 
    WIFI_MGR_CONNECTED,
    &my_handler, 
    NULL
));
```

### Public Functions

#### Initialization

```c
/**
 * @brief Initialize WiFi manager
 * 
 * Creates WiFi netif, initializes WiFi driver, registers event handlers,
 * loads credentials from NVS (via config_manager).
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if allocation fails
 *         ESP_FAIL if WiFi init fails
 * 
 * @note Must be called after nvs_flash_init() and esp_event_loop_create_default()
 * @note Not thread-safe, call from main task only
 */
esp_err_t wifi_manager_init(void);
```

#### Configuration

```c
/**
 * @brief Set WiFi credentials (SSID and password)
 * 
 * Stores credentials in NVS via config_manager. Does NOT initiate connection.
 * 
 * @param ssid WiFi SSID (max 32 chars, null-terminated)
 * @param password WiFi password (max 64 chars, null-terminated)
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if ssid/password NULL or too long
 *         ESP_ERR_NVS_* if NVS write fails
 * 
 * @note Thread-safe
 */
esp_err_t wifi_manager_set_credentials(const char *ssid, const char *password);

/**
 * @brief Get WiFi credentials from NVS
 * 
 * @param[out] ssid Buffer for SSID (min 33 bytes)
 * @param ssid_len Size of ssid buffer
 * @param[out] password Buffer for password (min 65 bytes)
 * @param password_len Size of password buffer
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if buffers NULL or too small
 *         ESP_ERR_NVS_NOT_FOUND if credentials not set
 * 
 * @note Thread-safe
 */
esp_err_t wifi_manager_get_credentials(char *ssid, size_t ssid_len,
                                        char *password, size_t password_len);
```

#### Connection Control

```c
/**
 * @brief Start WiFi connection
 * 
 * Begins connection attempt using stored credentials. Returns immediately.
 * Connection progress reported via WIFI_MGR_EVENT events.
 * 
 * @return ESP_OK if connection attempt started
 *         ESP_ERR_INVALID_STATE if already connected or connecting
 *         ESP_ERR_NVS_NOT_FOUND if credentials not configured
 * 
 * @note Non-blocking, use events or wifi_manager_get_state() to monitor progress
 * @note Thread-safe
 */
esp_err_t wifi_manager_connect(void);

/**
 * @brief Disconnect from WiFi
 * 
 * Stops any ongoing connection attempt and disconnects if connected.
 * Auto-reconnect is suspended until wifi_manager_connect() called again.
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_STATE if not connected
 * 
 * @note Blocking, waits for disconnection to complete (max 5 seconds)
 * @note Thread-safe
 */
esp_err_t wifi_manager_disconnect(void);
```

#### Status Query

```c
/**
 * @brief Get current WiFi manager state
 * 
 * Returns snapshot of current connection state, IP info, signal strength, etc.
 * 
 * @return Current state structure (by value)
 * 
 * @note Thread-safe (protected by internal mutex)
 * @note Non-blocking
 */
wifi_manager_state_t wifi_manager_get_state(void);

/**
 * @brief Get IP address as string
 * 
 * @return Pointer to static string "192.168.1.100" or NULL if not connected
 * 
 * @note Thread-safe
 * @note Returned pointer valid until next call from any thread
 */
const char* wifi_manager_get_ip_address(void);
```

---

## 2. Application State Machine (`state_machine`)

### Component Information
- **Path**: `esp-idf/components/state_machine/`
- **Header**: `<app_state.h>`
- **Dependencies**: `esp_event`, `esp_system`

### Public Types

```c
// Application lifecycle states
typedef enum {
    APP_STATE_INIT,         // Initializing components
    APP_STATE_CONNECTING,   // Waiting for WiFi
    APP_STATE_CONNECTED,    // WiFi connected, idle
    APP_STATE_RUNNING,      // Network control active
    APP_STATE_ERROR,        // Recoverable error
    APP_STATE_SAFE_MODE,    // Boot loop detected
    APP_STATE_SHUTDOWN      // Graceful shutdown
} app_state_t;

// Extended state information
typedef struct {
    app_state_t current;
    app_state_t previous;
    uint32_t transition_time_ms;    // Timestamp of last transition
    uint32_t state_duration_ms;     // Time in current state
    char error_message[128];        // Error context (if ERROR state)
} app_state_info_t;
```

### Custom Events

Event base: `APP_STATE_EVENT`

| Event ID | Payload | Description |
|----------|---------|-------------|
| `STATE_CHANGED` | `app_state_info_t*` | State transition occurred |

### Public Functions

```c
/**
 * @brief Initialize state machine
 * 
 * Creates mutex, initializes to INIT state, posts initial STATE_CHANGED event.
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NO_MEM if mutex creation fails
 * 
 * @note Must be called after esp_event_loop_create_default()
 * @note Not thread-safe, call from main task only
 */
esp_err_t app_state_init(void);

/**
 * @brief Transition to new state
 * 
 * Validates transition (per state machine rules), updates state, posts event.
 * Invalid transitions are rejected and logged.
 * 
 * @param new_state Target state
 * @param reason Human-readable reason for transition (max 127 chars, can be NULL)
 * 
 * @return ESP_OK if transition allowed and completed
 *         ESP_ERR_INVALID_STATE if transition not allowed
 * 
 * @note Thread-safe (mutex-protected)
 * @note Blocks briefly to acquire mutex and post event
 */
esp_err_t app_state_transition(app_state_t new_state, const char *reason);

/**
 * @brief Get current state
 * 
 * @return Current state enum value
 * 
 * @note Thread-safe (mutex-protected)
 * @note Non-blocking (mutex timeout: 100ms)
 */
app_state_t app_state_get_current(void);

/**
 * @brief Get extended state information
 * 
 * @return State info structure (by value)
 * 
 * @note Thread-safe (mutex-protected)
 * @note Non-blocking (mutex timeout: 100ms)
 */
app_state_info_t app_state_get_info(void);

/**
 * @brief Set error state with message
 * 
 * Convenience function: transitions to ERROR state and stores error message.
 * 
 * @param error_message Error description (max 127 chars)
 * 
 * @return ESP_OK on success
 * 
 * @note Thread-safe
 */
esp_err_t app_state_set_error(const char *error_message);
```

**Valid Transition Rules**:
```
INIT → CONNECTING | ERROR
CONNECTING → CONNECTED | ERROR | CONNECTING (retry)
CONNECTED → RUNNING | CONNECTING (disconnect) | ERROR
RUNNING → CONNECTED (stop) | CONNECTING | ERROR
ERROR → CONNECTING (recovery) | SAFE_MODE | SHUTDOWN
SAFE_MODE → INIT (reset) | SHUTDOWN
SHUTDOWN → (reboot)
```

---

## 3. Configuration Manager (`config_manager`)

### Component Information
- **Path**: `esp-idf/components/config_manager/`
- **Header**: `<config_manager.h>`
- **Dependencies**: `nvs_flash`

### Public Types

```c
// WiFi configuration
typedef struct {
    char ssid[33];              // SSID (32 + null)
    char password[65];          // Password (64 + null)
    bool auto_reconnect;        // Enable auto-reconnect
} config_wifi_t;

// Application configuration
typedef struct {
    uint8_t brightness;         // Display brightness (0-255)
    uint32_t idle_timeout_s;    // Network idle timeout (seconds)
    bool network_enabled;       // Enable network control
} config_app_t;

// Crash log entry
typedef struct {
    esp_reset_reason_t reason;
    char task_name[16];
    uint32_t pc;                // Program counter
    uint32_t timestamp;         // Unix timestamp
    char description[64];
} crash_log_t;
```

### Public Functions

#### Initialization

```c
/**
 * @brief Initialize config manager
 * 
 * Initializes NVS, creates/opens namespaces.
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NVS_NO_FREE_PAGES if NVS full (call nvs_flash_erase)
 *         ESP_ERR_NVS_NEW_VERSION_FOUND if NVS format changed
 * 
 * @note Must be called after nvs_flash_init()
 * @note Not thread-safe, call from main task only
 */
esp_err_t config_manager_init(void);
```

#### WiFi Configuration

```c
/**
 * @brief Get WiFi configuration from NVS
 * 
 * @param[out] cfg WiFi configuration structure
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NVS_NOT_FOUND if not configured (returns defaults)
 *         ESP_ERR_INVALID_ARG if cfg is NULL
 * 
 * @note Thread-safe
 * @note Non-blocking
 */
esp_err_t config_get_wifi(config_wifi_t *cfg);

/**
 * @brief Set WiFi configuration in NVS
 * 
 * @param cfg WiFi configuration structure
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if cfg is NULL or invalid
 *         ESP_ERR_NVS_* on NVS errors
 * 
 * @note Thread-safe
 * @note Blocking (NVS write ~10ms)
 */
esp_err_t config_set_wifi(const config_wifi_t *cfg);
```

#### Application Configuration

```c
/**
 * @brief Get application configuration from NVS
 * 
 * @param[out] cfg Application configuration structure
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NVS_NOT_FOUND if not configured (returns defaults)
 *         ESP_ERR_INVALID_ARG if cfg is NULL
 * 
 * @note Thread-safe
 */
esp_err_t config_get_app(config_app_t *cfg);

/**
 * @brief Set application configuration in NVS
 * 
 * @param cfg Application configuration structure
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if cfg is NULL
 *         ESP_ERR_NVS_* on NVS errors
 * 
 * @note Thread-safe
 */
esp_err_t config_set_app(const config_app_t *cfg);
```

#### System Data

```c
/**
 * @brief Get last crash log from NVS
 * 
 * @param[out] log Crash log structure
 * 
 * @return ESP_OK on success
 *         ESP_ERR_NVS_NOT_FOUND if no crash logged
 *         ESP_ERR_INVALID_ARG if log is NULL
 * 
 * @note Thread-safe
 */
esp_err_t config_get_crash_log(crash_log_t *log);

/**
 * @brief Store crash log in NVS
 * 
 * @param log Crash log structure
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if log is NULL
 *         ESP_ERR_NVS_* on NVS errors
 * 
 * @note Thread-safe
 */
esp_err_t config_set_crash_log(const crash_log_t *log);
```

#### Factory Reset

```c
/**
 * @brief Erase all user configuration
 * 
 * Erases botieyes_wifi and botieyes_app namespaces. System data (crash logs,
 * boot count) are preserved.
 * 
 * @return ESP_OK on success
 *         ESP_FAIL on erase error
 * 
 * @note Thread-safe
 * @note Blocking (NVS erase ~50ms)
 */
esp_err_t config_factory_reset(void);
```

---

## 4. Health Monitor (`health_monitor`)

### Component Information
- **Path**: `esp-idf/components/health_monitor/`
- **Header**: `<health_monitor.h>`
- **Dependencies**: `esp_system`, `esp_task_wdt`, `nvs_flash`, `config_manager`

### Public Types

```c
// System health status
typedef struct {
    uint32_t boot_count;            // Boots since power-on (RTC memory)
    uint32_t uptime_sec;            // Seconds since boot
    uint32_t last_crash_time;       // Unix timestamp of last crash
    bool safe_mode;                 // True if in safe mode
    uint8_t registered_tasks;       // Watchdog-monitored task count
} health_status_t;
```

### Public Functions

```c
/**
 * @brief Initialize health monitor
 * 
 * Configures task watchdog, creates monitoring task.
 * 
 * @return ESP_OK on success
 *         ESP_FAIL if watchdog init fails
 * 
 * @note Must be called after config_manager_init()
 * @note Not thread-safe, call from main task only
 */
esp_err_t health_monitor_init(void);

/**
 * @brief Register task for watchdog monitoring
 * 
 * Subscribes task to task watchdog. Task must call esp_task_wdt_reset()
 * regularly (at least every 30 seconds) to prove liveness.
 * 
 * @param task Task handle to monitor
 * @param name Task name for logging (max 15 chars)
 * 
 * @return ESP_OK on success
 *         ESP_ERR_INVALID_ARG if task is NULL
 *         ESP_FAIL if watchdog subscription fails
 * 
 * @note Thread-safe
 * @note Call from task being registered or from main task
 */
esp_err_t health_monitor_register_task(TaskHandle_t task, const char *name);

/**
 * @brief Boot-time crash check
 * 
 * Checks for boot loop, reads previous crash info, logs to console.
 * If boot loop detected, enters safe mode.
 * 
 * @note Must be called early in app_main(), after config_manager_init()
 * @note Not thread-safe, call from main task only
 */
void health_monitor_on_boot(void);

/**
 * @brief Get system health status
 * 
 * @return Health status structure (by value)
 * 
 * @note Thread-safe
 * @note Non-blocking
 */
health_status_t health_monitor_get_status(void);

/**
 * @brief Check if system is in safe mode
 * 
 * @return true if safe mode active, false otherwise
 * 
 * @note Thread-safe
 */
bool health_monitor_is_safe_mode(void);
```

---

## 5. Hardware Abstraction Layer (`hal_board`)

### Component Information
- **Path**: `esp-idf/components/hal_board/`
- **Headers**: `<hal_board.h>`, `<hal_display.h>`, `<hal_led.h>`
- **Dependencies**: `driver`, `Adafruit_GFX`, `led_strip`

### Public Functions

#### Board Initialization

```c
/**
 * @brief Initialize board hardware
 * 
 * Configures GPIO, initializes display, LED, and other board-specific
 * peripherals. Board variant selected via Kconfig.
 * 
 * @return ESP_OK on success
 *         ESP_FAIL if display or LED init fails
 * 
 * @note Must be called from main task after NVS init
 * @note Not thread-safe
 */
esp_err_t hal_board_init(void);
```

#### Display Abstraction (`hal_display.h`)

```c
/**
 * @brief Initialize display (I2C or SPI based on Kconfig)
 * 
 * @return ESP_OK on success
 *         ESP_FAIL if display init fails
 * 
 * @note Called by hal_board_init(), do not call directly
 */
esp_err_t hal_display_init(void);

/**
 * @brief Clear display buffer
 * 
 * @note Thread-safe (mutex-protected)
 * @note Non-blocking (mutex timeout: 100ms)
 */
void hal_display_clear(void);

/**
 * @brief Refresh display (write buffer to hardware)
 * 
 * @note Thread-safe (mutex-protected)
 * @note Blocking (~10-20ms for full refresh)
 */
void hal_display_refresh(void);

/**
 * @brief Get Adafruit GFX handle for drawing
 * 
 * Returns pointer to internal Adafruit_SSD1306 object for direct rendering.
 * Caller responsible for locking/unlocking display for batch operations.
 * 
 * @return Pointer to Adafruit_GFX instance
 * 
 * @note NOT thread-safe, use hal_display_lock/unlock for exclusive access
 */
Adafruit_GFX* hal_display_get_gfx(void);

/**
 * @brief Lock display for exclusive access
 * 
 * @param timeout Maximum time to wait for lock (ticks)
 * 
 * @return ESP_OK if locked
 *         ESP_ERR_TIMEOUT if timeout expired
 * 
 * @note Must call hal_display_unlock() after drawing
 */
esp_err_t hal_display_lock(TickType_t timeout);

/**
 * @brief Unlock display
 * 
 * @note Must be called after hal_display_lock()
 */
void hal_display_unlock(void);
```

#### LED Abstraction (`hal_led.h`)

```c
/**
 * @brief Initialize status LED
 * 
 * @return ESP_OK on success
 *         ESP_OK if LED disabled (CONFIG_BOTIEYES_STATUS_LED_PIN < 0)
 *         ESP_FAIL if LED init fails
 * 
 * @note Called by hal_board_init(), do not call directly
 */
esp_err_t hal_led_init(void);

/**
 * @brief Set LED color (RGB)
 * 
 * @param r Red intensity (0-255)
 * @param g Green intensity (0-255)
 * @param b Blue intensity (0-255)
 * 
 * @note Thread-safe
 * @note Non-blocking
 * @note No-op if LED disabled
 */
void hal_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief Turn off LED
 * 
 * @note Thread-safe
 * @note Non-blocking
 * @note No-op if LED disabled
 */
void hal_led_off(void);
```

---

## 6. Network Command Queue (Application Contract)

### Queue Handle (Global)

```c
// Defined in main.cpp
extern QueueHandle_t g_network_cmd_queue;
```

### Message Format

```c
#define NETWORK_CMD_DATA_SIZE 64

typedef enum {
    CMD_SET_EMOTION,    // Set emotion (valence, arousal)
    CMD_SET_POSITION,   // Set eye position (x, y)
    CMD_IDLE_MODE,      // Enter idle animation
    CMD_PING,           // Heartbeat
    CMD_RESET           // Reset to defaults
} network_cmd_type_t;

typedef struct {
    network_cmd_type_t type;
    uint8_t data[NETWORK_CMD_DATA_SIZE];
    uint32_t timestamp_ms;
} network_cmd_t;
```

### Usage Contract

**Producer (network task)**:
- Use `xQueueSend(g_network_cmd_queue, &cmd, pdMS_TO_TICKS(100))`
- Handle queue full gracefully (log warning, drop packet)

**Consumer (application task)**:
- Use `xQueueReceive(g_network_cmd_queue, &cmd, pdMS_TO_TICKS(100))`
- Process all commands from queue before next frame update

---

## API Stability Guarantees

### Semantic Versioning

Components follow semantic versioning:
- **Major**: Breaking API changes (function signature, behavior change)
- **Minor**: New features, backward-compatible additions
- **Patch**: Bug fixes, no API changes

### Deprecation Policy

1. Mark function as deprecated in header (with replacement)
2. Log warning on first use
3. Remove in next major version (min 6 months after deprecation)

---

## Error Handling Contract

All components follow ESP-IDF error conventions:
- **`ESP_OK` (0)**: Success
- **`ESP_ERR_*`**: Specific error codes
- **`ESP_FAIL` (-1)**: Generic failure

Functions that can fail MUST return `esp_err_t`. Use `ESP_ERROR_CHECK()` in application code for fatal errors.

---

## Thread Safety Contract

Function thread safety is documented in comments:
- **Thread-safe**: Can be called from multiple tasks concurrently
- **Not thread-safe**: Must be called from single task (usually main task during init)

Components use mutexes with timeouts to prevent deadlocks. Default timeout: 100ms for most operations.

---

## Summary

These component APIs provide:
1. **Stable Interfaces**: Well-defined contracts for inter-component communication
2. **Thread Safety**: Explicit guarantees and mutex protection where needed
3. **Error Handling**: Consistent use of esp_err_t and error codes
4. **Documentation**: Doxygen-style comments for all public functions
5. **Testability**: Components can be tested in isolation via public APIs

All components follow ESP-IDF conventions and industrial embedded best practices.
