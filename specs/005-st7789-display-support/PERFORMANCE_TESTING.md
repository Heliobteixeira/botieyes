# Performance Measurement Guide: ST7789 Display

**Feature**: 005-st7789-display-support  
**Purpose**: Measure actual performance metrics for hardware validation (T037-T040)

This guide shows how to instrument the ESP-IDF firmware to measure display performance.

---

## Prerequisites

```bash
cd /Users/helioteixeira/dev/botieyes/esp-idf
source ~/.espressif/tools/activate_idf_v6.0.1.sh
```

---

## 1. Display Initialization Timing (T037: <2s)

### Add to main.cpp (app_main function)

```cpp
#include "esp_timer.h"

extern "C" void app_main(void)
{
    // ... existing NVS/event loop initialization ...

    // === PERFORMANCE METRIC: Display Init Timing ===
    int64_t display_init_start = esp_timer_get_time();
    
    // Initialize display
    Adafruit_GFX* display = display_init();
    if (!display) {
        ESP_LOGE(TAG, "Display initialization failed!");
        return;
    }
    
    int64_t display_init_end = esp_timer_get_time();
    float display_init_ms = (display_init_end - display_init_start) / 1000.0f;
    
    ESP_LOGI(TAG, "✓ T037: Display init time: %.2f ms (target: <2000 ms, status: %s)",
             display_init_ms,
             display_init_ms < 2000.0f ? "PASS" : "FAIL");
    
    // Clear display and show boot logo
    display->fillScreen(0x0000);
    // ... rest of initialization ...
}
```

### Expected Output

```
I (1234) main: ✓ T037: Display init time: 145.23 ms (target: <2000 ms, status: PASS)
```

---

## 2. Frame Rate Measurement (T038: ≥10 FPS)

### Method A: Simple FPS Counter (add to main loop)

```cpp
// In app_main() or main animation loop

#include "esp_timer.h"

static int frame_count = 0;
static int64_t fps_window_start = 0;

void app_main(void) {
    // ... initialization ...
    
    fps_window_start = esp_timer_get_time();
    
    while (1) {
        // Render frame
        display->fillScreen(0x0000);
        eyes.draw();
        
        if (BotiEyes::DisplayFlushable* flushable = 
            dynamic_cast<BotiEyes::DisplayFlushable*>(display)) {
            flushable->flush();
        }
        
        frame_count++;
        
        // Calculate FPS every 100 frames
        if (frame_count % 100 == 0) {
            int64_t now = esp_timer_get_time();
            int64_t elapsed_us = now - fps_window_start;
            float fps = (100.0f * 1000000.0f) / elapsed_us;
            
            ESP_LOGI(TAG, "✓ T038: Frame rate: %.2f FPS (target: ≥10 FPS, status: %s)",
                     fps,
                     fps >= 10.0f ? "PASS" : "FAIL");
            
            fps_window_start = now;
        }
        
        vTaskDelay(pdMS_TO_TICKS(33)); // 30 FPS target
    }
}
```

### Method B: Detailed Frame Timing (add before while loop)

```cpp
// In app_main(), before main loop

typedef struct {
    int64_t frame_start;
    int64_t render_done;
    int64_t flush_done;
    float render_ms;
    float flush_ms;
    float total_ms;
} frame_timing_t;

#define TIMING_SAMPLES 30
frame_timing_t timings[TIMING_SAMPLES];
int timing_idx = 0;

// In main loop:
while (1) {
    frame_timing_t* t = &timings[timing_idx];
    t->frame_start = esp_timer_get_time();
    
    // Render
    display->fillScreen(0x0000);
    eyes.draw();
    t->render_done = esp_timer_get_time();
    
    // Flush
    if (BotiEyes::DisplayFlushable* flushable = 
        dynamic_cast<BotiEyes::DisplayFlushable*>(display)) {
        flushable->flush();
    }
    t->flush_done = esp_timer_get_time();
    
    // Calculate timings
    t->render_ms = (t->render_done - t->frame_start) / 1000.0f;
    t->flush_ms = (t->flush_done - t->render_done) / 1000.0f;
    t->total_ms = (t->flush_done - t->frame_start) / 1000.0f;
    
    timing_idx = (timing_idx + 1) % TIMING_SAMPLES;
    
    // Report every 30 frames
    if (timing_idx == 0) {
        float avg_render = 0, avg_flush = 0, avg_total = 0;
        for (int i = 0; i < TIMING_SAMPLES; i++) {
            avg_render += timings[i].render_ms;
            avg_flush += timings[i].flush_ms;
            avg_total += timings[i].total_ms;
        }
        avg_render /= TIMING_SAMPLES;
        avg_flush /= TIMING_SAMPLES;
        avg_total /= TIMING_SAMPLES;
        
        float fps = 1000.0f / avg_total;
        
        ESP_LOGI(TAG, "Frame timing: render=%.2f ms, flush=%.2f ms, total=%.2f ms (%.1f FPS)",
                 avg_render, avg_flush, avg_total, fps);
    }
    
    vTaskDelay(pdMS_TO_TICKS(33));
}
```

### Expected Output

```
I (5678) main: ✓ T038: Frame rate: 28.34 FPS (target: ≥10 FPS, status: PASS)
I (8901) main: Frame timing: render=12.3 ms, flush=18.7 ms, total=31.0 ms (32.3 FPS)
```

---

## 3. Emotion State Rendering Performance (T039)

### Add emotion transition tracking

```cpp
// In app_main() or application task

#include "EmotionState.h"

// Track emotion changes
static int emotion_change_count = 0;
static int64_t last_emotion_change = 0;

// When setting new emotion:
void handleEmotionCommand(float valence, float arousal) {
    int64_t change_start = esp_timer_get_time();
    
    eyes.setEmotion(valence, arousal, 400); // 400ms transition
    
    int64_t change_done = esp_timer_get_time();
    float change_ms = (change_done - change_start) / 1000.0f;
    
    emotion_change_count++;
    last_emotion_change = change_done;
    
    ESP_LOGI(TAG, "✓ T039: Emotion change #%d: %.2f ms (valence=%.2f, arousal=%.2f)",
             emotion_change_count, change_ms, valence, arousal);
}

// In main loop, check for visual artifacts:
// (Manual verification - watch display for jitter, tearing, or flicker)
```

### Test Sequence

```cpp
// Add to app_main() after initialization

ESP_LOGI(TAG, "=== T039: Emotion State Rendering Test ===");

// Test all emotion presets
const struct {
    const char* name;
    float valence;
    float arousal;
} emotions[] = {
    {"Happy", 0.8f, 0.6f},
    {"Sad", -0.8f, 0.3f},
    {"Angry", -0.6f, 0.9f},
    {"Calm", 0.4f, 0.1f},
    {"Excited", 0.7f, 0.9f},
    {"Neutral", 0.0f, 0.5f}
};

for (int i = 0; i < 6; i++) {
    ESP_LOGI(TAG, "Testing emotion: %s", emotions[i].name);
    handleEmotionCommand(emotions[i].valence, emotions[i].arousal);
    vTaskDelay(pdMS_TO_TICKS(2000)); // 2 seconds per emotion
}

ESP_LOGI(TAG, "=== T039: Test complete - inspect display for artifacts ===");
```

### Expected Output

```
I (1234) main: === T039: Emotion State Rendering Test ===
I (1235) main: Testing emotion: Happy
I (1236) main: ✓ T039: Emotion change #1: 2.45 ms (valence=0.80, arousal=0.60)
I (3236) main: Testing emotion: Sad
I (3237) main: ✓ T039: Emotion change #2: 2.38 ms (valence=-0.80, arousal=0.30)
...
I (13236) main: === T039: Test complete - inspect display for artifacts ===
```

**Manual Check**: Watch display - should see smooth transitions, no tearing or jitter.

---

## 4. WiFi Compatibility Testing (T040)

### Add WiFi initialization timing

```cpp
// In app_main(), around WiFi initialization

#include "wifi_manager.h"

extern "C" void app_main(void)
{
    // ... display initialization ...
    
    int64_t wifi_init_start = esp_timer_get_time();
    
    // Initialize WiFi manager
    esp_err_t ret = wifi_manager_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi manager init failed: %s", esp_err_to_name(ret));
    }
    
    // Start WiFi connection
    ret = wifi_manager_start();
    
    int64_t wifi_init_end = esp_timer_get_time();
    float wifi_init_ms = (wifi_init_end - wifi_init_start) / 1000.0f;
    
    ESP_LOGI(TAG, "✓ T040: WiFi init time: %.2f ms", wifi_init_ms);
    
    // Monitor display during WiFi activity
    ESP_LOGI(TAG, "=== T040: Display + WiFi Compatibility Test ===");
    ESP_LOGI(TAG, "Watch display for:");
    ESP_LOGI(TAG, "  - Display glitches during WiFi connection");
    ESP_LOGI(TAG, "  - Animation stuttering");
    ESP_LOGI(TAG, "  - System crashes or resets");
    
    // Continue with main loop...
}
```

### Extended WiFi Stress Test

```cpp
// Add after WiFi connection established

void testWiFiDisplayCompatibility() {
    ESP_LOGI(TAG, "=== T040: WiFi + Display Stress Test ===");
    
    // Test 1: Display rendering during WiFi scan
    ESP_LOGI(TAG, "Test 1: WiFi scan during animation");
    int64_t scan_start = esp_timer_get_time();
    
    wifi_ap_record_t ap_info[10];
    uint16_t ap_count = 10;
    esp_wifi_scan_start(NULL, true); // Blocking scan
    esp_wifi_scan_get_ap_records(&ap_count, ap_info);
    
    int64_t scan_end = esp_timer_get_time();
    ESP_LOGI(TAG, "WiFi scan: %.2f ms, found %d APs (display should stay stable)",
             (scan_end - scan_start) / 1000.0f, ap_count);
    
    // Test 2: UDP traffic during rendering
    ESP_LOGI(TAG, "Test 2: UDP traffic during animation");
    // (Network controller would send commands here)
    
    // Test 3: Sustained WiFi activity
    ESP_LOGI(TAG, "Test 3: 60-second WiFi + display stress test");
    int64_t stress_start = esp_timer_get_time();
    int stress_frames = 0;
    
    while ((esp_timer_get_time() - stress_start) < 60000000) { // 60 seconds
        // Render frame
        display->fillScreen(0x0000);
        eyes.draw();
        if (BotiEyes::DisplayFlushable* f = 
            dynamic_cast<BotiEyes::DisplayFlushable*>(display)) {
            f->flush();
        }
        stress_frames++;
        vTaskDelay(pdMS_TO_TICKS(33));
    }
    
    float stress_fps = (stress_frames * 1000000.0f) / (esp_timer_get_time() - stress_start);
    ESP_LOGI(TAG, "✓ T040: Stress test complete - %d frames, %.1f FPS average",
             stress_frames, stress_fps);
}

// Call in app_main() after WiFi connected:
testWiFiDisplayCompatibility();
```

### Expected Output

```
I (2000) main: ✓ T040: WiFi init time: 1234.56 ms
I (2001) main: === T040: Display + WiFi Compatibility Test ===
I (3000) main: === T040: WiFi + Display Stress Test ===
I (3001) main: Test 1: WiFi scan during animation
I (3500) main: WiFi scan: 498.23 ms, found 5 APs (display should stay stable)
I (3501) main: Test 2: UDP traffic during animation
I (3502) main: Test 3: 60-second WiFi + display stress test
I (63502) main: ✓ T040: Stress test complete - 1800 frames, 30.0 FPS average
```

**Manual Checks**:
- No display corruption during WiFi scan
- No animation stuttering
- No ESP32 watchdog resets
- No visual artifacts when network packets arrive

---

## 5. Memory Usage Monitoring

### Add heap monitoring

```cpp
#include "esp_heap_caps.h"

void reportMemoryUsage(const char* label) {
    size_t free_heap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    size_t total_heap = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    size_t used_heap = total_heap - free_heap;
    
    ESP_LOGI(TAG, "[%s] Heap: %zu / %zu bytes used (%.1f%%), min_free: %zu bytes",
             label,
             used_heap,
             total_heap,
             (used_heap * 100.0f) / total_heap,
             min_free);
    
    #ifdef CONFIG_ST7789_FRAME_BUFFER
        uint32_t fb_size = CONFIG_ST7789_WIDTH * CONFIG_ST7789_HEIGHT * 2;
        ESP_LOGI(TAG, "[%s] Frame buffer: %lu bytes (%.1f%% of total heap)",
                 label,
                 fb_size,
                 (fb_size * 100.0f) / total_heap);
    #endif
}

// Call at key points:
reportMemoryUsage("Startup");
// ... after display init
reportMemoryUsage("After Display Init");
// ... after WiFi init
reportMemoryUsage("After WiFi Init");
// ... during main loop
reportMemoryUsage("During Animation");
```

---

## 6. Automated Performance Test Script

### Create: `esp-idf/scripts/performance_test.py`

```python
#!/usr/bin/env python3
"""
Performance test runner for ST7789 display
Captures serial output and validates performance metrics
"""

import serial
import time
import re
import sys

METRICS = {
    'T037': {'name': 'Display Init', 'threshold': 2000.0, 'unit': 'ms'},
    'T038': {'name': 'Frame Rate', 'threshold': 10.0, 'unit': 'FPS'},
    'T039': {'name': 'Emotion Change', 'threshold': 100.0, 'unit': 'ms'},
    'T040': {'name': 'WiFi Stress Test', 'threshold': 10.0, 'unit': 'FPS'}
}

def parse_metrics(serial_port, duration=120):
    """Parse performance metrics from serial output"""
    ser = serial.Serial(serial_port, 115200, timeout=1)
    results = {}
    start_time = time.time()
    
    print(f"Monitoring {serial_port} for {duration} seconds...")
    
    while (time.time() - start_time) < duration:
        try:
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if not line:
                continue
                
            print(line)  # Echo to console
            
            # Parse T037: Display init
            match = re.search(r'T037.*?(\d+\.\d+)\s*ms', line)
            if match:
                results['T037'] = float(match.group(1))
            
            # Parse T038: Frame rate
            match = re.search(r'T038.*?(\d+\.\d+)\s*FPS', line)
            if match:
                if 'T038' not in results:
                    results['T038'] = []
                results['T038'].append(float(match.group(1)))
            
            # Parse T039: Emotion change
            match = re.search(r'T039.*?(\d+\.\d+)\s*ms', line)
            if match:
                if 'T039' not in results:
                    results['T039'] = []
                results['T039'].append(float(match.group(1)))
            
            # Parse T040: Stress test
            match = re.search(r'T040.*?(\d+\.\d+)\s*FPS', line)
            if match:
                results['T040'] = float(match.group(1))
                
        except Exception as e:
            print(f"Error: {e}")
    
    ser.close()
    return results

def validate_results(results):
    """Validate metrics against thresholds"""
    print("\n" + "="*60)
    print("PERFORMANCE TEST RESULTS")
    print("="*60)
    
    all_pass = True
    
    for task_id, metric in METRICS.items():
        if task_id not in results:
            print(f"❌ {task_id} ({metric['name']}): NO DATA")
            all_pass = False
            continue
        
        value = results[task_id]
        if isinstance(value, list):
            value = sum(value) / len(value)  # Average
        
        threshold = metric['threshold']
        unit = metric['unit']
        
        if task_id == 'T038' or task_id == 'T040':
            # Higher is better for FPS
            passed = value >= threshold
        else:
            # Lower is better for timing
            passed = value <= threshold
        
        status = "✅ PASS" if passed else "❌ FAIL"
        print(f"{status} {task_id} ({metric['name']}): {value:.2f} {unit} "
              f"(threshold: {'≥' if 'FPS' in unit else '≤'}{threshold} {unit})")
        
        if not passed:
            all_pass = False
    
    print("="*60)
    return all_pass

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python performance_test.py <serial_port>")
        print("Example: python performance_test.py /dev/ttyUSB0")
        sys.exit(1)
    
    port = sys.argv[1]
    results = parse_metrics(port, duration=120)
    
    all_pass = validate_results(results)
    sys.exit(0 if all_pass else 1)
```

### Run automated test:

```bash
cd esp-idf
python scripts/performance_test.py /dev/ttyUSB0
```

---

## 7. Complete Test Checklist

### Before Testing

- [ ] Flash firmware with performance instrumentation
- [ ] Connect serial monitor (115200 baud)
- [ ] Ensure TTGO T-Display powered and display working
- [ ] WiFi credentials configured in menuconfig
- [ ] Frame buffer enabled/disabled as desired

### During Testing

- [ ] **T037**: Note display init time in serial output
- [ ] **T038**: Monitor FPS counter for 30+ seconds
- [ ] **T039**: Watch display during emotion transitions (visual check)
- [ ] **T040**: Run WiFi stress test, watch for glitches

### Pass Criteria

| Test | Metric | Threshold | How to Verify |
|------|--------|-----------|---------------|
| T037 | Display init time | <2000 ms | Serial log: "T037: Display init time: X ms" |
| T038 | Animation frame rate | ≥10 FPS | Serial log: "T038: Frame rate: X FPS" |
| T039 | Emotion rendering | Smooth, no artifacts | Visual inspection of display |
| T040 | WiFi + display | No crashes, stable FPS | Visual + serial log check |

---

## 8. Troubleshooting

### Low Frame Rate (<10 FPS)

**Possible causes**:
- Frame buffer disabled (enable in menuconfig)
- SPI clock too low (increase to 40-80 MHz)
- Complex rendering (optimize BotiEyes drawing)

**Debug**:
```cpp
ESP_LOGI(TAG, "Frame breakdown: render=%d ms, flush=%d ms", render_time, flush_time);
// If flush_time > 25ms, increase SPI clock
// If render_time > 15ms, optimize drawing code
```

### Display Artifacts During WiFi

**Possible causes**:
- Shared SPI bus (WiFi radio interference)
- Insufficient task priorities
- Memory allocation during animation

**Fix**:
- Pin display task to core 0, WiFi to core 1
- Increase display task priority
- Pre-allocate all memory before loop

---

## Summary

This instrumentation provides comprehensive metrics for:
- ✅ T037: Display initialization timing
- ✅ T038: Real-time frame rate measurement  
- ✅ T039: Emotion state rendering validation
- ✅ T040: WiFi compatibility stress testing

Add the relevant code snippets to `main.cpp`, build, flash, and monitor serial output to collect actual performance data.
