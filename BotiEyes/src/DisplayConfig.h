#ifndef DISPLAY_CONFIG_H
#define DISPLAY_CONFIG_H

#include <Arduino.h>

namespace BotiEyes {

/**
 * Display types supported by BotiEyes library
 */
enum DisplayType {
    DISPLAY_SSD1306_128x64,  // SSD1306 OLED 128x64
    DISPLAY_SSD1306_128x32,  // SSD1306 OLED 128x32
    DISPLAY_SH1106_128x64,   // SH1106 OLED 128x64
    DISPLAY_SSD1306_64x48    // SSD1306 OLED 64x48 (rare)
};

/**
 * Communication protocols
 */
enum Protocol {
    PROTOCOL_I2C,      // I2C (typical for OLED)
    PROTOCOL_SPI_HW,   // Hardware SPI
    PROTOCOL_SPI_SW    // Software SPI (bit-bang)
};

/**
 * Display configuration structure
 * Must be filled in by user before calling initialize()
 */
struct DisplayConfig {
    DisplayType type;       // Display model
    Protocol protocol;      // Communication method
    
    // Display dimensions (must match type)
    uint8_t width;          // Width in pixels
    uint8_t height;         // Height in pixels
    
    // I2C Configuration
    uint8_t i2cAddress;     // I2C address (typically 0x3C or 0x3D)
    
    // SPI Configuration (ignored if using I2C)
    int8_t cs_pin;          // Chip select pin (-1 = not used)
    int8_t dc_pin;          // Data/command pin (-1 = not used)
    int8_t reset_pin;       // Reset pin (-1 = not used)
};

} // namespace BotiEyes

#endif // DISPLAY_CONFIG_H
