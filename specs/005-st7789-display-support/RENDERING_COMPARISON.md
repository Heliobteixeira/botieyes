# Rendering Comparison: SSD1306 vs ST7789

## How Pupil Drawing Works

BotiEyes doesn't actually draw "pupils" - it draws **filled ellipses** that represent the entire eye shape. The eyes are simple solid shapes that animate through size, position, and eyelid overlays.

## Complete Call Flow

### High-Level (Same for Both Drivers)

```cpp
// In BotiEyes::render() - line ~690 in BotiEyes.cpp
RenderingHelpers::fillEllipse(display, leftX, eyeY, width, height, 1);
                                                                    ↑
                                                            color = 1 (white)
```

### Mid-Level: fillEllipse Implementation (Same for Both)

```cpp
// In RenderingHelpers.cpp
void fillEllipse(Adafruit_GFX* display, int16_t xCenter, int16_t yCenter,
                 uint8_t width, uint8_t height, uint16_t color) {
    
    // Bresenham ellipse algorithm - computes (x,y) points
    // For each horizontal slice of the ellipse:
    fillEllipseLines(display, xCenter, yCenter, x, y, color);
}

void fillEllipseLines(Adafruit_GFX* display, int16_t xc, int16_t yc,
                      int16_t x, int16_t y, uint16_t color) {
    // Draw horizontal line to fill the ellipse
    display->drawFastHLine(xc - x, yc + y, 2 * x + 1, color);
    display->drawFastHLine(xc - x, yc - y, 2 * x + 1, color);
}
```

This is **hardware-agnostic** - BotiEyes has no idea what display is underneath!

### Low-Level: Where Drivers Differ

---

## Current: SSD1306 (Monochrome OLED)

```cpp
// esp-idf/components/display/src/esp_ssd1306.cpp

void ESP_SSD1306::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Bounds check
    if (x < 0 || x >= 128 || y < 0 || y >= 64) return;
    
    // 1-bit monochrome buffer: 128×64 = 8,192 bits = 1 KB
    uint16_t byte_index = x + (y / 8) * WIDTH;  // Which byte?
    uint8_t bit_mask = 1 << (y % 8);            // Which bit in byte?
    
    if (color) {
        _buffer[byte_index] |= bit_mask;   // Set bit (white pixel)
    } else {
        _buffer[byte_index] &= ~bit_mask;  // Clear bit (black pixel)
    }
}

void ESP_SSD1306::display() {
    // Flush entire 1KB buffer to SSD1306 via I2C/SPI
    ssd1306_set_buffer(&_dev, _buffer);
    ssd1306_show_buffer(&_dev);
}
```

**Color Handling**: `color` parameter is interpreted as:
- `0` = black (bit OFF)
- `non-zero` = white (bit ON)

---

## New: ST7789 (Color TFT)

```cpp
// esp-idf/components/display/src/esp_st7789.cpp (TO BE CREATED)

void ESP_ST7789::drawPixel(int16_t x, int16_t y, uint16_t color) {
    // Bounds check
    if (x < 0 || x >= 240 || y < 0 || y >= 135) return;
    
    if (_buffer) {
        // BUFFERED MODE: RGB565 frame buffer: 240×135×2 = 64,800 bytes = 65 KB
        uint32_t index = y * WIDTH + x;
        _buffer[index] = color;  // Store RGB565 color directly (16 bits)
        
    } else {
        // DIRECT MODE: Write pixel immediately to hardware via SPI
        // (Slower but no RAM overhead)
        lcdDrawPixel(&_dev, x, y, color);
    }
}

void ESP_ST7789::display() {
    if (_buffer) {
        // Flush entire 65KB RGB565 buffer to ST7789 via SPI
        // ~13ms at 40 MHz SPI for 240×135 display
        lcdDrawMultiPixels(&_dev, 0, 0, WIDTH, HEIGHT, _buffer);
    }
    // Direct mode: nothing to flush (already on screen)
}
```

**Color Handling**: `color` is RGB565 format:
- `0x0000` = black (RGB 0,0,0)
- `0xFFFF` = white (RGB 255,255,255)
- `0xF800` = red (RGB 255,0,0)
- `0x07E0` = green (RGB 0,255,0)
- `0x001F` = blue (RGB 0,0,255)

---

## Key Differences Summary

| Aspect | SSD1306 (Current) | ST7789 (New) |
|--------|-------------------|--------------|
| **Buffer format** | 1-bit monochrome | 16-bit RGB565 |
| **Buffer size** | 1 KB (128×64÷8) | 65 KB (240×135×2) |
| **drawPixel()** | Set/clear bit in buffer | Write 16-bit color to buffer |
| **Color param** | 0=black, 1=white | Full RGB565 (65,536 colors) |
| **display()** | I2C/SPI transfer 1KB | SPI transfer 65KB |
| **Transfer time** | ~5ms (I2C 400kHz) | ~13ms (SPI 40MHz) |
| **Memory overhead** | Negligible (1KB) | Significant (65KB = 12.5% of ESP32 SRAM) |

---

## What Stays the Same?

✅ **BotiEyes library**: Zero changes - it just calls `display->fillEllipse()` and doesn't care about the driver

✅ **Adafruit_GFX interface**: Both drivers implement `drawPixel()` the same way from BotiEyes' perspective

✅ **Rendering logic**: Same ellipse algorithm, same eyelid overlays, same animations

✅ **API calls**: `fillEllipse()`, `drawFastHLine()`, `fillScreen()` all work identically

---

## What Changes?

❌ **Pixel representation**: 1 bit → 16 bits

❌ **Memory usage**: 1 KB → 65 KB

❌ **Color support**: Monochrome → 65,536 colors

❌ **Buffer layout**: Vertical bit packing → linear RGB565 array

❌ **Hardware protocol**: I2C/SPI (SSD1306) → SPI only (ST7789)

---

## Practical Example: Drawing One Eye

### For a 40×30 pixel eye at center (64, 32):

**BotiEyes calls**:
```cpp
RenderingHelpers::fillEllipse(display, 64, 32, 40, 30, 1);
```

**What happens in SSD1306**:
1. Bresenham computes ~900 pixels for filled ellipse
2. Each `drawPixel()` sets 1 bit in 1KB buffer
3. `display()` sends 1024 bytes via I2C → ~5ms

**What happens in ST7789**:
1. Bresenham computes ~900 pixels for filled ellipse (same algorithm)
2. Each `drawPixel()` writes 2 bytes (RGB565 white = 0xFFFF) to 65KB buffer
3. `display()` sends 64,800 bytes via SPI → ~13ms

**Total pixels modified**: 900 (same for both)

**Bytes written to buffer**: 900 bits ≈ 113 bytes (SSD1306) vs 1,800 bytes (ST7789)

**Display update time**: ~5ms (SSD1306 I2C) vs ~13ms (ST7789 SPI 40MHz)

---

## Color in BotiEyes (Future Enhancement)

Currently BotiEyes passes `color=1` for all drawing (monochrome design). For ST7789 color support:

```cpp
// Potential future enhancement in BotiEyes
const uint16_t EYE_COLOR = 0xFFFF;        // White
const uint16_t PUPIL_COLOR = 0x0000;      // Black (if adding pupils)
const uint16_t IRIS_COLOR = 0x07E0;       // Green
const uint16_t BACKGROUND = 0x0000;       // Black

// Current (color-agnostic):
fillEllipse(display, x, y, w, h, 1);

// Future (color-aware):
fillEllipse(display, x, y, w, h, EYE_COLOR);
```

The adapter translates `color=1` → `0xFFFF` (white) automatically for ST7789, maintaining backward compatibility.

---

## Buffered vs Direct Mode

### Buffered Mode (Default for ST7789)
```cpp
ESP_ST7789 display(240, 135);  // Allocates 65KB frame buffer
display.drawPixel(10, 20, 0xF800);  // Writes to RAM
display.display();  // Flushes all 65KB to screen (~13ms)
```

**Pros**: Fast burst transfer, smooth animations  
**Cons**: 65KB RAM usage

### Direct Mode (Optional, Lower RAM)
```cpp
ESP_ST7789 display(240, 135, false);  // No frame buffer
display.drawPixel(10, 20, 0xF800);  // Writes directly to ST7789 via SPI
// No display() call needed
```

**Pros**: Only ~2KB RAM overhead  
**Cons**: Much slower (every pixel triggers SPI transaction), no atomic updates

**Recommendation**: Use buffered mode for BotiEyes (smooth animation requires atomic frame updates).
