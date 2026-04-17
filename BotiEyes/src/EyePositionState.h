#ifndef EYE_POSITION_STATE_H
#define EYE_POSITION_STATE_H

#include <Arduino.h>

namespace BotiEyes {

/**
 * EyePositionState - Coupled 2D position for both eyes
 * 
 * Both eyes move together (v1 design - coupled control).
 * Independent per-eye control deferred to v2.
 * 
 * Horizontal: -90° (full left) to +90° (full right)
 * Vertical: -45° (down) to +45° (up)
 */
struct EyePositionState {
    // Current interpolated position (both eyes)
    int16_t horizontal;      // -90° to +90°
    int16_t vertical;        // -45° to +45°
    
    // Target position for interpolation
    int16_t targetH;
    int16_t targetV;
    
    // Interpolation state
    uint32_t startTime;      // Interpolation start (millis())
    uint16_t duration;       // Transition duration (default: 300ms)
    
    /**
     * Initialize to neutral position (0°, 0°)
     */
    EyePositionState();
    
    /**
     * Set new target position and start interpolation
     * 
     * @param h Target horizontal angle (-90 to +90)
     * @param v Target vertical angle (-45 to +45)
     * @param duration_ms Transition duration
     */
    void setTarget(int16_t h, int16_t v, uint16_t duration_ms);
    
    /**
     * Update interpolation (call every frame)
     * 
     * @param t Normalized time [0.0, 1.0]
     */
    void updateInterpolation(float t);
    
    /**
     * Check if interpolation is complete
     * 
     * @return True if reached target
     */
    bool isComplete() const;
    
    /**
     * Clamp angles to valid ranges
     * 
     * @param h Horizontal angle to clamp
     * @param v Vertical angle to clamp
     */
    static void clamp(int16_t* h, int16_t* v);
};

} // namespace BotiEyes

#endif // EYE_POSITION_STATE_H
