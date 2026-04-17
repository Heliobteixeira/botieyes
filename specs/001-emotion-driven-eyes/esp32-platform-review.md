# ESP32 Platform Expert Review: BotiEyes Library

**Reviewer Role**: ESP32 Platform Expert (FreeRTOS, ESP-IDF, dual-core optimization)  
**Review Date**: 2026-04-17  
**Library Version**: v1.0.0 (emotion-driven eyes)  
**Verdict**: ✅ **APPROVED with ESP32-Specific Recommendations**

---

## Executive Summary

The BotiEyes library design is **fundamentally sound** and will work well on ESP32. However, the current Nano-first architecture leaves significant ESP32 capabilities underutilized. This review provides concrete recommendations to unlock ESP32's full potential while maintaining backward compatibility.

**Key Findings**:
- ✅ Core design (valence-arousal model, coupled eyes, smooth interpolation) is excellent
- ✅ Memory footprint (1.04KB) is negligible on ESP32 (520KB SRAM available)
- ⚠️ Current design treats ESP32 as "faster Arduino" - missing dual-core, DMA, WiFi opportunities
- ⚠️ I2C bottleneck (15ms @ 400kHz) limits ESP32 to ~30 FPS when 60+ FPS is achievable
- ⚠️ No WiFi/BLE integration strategy for AI control (key ESP32 advantage)
- ⚠️ No FreeRTOS task utilization (rendering could run on dedicated core)

**Recommendation**: Implement **ESP32 optional enhancements** via compile-time flags without breaking Arduino compatibility.

---

## 1. Approval Status: ✅ APPROVED

### What Works Well for ESP32

1. **Memory Efficiency**: 1.04KB library footprint is excellent - leaves 519KB for advanced features
2. **API Design**: Clean, non-blocking API (`update()` at target FPS) fits FreeRTOS task model perfectly
3. **Static Allocation**: No heap fragmentation issues - critical for long-running ESP32 robots
4. **Emotion Helpers**: Simple API reduces cognitive load - ideal for WiFi/BLE control protocols
5. **Parametric Model**: Continuous valence-arousal values perfect for AI streaming over WiFi/BLE

### Design Concerns for ESP32

1. **Single-Threaded Rendering**: No dual-core utilization (Core 0 idle during rendering)
2. **Synchronous I2C**: Missing DMA opportunity - blocking CPU during display transfers
3. **No WiFi Integration**: No guidance for WiFi coexistence or BLE emotion control
4. **Frame Rate Cap**: 30-60 FPS target conservative for 240MHz dual-core CPU
5. **No Caching**: With 520KB RAM, could cache rendered frames for instant emotion switches
6. **No OTA Strategy**: Field updates for expression tuning not considered

---

## 2. ESP32 Best Practices Checklist

### ✅ Compliance (Good)
- [x] Static memory allocation (no `new`/`malloc` after init)
- [x] Non-blocking API design (no `delay()` in library)
- [x] Clean initialization/shutdown (supports deep sleep if user manages)
- [x] Simple error codes (no exceptions - good for FreeRTOS)
- [x] Deterministic update cycle (user controls timing)

### ⚠️ Opportunities (Could Improve)
- [ ] **Dual-core task pinning** (rendering on Core 1, WiFi on Core 0)
- [ ] **I2C/SPI DMA transfers** (async framebuffer push)
- [ ] **Hardware timer integration** (for precise FPS without manual `update()` calls)
- [ ] **FreeRTOS task priorities** (animation > WiFi responsiveness)
- [ ] **Power management hooks** (light sleep during idle, deep sleep support)
- [ ] **WiFi/BLE coexistence** (task pinning to avoid interference)
- [ ] **OTA partition scheme** (for expression parameter updates)
- [ ] **Watchdog timer management** (long renders need task feeding)
- [ ] **CPU frequency scaling** (80MHz may suffice, save power)
- [ ] **PSRAM support** (if enabled, use for frame history/caching)

### ❌ Missing ESP32-Specific Features
- [ ] **JSON export on Arduino** (ESP32 has 520KB - enable `getExpressionState()`)
- [ ] **BLE GATT service** (for low-latency AI emotion control)
- [ ] **WiFi AP mode** (for robot configuration/tuning)
- [ ] **SPIFFS/LittleFS** (save emotion presets to flash)
- [ ] **Hardware acceleration** (ESP32-S3 has 2D graphics acceleration)

---

## 3. Performance Optimization Recommendations

### 🚀 High-Impact Optimizations (50%+ FPS improvement)

#### 3.1. I2C DMA Transfers (10-20 FPS gain)
**Problem**: Current I2C blocking transfer takes ~15ms @ 400kHz, limiting to 30 FPS  
**Solution**: Use ESP32 I2C driver DMA mode for async framebuffer push

```cpp
#ifdef ESP32
    // In BotiEyes::update() after rendering to GFXcanvas1
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, framebuffer, BUFFER_SIZE, true);
    i2c_master_stop(cmd);
    
    // Non-blocking send with DMA
    i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
    i2c_cmd_link_delete(cmd);
    // CPU free to prepare next frame while transfer happens
#endif
```

**Expected Impact**: Reduces blocking time from 15ms → 2ms, enables 60+ FPS

---

#### 3.2. Dual-Core Rendering (15-25 FPS gain)
**Problem**: Core 0 handles WiFi, Core 1 idle during rendering  
**Solution**: Pin rendering task to Core 1, WiFi/BLE to Core 0

```cpp
#ifdef ESP32
    // In initialize()
    xTaskCreatePinnedToCore(
        renderTask,         // Task function
        "BotiEyes_Render",  // Name
        4096,               // Stack size (4KB)
        this,               // Parameter (BotiEyes instance)
        2,                  // Priority (higher than WiFi)
        &renderTaskHandle,  // Handle
        1                   // Core 1 (app core)
    );
    
    // Rendering runs continuously on Core 1
    void renderTask(void* param) {
        BotiEyes* eyes = (BotiEyes*)param;
        TickType_t lastWakeTime = xTaskGetTickCount();
        const TickType_t frequency = pdMS_TO_TICKS(16); // 60 FPS
        
        while (true) {
            eyes->update(); // Non-blocking render
            vTaskDelayUntil(&lastWakeTime, frequency);
        }
    }
#endif
```

**Expected Impact**:
- User code no longer needs to call `update()` manually (FreeRTOS handles timing)
- WiFi activity doesn't interrupt rendering (separate cores)
- Consistent 60 FPS even during WiFi transfers

---

#### 3.3. Hardware SPI over I2C (3-4x speedup)
**Problem**: I2C limited to ~400kHz, SPI can reach 40MHz  
**Solution**: Recommend SPI-compatible displays for ESP32 builds

```cpp
// platformio.ini recommendation
[env:esp32_spi]
build_flags = 
    -D USE_SPI_DISPLAY
    -D SPI_FREQUENCY=40000000  ; 40MHz SPI clock
```

**Expected Impact**: 15ms transfer → 3-5ms, easily hit 100+ FPS

---

### ⚡ Medium-Impact Optimizations (10-20% improvement)

#### 3.4. Expression Parameter Caching (520KB RAM advantage)
**Problem**: Re-calculating emotion mapping on every frame wastes CPU  
**Solution**: Cache computed `ExpressionParameters` for 100-1000 recent (valence, arousal) pairs

```cpp
#ifdef ESP32
    // With 520KB RAM, allocate 10KB cache (10,000 entries × 24 bytes)
    struct CachedExpression {
        uint16_t valence_key;   // Quantized valence × 1000
        uint16_t arousal_key;   // Quantized arousal × 1000
        ExpressionParameters params;
    };
    
    CachedExpression emotionCache[400]; // ~10KB
    
    // O(1) lookup vs O(n) computation
    ExpressionParameters* lookup(float v, float a) {
        uint16_t vKey = (uint16_t)((v + 0.5) * 1000);
        uint16_t aKey = (uint16_t)(a * 1000);
        uint16_t hash = (vKey ^ aKey) % 400;
        if (emotionCache[hash].valence_key == vKey && 
            emotionCache[hash].arousal_key == aKey) {
            return &emotionCache[hash].params; // Cache hit
        }
        return nullptr; // Cache miss, compute + store
    }
#endif
```

**Expected Impact**: Saves 2-5ms per frame (emotion mapping computation)

---

#### 3.5. Pre-rendered Frame Atlas (10KB cache)
**Problem**: Re-drawing 5 primitives per eye per frame is redundant  
**Solution**: Pre-render 10 emotion keyframes (happy, sad, angry, etc.) to RAM

```cpp
#ifdef ESP32
    // With 520KB RAM, cache 10 emotions × 1KB framebuffer = 10KB
    uint8_t frameAtlas[10][1024]; // Pre-rendered emotions
    
    // On emotion helper call (e.g., happy()), instant blit
    void happy(float intensity) {
        int keyframe = EMOTION_HAPPY;
        memcpy(display.getBuffer(), frameAtlas[keyframe], 1024);
        // Apply intensity scaling to pupil/eyelid if needed
        display.display(); // 15ms transfer only, skip rendering (5-10ms saved)
    }
#endif
```

**Expected Impact**: Instant emotion switches (5-10ms rendering saved), ~80+ FPS

---

### 🔧 Low-Impact (Nice-to-Have)

#### 3.6. CPU Frequency Scaling
```cpp
// In initialize(), test if 80MHz suffices (save power)
setCpuFrequencyMhz(80); // Default 240MHz may be overkill
```

#### 3.7. Watchdog Timer Feeding
```cpp
// In update() loop
esp_task_wdt_reset(); // Prevent watchdog reset during long renders
```

---

## 4. ESP32 Unique Advantages to Leverage

### 🌐 4.1. WiFi/BLE for AI Emotion Control

**Opportunity**: ESP32's killer feature - wireless AI integration  
**Use Case**: Cloud AI → WiFi → ESP32 → emotion change in <50ms latency

#### BLE GATT Service for Low-Latency Control
```cpp
#ifdef ESP32
    // BLE service UUID for BotiEyes emotion control
    #define EMOTION_SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
    #define VALENCE_CHAR_UUID    "12345678-1234-1234-1234-123456789abd"
    #define AROUSAL_CHAR_UUID    "12345678-1234-1234-1234-123456789abe"
    
    // BLE callback for AI writes
    class EmotionCallbacks : public BLECharacteristicCallbacks {
        void onWrite(BLECharacteristic* pCharacteristic) {
            std::string value = pCharacteristic->getValue();
            if (pCharacteristic->getUUID().toString() == VALENCE_CHAR_UUID) {
                float v = *(float*)value.data();
                eyes.setEmotion(v, currentArousal);
            }
        }
    };
    
    // Initialize BLE
    BLEDevice::init("BotiEyes Robot");
    BLEServer* pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService(EMOTION_SERVICE_UUID);
    
    BLECharacteristic* pValence = pService->createCharacteristic(
        VALENCE_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pValence->setCallbacks(new EmotionCallbacks());
#endif
```

**Expected Impact**:
- AI sends 8-byte BLE packet → emotion updates in 20-50ms (vs 200ms+ WiFi HTTP)
- No network infrastructure needed (direct BLE pairing)
- Works during WiFi disconnection

---

### 🌐 4.2. WiFi AP Mode for Expression Tuning

**Use Case**: Robot hosts WiFi AP → developer connects via phone → real-time tuning UI

```cpp
#ifdef ESP32
    // In initialize() - start AP mode
    WiFi.softAP("BotiEyes Config", "password123");
    IPAddress IP = WiFi.softAPIP();
    
    // Simple web server for tuning
    server.on("/emotion", HTTP_POST, []() {
        float v = server.arg("valence").toFloat();
        float a = server.arg("arousal").toFloat();
        eyes.setEmotion(v, a);
        server.send(200, "text/plain", "OK");
    });
    
    server.begin();
#endif
```

**Expected Impact**: Field tuning without recompilation/upload

---

### 💾 4.3. OTA Updates for Expression Parameters

**Use Case**: Update emotion mapping formulas remotely without physical access

```cpp
#ifdef ESP32
    #include <ArduinoOTA.h>
    
    // In setup()
    ArduinoOTA.onStart([]() {
        // Pause rendering during OTA
        vTaskSuspend(renderTaskHandle);
    });
    
    ArduinoOTA.begin();
    
    // Check for updates periodically
    ArduinoOTA.handle();
#endif
```

**Partition Scheme** (platformio.ini):
```ini
board_build.partitions = default_16MB.csv  ; 16MB flash
; Layout: OTA_0 (2MB) | OTA_1 (2MB) | SPIFFS (4MB) | ...
```

---

### 🧠 4.4. Expression Presets in Flash (SPIFFS/LittleFS)

**Use Case**: Save 10-20 custom emotion presets to flash, recall instantly

```cpp
#ifdef ESP32
    #include <SPIFFS.h>
    
    void savePreset(const char* name, float v, float a) {
        File file = SPIFFS.open(name, FILE_WRITE);
        file.write((uint8_t*)&v, sizeof(float));
        file.write((uint8_t*)&a, sizeof(float));
        file.close();
    }
    
    void loadPreset(const char* name) {
        File file = SPIFFS.open(name, FILE_READ);
        float v, a;
        file.read((uint8_t*)&v, sizeof(float));
        file.read((uint8_t*)&a, sizeof(float));
        eyes.setEmotion(v, a);
        file.close();
    }
#endif
```

---

### 🎨 4.5. Enable JSON Export on ESP32 (Unlike Nano)

**Problem**: JSON export removed from Arduino due to 2KB RAM constraint  
**Solution**: Re-enable on ESP32 (520KB available)

```cpp
#ifdef ESP32
    // Add back to BotiEyes class
    String getExpressionState() {
        StaticJsonDocument<512> doc;
        doc["valence"] = emotionState.currentValence;
        doc["arousal"] = emotionState.currentArousal;
        doc["pupilDilation"] = expressionParams.pupilDilation;
        doc["eyelidOpenness"] = expressionParams.eyelidOpenness;
        // ... etc
        
        String output;
        serializeJson(doc, output);
        return output;
    }
#endif
```

**Use Case**: WiFi debugging, send JSON to cloud for analysis

---

## 5. ESP32 Standard Development Practices

### 📦 5.1. PlatformIO Configuration (platformio.ini)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

; Compiler optimizations
build_flags = 
    -O2                              ; Optimize for speed
    -D CORE_DEBUG_LEVEL=0            ; No debug logs (performance)
    -D CONFIG_ARDUHAL_LOG_COLORS=0   ; Disable colored logs
    -D USE_ESP32_OPTIMIZATIONS       ; Enable custom ESP32 code paths
    
; I2C/SPI configurations
    -D I2C_FREQUENCY=400000          ; 400kHz I2C fast mode
    -D SPI_FREQUENCY=40000000        ; 40MHz SPI (if using SPI display)
    
; FreeRTOS task settings
    -D RENDER_TASK_PRIORITY=2        ; Higher than default (1)
    -D RENDER_TASK_STACK_SIZE=4096   ; 4KB stack
    -D RENDER_TASK_CORE=1            ; Pin to Core 1 (app core)

; Memory optimizations
    -D CONFIG_SPIRAM_SUPPORT=1       ; Enable PSRAM if available

; Partition scheme for OTA
board_build.partitions = default_16MB.csv

; Upload speed
upload_speed = 921600
monitor_speed = 115200

; Dependencies
lib_deps = 
    adafruit/Adafruit GFX Library @ ^1.11.0
    adafruit/Adafruit SSD1306 @ ^2.5.0
    bblanchon/ArduinoJson @ ^6.21.0  ; For ESP32 JSON export
```

---

### 🗂️ 5.2. Partition Schemes for OTA

**Recommended**: `default_16MB.csv` for 16MB flash ESP32

```csv
# Name,   Type, SubType, Offset,  Size
nvs,      data, nvs,     0x9000,  0x5000   ; Non-volatile storage
otadata,  data, ota,     0xe000,  0x2000   ; OTA data
app0,     app,  ota_0,   0x10000, 0x200000 ; OTA partition 0 (2MB)
app1,     app,  ota_1,   0x210000,0x200000 ; OTA partition 1 (2MB)
spiffs,   data, spiffs,  0x410000,0x3F0000 ; SPIFFS (4MB)
```

**For 4MB Flash** (common): Use `default.csv` (no OTA, 1.3MB app max)

---

### 🔄 5.3. OTA Update Strategy

**Approach 1: ArduinoOTA** (WiFi-based, simple)
```cpp
#include <ArduinoOTA.h>

void setupOTA() {
    ArduinoOTA.setHostname("botieyes-robot");
    ArduinoOTA.setPassword("admin123");
    
    ArduinoOTA.onStart([]() {
        vTaskSuspend(renderTaskHandle); // Pause rendering
    });
    
    ArduinoOTA.onEnd([]() {
        ESP.restart(); // Reboot to new firmware
    });
    
    ArduinoOTA.begin();
}

void loop() {
    ArduinoOTA.handle(); // Non-blocking check
}
```

**Approach 2: HTTP OTA** (cloud-based, automatic)
```cpp
#include <HTTPUpdate.h>

void checkForUpdates() {
    WiFiClient client;
    t_httpUpdate_return ret = httpUpdate.update(client, 
        "http://myserver.com/botieyes-firmware.bin");
    
    if (ret == HTTP_UPDATE_OK) {
        ESP.restart();
    }
}
```

---

### ⚠️ 5.4. ESP32 Gotchas & Workarounds

#### Gotcha 1: WiFi Interference with I2C Timing
**Problem**: WiFi radio can cause I2C timing glitches  
**Solution**: Pin rendering to Core 1, WiFi stays on Core 0 (separate)

```cpp
// Rendering on Core 1 (no WiFi)
xTaskCreatePinnedToCore(renderTask, "Render", 4096, NULL, 2, NULL, 1);

// WiFi operations on Core 0 (default)
WiFi.begin(ssid, password); // Runs on Core 0
```

---

#### Gotcha 2: Brownout Detector Resets
**Problem**: High current draw from OLED + WiFi causes brownout reset  
**Solution**: Adjust brownout threshold or use external regulator

```cpp
// In platformio.ini
build_flags = 
    -D CONFIG_ESP32_BROWNOUT_DET_LVL=0  ; Disable brownout (if power stable)
```

**Better**: Use dedicated 5V/3.3V regulator (not USB power)

---

#### Gotcha 3: Watchdog Timer Resets
**Problem**: Long rendering frames (>1 second) trigger watchdog reset  
**Solution**: Feed watchdog in `update()` loop

```cpp
#include "esp_task_wdt.h"

void BotiEyes::update() {
    esp_task_wdt_reset(); // Pet the watchdog
    // ... rendering code ...
}
```

---

#### Gotcha 4: I2C Pull-up Resistors
**Problem**: ESP32 internal pull-ups (weak) cause I2C errors at 400kHz  
**Solution**: Add external 2.2kΩ pull-ups on SDA/SCL

```
ESP32 GPIO21 (SDA) ----[2.2kΩ]---- 3.3V
ESP32 GPIO22 (SCL) ----[2.2kΩ]---- 3.3V
```

---

#### Gotcha 5: Float Performance (No FPU on ESP32)
**Problem**: ESP32 (non-S3) has no hardware FPU, float math slow  
**Solution**: Use fixed-point math for easing functions

```cpp
// Instead of: float eased = ease_in_out_cubic(progress);
int32_t eased_fixed = ease_in_out_cubic_fixed(progress_fixed); // 16.16 fixed
```

**Note**: ESP32-S3 has FPU, floats are fast

---

## 6. Code Examples

### Example 1: FreeRTOS Dual-Core Rendering Task

```cpp
#ifdef ESP32

TaskHandle_t renderTaskHandle = NULL;

void BotiEyes::initialize(const DisplayConfig& config) {
    // ... display init ...
    
    // Create dedicated rendering task on Core 1
    xTaskCreatePinnedToCore(
        renderTaskLoop,      // Function
        "BotiEyes_Render",   // Name
        4096,                // Stack (4KB)
        this,                // Parameter (BotiEyes instance)
        2,                   // Priority (higher than WiFi=1)
        &renderTaskHandle,   // Handle for suspend/resume
        1                    // Core 1 (app core, no WiFi)
    );
}

static void renderTaskLoop(void* param) {
    BotiEyes* eyes = (BotiEyes*)param;
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t framePeriod = pdMS_TO_TICKS(16); // 60 FPS
    
    while (true) {
        // Render frame (5-10ms)
        eyes->update();
        
        // Precise timing - compensates for render time jitter
        vTaskDelayUntil(&lastWakeTime, framePeriod);
    }
}

#endif // ESP32
```

**Benefits**:
- User doesn't call `update()` manually (FreeRTOS handles timing)
- Consistent 60 FPS regardless of WiFi activity
- Core 1 dedicated to rendering, Core 0 handles WiFi/BLE

---

### Example 2: BLE Emotion Control Service

```cpp
#ifdef ESP32
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

class EmotionCharCallbacks : public BLECharacteristicCallbacks {
    BotiEyes* eyes;
    
public:
    EmotionCharCallbacks(BotiEyes* e) : eyes(e) {}
    
    void onWrite(BLECharacteristic* pChar) {
        std::string uuid = pChar->getUUID().toString();
        std::string value = pChar->getValue();
        
        if (uuid == VALENCE_CHAR_UUID) {
            float valence = *(float*)value.data();
            float currentA;
            eyes->getCurrentEmotion(&currentV, &currentA);
            eyes->setEmotion(valence, currentA, 300);
        }
        else if (uuid == AROUSAL_CHAR_UUID) {
            float arousal = *(float*)value.data();
            float currentV;
            eyes->getCurrentEmotion(&currentV, &currentA);
            eyes->setEmotion(currentV, arousal, 300);
        }
    }
};

void setupBLEControl(BotiEyes& eyes) {
    BLEDevice::init("BotiEyes Robot");
    BLEServer* pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService(EMOTION_SERVICE_UUID);
    
    // Valence characteristic (write-only)
    BLECharacteristic* pValence = pService->createCharacteristic(
        VALENCE_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pValence->setCallbacks(new EmotionCharCallbacks(&eyes));
    
    // Arousal characteristic (write-only)
    BLECharacteristic* pArousal = pService->createCharacteristic(
        AROUSAL_CHAR_UUID,
        BLECharacteristic::PROPERTY_WRITE
    );
    pArousal->setCallbacks(new EmotionCharCallbacks(&eyes));
    
    pService->start();
    
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(EMOTION_SERVICE_UUID);
    pAdvertising->start();
    
    Serial.println("BLE Emotion Service started!");
}

#endif // ESP32
```

**Usage (Python AI client)**:
```python
import asyncio
from bleak import BleakClient

VALENCE_UUID = "12345678-1234-1234-1234-123456789abd"
AROUSAL_UUID = "12345678-1234-1234-1234-123456789abe"

async def set_emotion(valence, arousal):
    async with BleakClient("BotiEyes Robot") as client:
        await client.write_gatt_char(VALENCE_UUID, struct.pack('f', valence))
        await client.write_gatt_char(AROUSAL_UUID, struct.pack('f', arousal))

# AI sends emotion updates
asyncio.run(set_emotion(0.3, 0.6))  # Happy
```

**Latency**: ~20-50ms (BLE) vs 200-500ms (WiFi HTTP)

---

### Example 3: WiFi JSON Export for Debugging

```cpp
#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

WebServer server(80);

void setupWebDebug(BotiEyes& eyes) {
    WiFi.begin("MyWiFi", "password");
    
    server.on("/state", HTTP_GET, [&eyes]() {
        // Re-enable JSON export on ESP32 (removed from Arduino)
        StaticJsonDocument<512> doc;
        
        float v, a;
        eyes.getCurrentEmotion(&v, &a);
        doc["emotion"]["valence"] = v;
        doc["emotion"]["arousal"] = a;
        
        int16_t h, vert;
        eyes.getEyePosition(&h, &vert);
        doc["position"]["horizontal"] = h;
        doc["position"]["vertical"] = vert;
        
        // Add expression parameters (requires internal access)
        doc["expression"]["pupilDilation"] = 0.65; // TODO: expose getter
        doc["expression"]["eyelidOpenness"] = 0.80;
        doc["expression"]["browAngle"] = 0.4;
        
        String output;
        serializeJson(doc, output);
        server.send(200, "application/json", output);
    });
    
    server.begin();
}

void loop() {
    server.handleClient();
}

#endif // ESP32
```

**Usage**: `curl http://192.168.1.100/state` → JSON state dump

---

## 7. Architecture Recommendations

### Recommendation 1: Optional ESP32 Enhancement Layer

**Problem**: Don't want to break Arduino compatibility  
**Solution**: Compile-time feature flags

```cpp
// BotiEyes.h
#ifdef ESP32_OPTIMIZATIONS
    void enableDualCore(); // Start rendering task on Core 1
    void enableBLE(const char* deviceName);
    void enableWiFiDebug(const char* ssid, const char* pass);
    String getExpressionStateJSON(); // Only on ESP32
#endif
```

**Usage**:
```cpp
// Arduino Nano - simple usage
BotiEyes eyes;
eyes.initialize(config);
loop() { eyes.update(); }

// ESP32 - enhanced usage
BotiEyes eyes;
eyes.initialize(config);
#ifdef ESP32_OPTIMIZATIONS
    eyes.enableDualCore(); // Rendering on Core 1, auto-update at 60 FPS
    eyes.enableBLE("MyRobot"); // AI control via BLE
#endif
// No manual update() call needed - FreeRTOS task handles it
```

---

### Recommendation 2: Platform-Specific Build Targets

**platformio.ini**:
```ini
; Minimal build for Arduino Nano
[env:nano]
platform = atmelavr
board = nanoatmega328new
build_flags = -D MINIMAL_BUILD

; Standard build for Arduino Mega
[env:mega]
platform = atmelavr
board = megaatmega2560
build_flags = -D STANDARD_BUILD

; Enhanced build for ESP32
[env:esp32]
platform = espressif32
board = esp32dev
build_flags = 
    -D ESP32_OPTIMIZATIONS
    -D ENABLE_DUAL_CORE
    -D ENABLE_BLE
    -D ENABLE_JSON_EXPORT
    -D ENABLE_WIFI_DEBUG
```

---

### Recommendation 3: ESP32 Feature Matrix

| Feature | Arduino Nano | Arduino Mega | ESP32 Standard | ESP32 Enhanced |
|---------|-------------|-------------|----------------|----------------|
| Core emotion control | ✅ | ✅ | ✅ | ✅ |
| Coupled eye position | ✅ | ✅ | ✅ | ✅ |
| Blink/wink animations | ✅ | ✅ | ✅ | ✅ |
| Emotion helpers | ✅ | ✅ | ✅ | ✅ |
| Manual `update()` | ✅ | ✅ | ✅ | ❌ (auto FreeRTOS) |
| JSON export | ❌ | ❌ | ❌ | ✅ |
| BLE control | ❌ | ❌ | ❌ | ✅ |
| WiFi debugging | ❌ | ❌ | ❌ | ✅ |
| OTA updates | ❌ | ❌ | ❌ | ✅ |
| Dual-core rendering | ❌ | ❌ | ❌ | ✅ |
| I2C DMA | ❌ | ❌ | ❌ | ✅ |
| Expression caching | ❌ | ❌ | ❌ | ✅ |
| Target FPS | 15 | 20 | 30 | 60+ |

---

## 8. Testing Recommendations for ESP32

### Unit Tests (PlatformIO Native)
```ini
[env:native_esp32]
platform = native
test_framework = unity
build_flags = 
    -D UNIT_TEST
    -D ESP32_OPTIMIZATIONS
test_filter = test_esp32_*
```

**Test cases**:
- `test_esp32_dual_core_init` - Verify FreeRTOS task creation
- `test_esp32_ble_advertising` - BLE service starts correctly
- `test_esp32_json_export` - JSON serialization works
- `test_esp32_dma_transfer` - I2C DMA doesn't corrupt framebuffer

---

### Hardware Tests (ESP32 DevKit)
```cpp
// test/embedded/test_esp32_performance.cpp
void test_esp32_framerate() {
    BotiEyes eyes;
    eyes.initialize(config);
    #ifdef ESP32_OPTIMIZATIONS
        eyes.enableDualCore();
    #endif
    
    uint32_t frameCount = 0;
    uint32_t startTime = millis();
    
    while (millis() - startTime < 10000) { // 10 seconds
        #ifndef ESP32_OPTIMIZATIONS
            eyes.update(); // Manual call
        #endif
        frameCount++;
        delay(1); // Small yield for WiFi stack
    }
    
    float fps = frameCount / 10.0;
    TEST_ASSERT_GREATER_THAN(50.0, fps); // Expect 50+ FPS
}
```

---

## 9. Power Management for ESP32

### Light Sleep During Idle
```cpp
#ifdef ESP32
void BotiEyes::enterLightSleep() {
    // Stop rendering task
    vTaskSuspend(renderTaskHandle);
    
    // Turn off display
    display.clearDisplay();
    display.display();
    
    // Enter light sleep (WiFi maintained, CPU halted)
    esp_sleep_enable_timer_wakeup(100000); // Wake every 100ms
    esp_light_sleep_start();
    
    // Resume rendering
    vTaskResume(renderTaskHandle);
}
#endif
```

**Use Case**: Robot idle state - reduces power from 200mA → 50mA

---

### Deep Sleep for Battery Robots
```cpp
#ifdef ESP32
void BotiEyes::shutdownForDeepSleep() {
    // Stop rendering task
    vTaskDelete(renderTaskHandle);
    
    // Turn off display
    display.clearDisplay();
    display.display();
    
    // Configure wake source (e.g., touch sensor, timer)
    esp_sleep_enable_ext0_wakeup(GPIO_NUM_33, 1); // Wake on GPIO33 HIGH
    
    // Enter deep sleep (all peripherals off, 10µA)
    esp_deep_sleep_start();
    // Execution stops here; ESP32 reboots on wake
}
#endif
```

**Use Case**: Battery-powered robot - sleep mode until user interaction

---

## 10. Summary & Action Items

### ✅ What to Keep (Already Good)
- Current API design (works perfectly with FreeRTOS)
- Static memory allocation (no fragmentation)
- Emotion helper methods (ideal for WiFi/BLE control)
- Parametric valence-arousal model (continuous, AI-friendly)

### 🔧 Recommended ESP32 Enhancements (Optional, Backward Compatible)

**Priority 1 (High Impact)**:
1. **I2C DMA transfers** - Enable 60+ FPS (currently limited to 30 FPS)
2. **Dual-core rendering task** - Auto-update at 60 FPS, no manual `update()` calls
3. **BLE emotion control service** - Low-latency AI integration (20-50ms)

**Priority 2 (Medium Impact)**:
4. **Re-enable JSON export** - WiFi debugging (520KB RAM available)
5. **Expression parameter caching** - 10KB cache, instant emotion switches
6. **Hardware SPI support** - 100+ FPS on SPI-capable displays

**Priority 3 (Nice-to-Have)**:
7. **WiFi AP mode** - Field tuning without recompilation
8. **OTA updates** - Remote firmware updates
9. **SPIFFS presets** - Save custom emotions to flash
10. **Power management hooks** - Light/deep sleep for battery robots

### 📚 Documentation Additions Needed
- `docs/esp32-quickstart.md` - ESP32-specific setup guide
- `examples/ESP32_BLE_Control/` - BLE GATT service example
- `examples/ESP32_DualCore/` - FreeRTOS dual-core rendering
- `platformio.ini` - ESP32 build configurations

### 🧪 Testing Additions
- FreeRTOS task creation tests
- BLE service advertisement tests
- I2C DMA transfer validation
- 60 FPS performance benchmarks

---

## Conclusion

The BotiEyes library is **well-designed and will work excellently on ESP32**. However, treating ESP32 as just a "faster Arduino" leaves significant platform advantages on the table.

**Key Message**: Implement optional ESP32 enhancements (dual-core, BLE, DMA) behind compile-time flags to unlock:
- **4x performance** (15 FPS → 60 FPS)
- **Wireless AI integration** (BLE/WiFi control)
- **Field updates** (OTA)
- **Better power efficiency** (sleep modes)

All while maintaining full backward compatibility with Arduino Nano/Mega.

---

**Reviewer**: ESP32 Platform Expert  
**Date**: 2026-04-17  
**Status**: ✅ Approved with Enhancement Recommendations
