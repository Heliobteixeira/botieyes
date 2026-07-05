# Synchronization Primitives - Decision Guide

This document explains when to use each FreeRTOS/ESP-IDF synchronization mechanism in the BotiEyes architecture. **Use the simplest mechanism that solves your problem.**

## Quick Reference Table

| Mechanism | Use When | Example | Overhead |
|-----------|----------|---------|----------|
| **Direct call** | Same component, no task boundary | `static void helper()` | None |
| **Queue** | Pass data between tasks | Network → App task commands | Low |
| **Mutex** | Protect shared resource | Display hardware, state machine | Low |
| **Binary semaphore** | Signal event between tasks | "Work ready", "Transfer done" | Low |
| **Task notification** | Simple 1:1 signaling | Lighter than semaphore | Very low |
| **Event group** | Wait on multiple conditions | WiFi AND display AND config ready | Medium |
| **ESP event loop** | System-wide events, multiple observers | WiFi events, state changes | Medium |

## Detailed Guidelines

### 1. Direct Function Calls

**Use when**: Within the same component, no task synchronization needed

```cpp
// ✅ GOOD: Helper functions in same component
static void validate_config(config_t *cfg) { ... }
static void apply_defaults(config_t *cfg) { ... }

esp_err_t config_load(config_t *cfg) {
    validate_config(cfg);  // Direct call - simple and fast
    apply_defaults(cfg);
    return ESP_OK;
}
```

**Don't use**: Across task boundaries or when resource protection is needed

---

### 2. FreeRTOS Queues

**Use when**: Passing data/commands between tasks

```cpp
// ✅ GOOD: Network task sends commands to app task
typedef struct {
    uint8_t type;
    uint8_t data[64];
} network_cmd_t;

QueueHandle_t cmd_queue;

// Network task (core 0)
void network_task(void *arg) {
    network_cmd_t cmd;
    // Receive packet
    xQueueSend(cmd_queue, &cmd, pdMS_TO_TICKS(100));
}

// Application task (core 1)
void app_task(void *arg) {
    network_cmd_t cmd;
    if (xQueueReceive(cmd_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
        process_command(&cmd);
    }
}
```

**Queue sizing**: Balance memory vs packet loss
- Small (5-10): Low memory, may drop packets under load
- Medium (10-20): Good for most cases
- Large (>20): High memory, only if burst handling critical

**Timeout strategy**:
- Send: 100-1000ms in non-critical tasks, 0 in ISR/critical paths
- Receive: Task-dependent, often blocking with long timeout

---

### 3. Mutexes

**Use when**: Multiple tasks access shared resource (hardware, data structure)

```cpp
// ✅ GOOD: Protect display hardware
SemaphoreHandle_t display_mutex;

void render_eyes(void) {
    if (xSemaphoreTake(display_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        // Exclusive access to display
        ssd1306_clear();
        draw_eyes();
        ssd1306_refresh();
        xSemaphoreGive(display_mutex);
    } else {
        ESP_LOGW(TAG, "Display busy, skipping frame");
    }
}

// ✅ GOOD: Protect state machine
SemaphoreHandle_t state_mutex;

app_state_t get_state(void) {
    app_state_t state;
    xSemaphoreTake(state_mutex, portMAX_DELAY);  // OK: Quick read
    state = current_state;
    xSemaphoreGive(state_mutex);
    return state;
}
```

**Mutex priorities**: Use priority inheritance to avoid priority inversion
```cpp
display_mutex = xSemaphoreCreateMutex();  // Has priority inheritance
```

**Timeout strategy**:
- Short operations (<10ms): portMAX_DELAY acceptable
- Long operations: Use timeout and handle failure gracefully

---

### 4. Binary Semaphores

**Use when**: Signal completion or availability between tasks

```cpp
// ✅ GOOD: Signal that work is ready
SemaphoreHandle_t work_ready;

void producer_task(void *arg) {
    while (1) {
        prepare_work();
        xSemaphoreGive(work_ready);  // Signal: work is ready
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void worker_task(void *arg) {
    while (1) {
        xSemaphoreTake(work_ready, portMAX_DELAY);  // Wait for signal
        process_work();
    }
}
```

**When NOT to use**: If you need to pass data → Use queue instead

---

### 5. Task Notifications (Preferred for 1:1 Signaling)

**Use when**: Simple signaling between two specific tasks (lighter than semaphore)

```cpp
// ✅ GOOD: Lightweight signaling
TaskHandle_t worker_handle;

void producer_task(void *arg) {
    while (1) {
        prepare_work();
        xTaskNotifyGive(worker_handle);  // Lightweight signal
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void worker_task(void *arg) {
    worker_handle = xTaskGetCurrentTaskHandle();
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // Wait for notification
        process_work();
    }
}
```

**Advantages over semaphores**:
- Lower memory footprint
- Faster execution
- Built into task control block

**Limitations**:
- Only 1:1 communication (one sender, one receiver)
- No broadcast capability

---

### 6. Event Groups

**Use when**: Waiting on multiple independent conditions

```cpp
// ✅ GOOD: System initialization waits for multiple subsystems
EventGroupHandle_t init_events;

#define WIFI_READY    BIT0
#define DISPLAY_READY BIT1
#define CONFIG_READY  BIT2
#define ALL_READY     (WIFI_READY | DISPLAY_READY | CONFIG_READY)

void init_task(void *arg) {
    // Wait for all subsystems
    EventBits_t bits = xEventGroupWaitBits(
        init_events,
        ALL_READY,
        pdFALSE,  // Don't clear bits
        pdTRUE,   // Wait for all bits
        pdMS_TO_TICKS(10000)
    );
    
    if ((bits & ALL_READY) == ALL_READY) {
        ESP_LOGI(TAG, "System ready");
        transition_to_running();
    }
}

// Each subsystem sets its bit when ready
void wifi_init_done(void) {
    xEventGroupSetBits(init_events, WIFI_READY);
}
```

**When NOT to use**: Simple single-condition waiting → Use semaphore/notification instead

**Limitations**: Only 24 bits available (3 reserved by FreeRTOS)

---

### 7. ESP Event Loop (`esp_event`)

**Use when**: System-wide events with multiple observers, decoupled architecture

```cpp
// ✅ GOOD: WiFi events observed by multiple components
ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT);

// Define event IDs
typedef enum {
    WIFI_MGR_CONNECTED,
    WIFI_MGR_DISCONNECTED,
    WIFI_MGR_GOT_IP
} wifi_mgr_event_t;

// WiFi manager posts events
void wifi_manager_on_connected(void) {
    esp_event_post(WIFI_MGR_EVENT, WIFI_MGR_CONNECTED, NULL, 0, portMAX_DELAY);
}

// Multiple components can observe
void app_init(void) {
    // Application task handler
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_MGR_EVENT, WIFI_MGR_CONNECTED,
        &app_on_wifi_connected, NULL
    ));
    
    // Display task handler
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_MGR_EVENT, WIFI_MGR_CONNECTED,
        &display_show_connected, NULL
    ));
}
```

**Advantages**:
- Decoupled: Publishers don't know subscribers
- Multiple observers per event
- Standard pattern for ESP-IDF system events

**When NOT to use**:
- Tight task-to-task communication → Use queue
- Simple 1:1 signaling → Use task notification
- High-frequency events (>100 Hz) → Use queue for performance

**Handler guidelines**:
- Keep handlers short (<10ms)
- Don't block in handlers
- For heavy processing, queue work to a task

---

## BotiEyes Specific Usage

### Current Architecture

```
Network Task (Core 0)
  ↓ [Queue: cmd_queue]
Application Task (Core 1)
  ↓ [Mutex: display_mutex]
Display Hardware

WiFi Manager
  ↓ [ESP Event Loop: WIFI_EVENT]
Event Handlers → State Machine
  ↓ [ESP Event Loop: STATE_CHANGE_EVENT]
Application Task
```

### What We Use Where

| Component | Mechanism | Why |
|-----------|-----------|-----|
| Network → App | Queue | Pass command data between cores |
| Display access | Mutex | Protect SPI/I2C hardware |
| State machine | Mutex | Protect state transitions |
| WiFi events | ESP event loop | Multiple observers, system events |
| State changes | ESP event loop | Decoupled notification |
| Config access | Mutex | Protect NVS operations |

### What We DON'T Use (and why)

| Mechanism | Why Not Needed |
|-----------|----------------|
| Event groups | No complex multi-condition waits in current design |
| Counting semaphores | No resource pools (display, network are singletons) |
| Recursive mutexes | No nested locking patterns needed |

## Anti-Patterns to Avoid

### ❌ Polling Loop
```cpp
// ❌ BAD: Wastes CPU
while (!wifi_connected) {
    vTaskDelay(pdMS_TO_TICKS(100));
}
```

```cpp
// ✅ GOOD: Event-driven
xEventGroupWaitBits(wifi_events, WIFI_CONNECTED_BIT, ...);
```

---

### ❌ Global Flags Without Synchronization
```cpp
// ❌ BAD: Race condition
volatile bool display_busy = false;  // Not atomic!

void task1(void) {
    if (!display_busy) {
        display_busy = true;  // Race here!
        use_display();
        display_busy = false;
    }
}
```

```cpp
// ✅ GOOD: Mutex protection
xSemaphoreTake(display_mutex, pdMS_TO_TICKS(100));
use_display();
xSemaphoreGive(display_mutex);
```

---

### ❌ Blocking Forever in Critical Path
```cpp
// ❌ BAD: Can deadlock system
xQueueReceive(queue, &msg, portMAX_DELAY);  // In network callback!
```

```cpp
// ✅ GOOD: Use timeout
if (xQueueReceive(queue, &msg, pdMS_TO_TICKS(100)) == pdTRUE) {
    process(msg);
} else {
    ESP_LOGW(TAG, "Queue receive timeout");
}
```

---

### ❌ Over-Engineering Simple Cases
```cpp
// ❌ BAD: Unnecessary event loop for simple case
esp_event_post(MY_EVENT, WORK_DONE, NULL, 0, portMAX_DELAY);

void handler(void *arg, ...) {
    // Just wakes up task
    xTaskNotifyGive(worker);
}
```

```cpp
// ✅ GOOD: Direct notification
xTaskNotifyGive(worker_handle);  // Much simpler!
```

---

## Decision Tree

```
Need to communicate/synchronize?
│
├─ Within same component?
│  └─ YES → Direct function call
│
├─ Pass data between tasks?
│  └─ YES → Queue
│
├─ Protect shared resource?
│  └─ YES → Mutex
│
├─ Simple signal (work ready, done)?
│  ├─ One sender, one receiver?
│  │  └─ YES → Task notification
│  └─ Multiple senders/receivers?
│     └─ YES → Binary semaphore
│
├─ Wait on multiple conditions?
│  └─ YES → Event group
│
└─ System-wide event with multiple observers?
   └─ YES → ESP event loop
```

## Summary

**Start simple, add complexity only when needed:**

1. **Default to direct calls** within a component
2. **Use queues** for inter-task data transfer
3. **Use mutexes** for shared resource protection
4. **Use task notifications** for simple 1:1 signaling
5. **Use ESP event loop** for system events and cross-component notifications
6. **Avoid** event groups unless truly waiting on multiple conditions
7. **Avoid** polling - always use proper synchronization primitives

**Remember**: The best synchronization is the simplest one that correctly solves your problem.
