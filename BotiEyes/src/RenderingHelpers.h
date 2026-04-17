#ifndef RENDERING_HELPERS_H
#define RENDERING_HELPERS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>

namespace BotiEyes {

/**
 * RenderingHelpers - Low-level drawing utilities
 * 
 * Custom rendering functions optimized for eye shapes.
 * Uses Bresenham algorithms and geometric primitives.
 */
class RenderingHelpers {
public:
    /**
     * Draw ellipse using Bresenham algorithm
     * 
     * Optimized for eye shapes. Draws outline of ellipse.
     * 
     * @param display GFX display to draw on
     * @param xCenter Center X coordinate
     * @param yCenter Center Y coordinate
     * @param width Ellipse width (horizontal axis)
     * @param height Ellipse height (vertical axis)
     * @param color Pixel color (typically 1 for white on OLED)
     */
    static void drawEllipse(Adafruit_GFX* display, int16_t xCenter, int16_t yCenter, 
                           uint8_t width, uint8_t height, uint16_t color);
    
    /**
     * Draw filled ellipse using Bresenham algorithm
     * 
     * @param display GFX display to draw on
     * @param xCenter Center X coordinate
     * @param yCenter Center Y coordinate
     * @param width Ellipse width (horizontal axis)
     * @param height Ellipse height (vertical axis)
     * @param color Pixel color
     */
    static void fillEllipse(Adafruit_GFX* display, int16_t xCenter, int16_t yCenter,
                           uint8_t width, uint8_t height, uint16_t color);
    
    /**
     * Draw eyelid overlay (curved mask shape)
     * 
     * Creates eyelid effect by drawing a filled shape that partially covers the eye.
     * Uses triangles or arc approximation for smooth curvature.
     * 
     * @param display GFX display to draw on
     * @param xCenter Eye center X
     * @param yCenter Eye center Y
     * @param width Eye width
     * @param height Eye height
     * @param coverage Coverage amount: 0.0 (none) to 0.5 (heavy)
     * @param isTop True for top eyelid, false for bottom
     * @param color Background color (typically 0 for black on OLED)
     */
    static void drawEyelidOverlay(Adafruit_GFX* display, int16_t xCenter, int16_t yCenter,
                                 uint8_t width, uint8_t height, float coverage,
                                 bool isTop, uint16_t color);
    
private:
    // Internal helpers for ellipse calculation
    static void plotEllipsePoints(Adafruit_GFX* display, int16_t xc, int16_t yc,
                                 int16_t x, int16_t y, uint16_t color);
    static void fillEllipseLines(Adafruit_GFX* display, int16_t xc, int16_t yc,
                                int16_t x, int16_t y, uint16_t color);
};

} // namespace BotiEyes

#endif // RENDERING_HELPERS_H
