# botieyes Development Guidelines

Auto-generated from all feature plans. Last updated: 2026-04-17

## Active Technologies

- C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator + Adafruit GFX (embedded rendering only); Pygame (emulator only) (001-emotion-driven-eyes)
- C++ (Arduino/PlatformIO, ESP32 core) network control layer + Python 3.8+ reference controller client + ESP32 Wi-Fi (WiFi/WiFiUDP); stdlib socket (controller) (002-esp32-network-service)

## Project Structure

```text
src/
tests/
```

## Commands

cd src; pytest; ruff check .

## Code Style

C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator: Follow standard conventions

## Recent Changes

- 002-esp32-network-service: Added ESP32 UDP network control service (best-effort streaming + acks/heartbeats, single-controller lock, 5 s timeout) layered on the existing BotiEyes API
- 001-emotion-driven-eyes: Added C++ (Arduino 1.8+/PlatformIO) for embedded library; Python 3.8+ for PC emulator + Adafruit GFX (embedded rendering only); Pygame (emulator only)

<!-- MANUAL ADDITIONS START -->

## Commit policy for embedded C++ changes

Before committing any change under `BotiEyes/src/**` or `BotiEyes/examples/**`
that affects rendering, timing, I2C, display init, or the public API:

1. Build and upload the relevant example sketch to the TTGO LoRa32 board.
2. Visually verify the behavior on the OLED.
3. Only then run `git add` + `git commit`.

Python emulator changes (`emulator/**`) can be validated by running the
emulator alone.

### Hardware reference

- Board: TTGO LoRa32 (ESP32 + integrated SSD1306 128x64).
- OLED pins: SDA=GPIO4, SCL=GPIO15, RST=GPIO16.

<!-- MANUAL ADDITIONS END -->

## Architecture Enforcement (Feature 004)

**CRITICAL**: When modifying `esp-idf/**` code, follow industrial ESP-IDF patterns:

### Component Boundaries
- Components live in `esp-idf/components/{component}/`
- Public API: `include/{component}/*.h`, Private: `src/*.c|cpp`
- NO direct #include of other component's `src/` files
- Declare deps in CMakeLists.txt: `idf_component_register(REQUIRES ...)`

### Communication (Event-Driven)
```cpp
// ✅ CORRECT: Use right tool for the job
// System-wide events → esp_event
ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));

// Inter-task data → queues
xQueueSend(cmd_queue, &message, pdMS_TO_TICKS(100));

// Shared resource → mutex
xSemaphoreTake(display_mutex, pdMS_TO_TICKS(100));
draw_to_display();
xSemaphoreGive(display_mutex);

// Simple signaling → task notification (lighter than semaphore)
xTaskNotifyGive(worker_task_handle);

// Wait multiple conditions → event group (only if really needed)
xEventGroupWaitBits(system_events, WIFI_READY | DISPLAY_READY, ...);

// Same component → direct call (no overhead)
static void helper_function(void) { }

// ❌ WRONG: Over-complicated or misused
register_callback(&my_callback);  // Use esp_event instead
while (!ready) { vTaskDelay(10); }  // Use event group or semaphore
xQueueReceive(queue, &msg, portMAX_DELAY);  // Timeout required!
global_flag = true;  // Race condition - use proper sync
```

### Task Patterns
```cpp
// ✅ CORRECT: Proper task structure
xTaskCreatePinnedToCore(network_task, "net", 4096, NULL, PRIORITY_HIGH, NULL, 0);
void network_task(void *arg) {
    while (1) {
        esp_task_wdt_reset();  // Feed watchdog
        // Process work
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ❌ WRONG: No watchdog, blocking forever
void bad_task(void *arg) {
    xQueueReceive(queue, &msg, portMAX_DELAY);  // Use timeout!
}
```

### Hardware Abstraction
```cpp
// ✅ CORRECT: Go through HAL
hal_init_display(CONFIG_DISPLAY_TYPE_SPI);
hal_set_led(LED_COLOR_GREEN);

// ❌ WRONG: Direct hardware access in application
gpio_set_level(GPIO_NUM_2, 1);  // HAL only!
spi_device_transmit(...);  // Driver layer only!
```

### Error Handling
```cpp
// ✅ CORRECT: Check and log
esp_err_t ret = nvs_open("storage", NVS_READWRITE, &handle);
if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS: %s (state=%d)", esp_err_to_name(ret), app_state);
    return ret;
}

// ❌ WRONG: Unchecked
nvs_open("storage", NVS_READWRITE, &handle);  // What if it fails?
```

### Copilot Checklist
Before accepting AI-generated ESP-IDF code, verify:
- [ ] Uses esp_event for WiFi/system events?
- [ ] Uses queues for inter-task data transfer?
- [ ] Uses mutexes for shared resource protection (display, state machine)?
- [ ] Uses appropriate sync: semaphore/notification for signaling, event group only if waiting on multiple conditions?
- [ ] Hardware access only in HAL layer?
- [ ] All error returns checked?
- [ ] Watchdog fed in loops?
- [ ] Proper timeouts (not portMAX_DELAY in critical tasks)?
- [ ] Hierarchical log tags (LAYER:COMP:MOD)?
- [ ] No busy-waiting or polling (except hardware registers)?
- [ ] Direct calls used within component when no sync needed?

<!-- SPECKIT START -->
For additional context about technologies to be used, project structure,
shell commands, and other important information, read the current plan:
specs/004-industrial-firmware-architecture/plan.md
<!-- SPECKIT END -->
