#include "RenderingHelpers.h"

namespace BotiEyes {

void RenderingHelpers::plotEllipsePoints(Adafruit_GFX* display, int16_t xc, int16_t yc,
                                        int16_t x, int16_t y, uint16_t color) {
    display->drawPixel(xc + x, yc + y, color);
    display->drawPixel(xc - x, yc + y, color);
    display->drawPixel(xc + x, yc - y, color);
    display->drawPixel(xc - x, yc - y, color);
}

void RenderingHelpers::fillEllipseLines(Adafruit_GFX* display, int16_t xc, int16_t yc,
                                       int16_t x, int16_t y, uint16_t color) {
    display->drawFastHLine(xc - x, yc + y, 2 * x + 1, color);
    display->drawFastHLine(xc - x, yc - y, 2 * x + 1, color);
}

void RenderingHelpers::drawEllipse(Adafruit_GFX* display, int16_t xCenter, int16_t yCenter,
                                  uint8_t width, uint8_t height, uint16_t color) {
    // Bresenham ellipse algorithm (outline only)
    int16_t a = width / 2;
    int16_t b = height / 2;
    
    int32_t a2 = a * a;
    int32_t b2 = b * b;
    int32_t fa2 = 4 * a2;
    int32_t fb2 = 4 * b2;
    
    // Region 1
    int16_t x = 0;
    int16_t y = b;
    int32_t sigma = 2 * b2 + a2 * (1 - 2 * b);
    
    while (b2 * x <= a2 * y) {
        plotEllipsePoints(display, xCenter, yCenter, x, y, color);
        
        if (sigma >= 0) {
            sigma += fa2 * (1 - y);
            y--;
        }
        sigma += b2 * ((4 * x) + 6);
        x++;
    }
    
    // Region 2
    x = a;
    y = 0;
    sigma = 2 * a2 + b2 * (1 - 2 * a);
    
    while (a2 * y <= b2 * x) {
        plotEllipsePoints(display, xCenter, yCenter, x, y, color);
        
        if (sigma >= 0) {
            sigma += fb2 * (1 - x);
            x--;
        }
        sigma += a2 * ((4 * y) + 6);
        y++;
    }
}

void RenderingHelpers::fillEllipse(Adafruit_GFX* display, int16_t xCenter, int16_t yCenter,
                                  uint8_t width, uint8_t height, uint16_t color) {
    // Filled Bresenham ellipse (draws horizontal lines)
    int16_t a = width / 2;
    int16_t b = height / 2;
    
    int32_t a2 = a * a;
    int32_t b2 = b * b;
    int32_t fa2 = 4 * a2;
    int32_t fb2 = 4 * b2;
    
    // Region 1
    int16_t x = 0;
    int16_t y = b;
    int32_t sigma = 2 * b2 + a2 * (1 - 2 * b);
    
    while (b2 * x <= a2 * y) {
        fillEllipseLines(display, xCenter, yCenter, x, y, color);
        
        if (sigma >= 0) {
            sigma += fa2 * (1 - y);
            y--;
        }
        sigma += b2 * ((4 * x) + 6);
        x++;
    }
    
    // Region 2
    x = a;
    y = 0;
    sigma = 2 * a2 + b2 * (1 - 2 * a);
    
    while (a2 * y <= b2 * x) {
        fillEllipseLines(display, xCenter, yCenter, x, y, color);
        
        if (sigma >= 0) {
            sigma += fb2 * (1 - x);
            x--;
        }
        sigma += a2 * ((4 * y) + 6);
        y++;
    }
}

void RenderingHelpers::drawEyelidOverlay(Adafruit_GFX* display, int16_t xCenter, int16_t yCenter,
                                        uint8_t width, uint8_t height, float coverage,
                                        bool isTop, uint16_t color) {
    if (coverage <= 0.0f) return;  // No overlay needed
    
    // Clamp coverage to [0.0, 0.5]
    if (coverage > 0.5f) coverage = 0.5f;
    
    // Calculate overlay dimensions
    int16_t halfWidth = width / 2;
    int16_t halfHeight = height / 2;
    int16_t coverageHeight = (int16_t)(halfHeight * coverage * 2.0f);  // 2x for visible effect
    
    if (isTop) {
        // Top eyelid: Draw filled rectangle from top of eye down
        int16_t y1 = yCenter - halfHeight;
        int16_t y2 = y1 + coverageHeight;
        display->fillRect(xCenter - halfWidth, y1, width, coverageHeight, color);

        // Curved edge: filled ellipse centered on the bottom edge of the rect,
        // its lower half bulges DOWN into the eye (droop curve).
        fillEllipse(display, xCenter, y2, width, coverageHeight, color);
    } else {
        // Bottom eyelid: Draw filled rectangle from bottom of eye up (smile curve)
        int16_t y2 = yCenter + halfHeight;
        int16_t y1 = y2 - coverageHeight;
        display->fillRect(xCenter - halfWidth, y1, width, coverageHeight, color);

        // Curved edge: filled ellipse centered on the top edge of the rect,
        // its upper half bulges UP into the eye (smile curve).
        fillEllipse(display, xCenter, y1, width, coverageHeight, color);
    }
}

} // namespace BotiEyes
