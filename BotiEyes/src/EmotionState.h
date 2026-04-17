#ifndef EMOTION_STATE_H
#define EMOTION_STATE_H

#include <Arduino.h>

namespace BotiEyes {

/**
 * EmotionState - Represents current and target emotional state
 * 
 * Uses valence-arousal model:
 * - Valence: -0.5 (negative) to +0.5 (positive)
 * - Arousal: 0.0 (calm) to 1.0 (excited)
 * 
 * Supports smooth transitions with interpolation.
 */
struct EmotionState {
    // Current interpolated values
    float currentValence;    // Current emotion: -0.5 to +0.5
    float currentArousal;    // Current energy: 0.0 to 1.0
    
    // Target values for interpolation
    float targetValence;     // Target emotion
    float targetArousal;     // Target energy
    
    // Interpolation state
    uint32_t startTime;      // Interpolation start timestamp (millis())
    uint16_t duration;       // Transition duration in milliseconds
    
    /**
     * Initialize to neutral expression (0.0, 0.5)
     */
    EmotionState();
    
    /**
     * Set new target emotion and start interpolation
     * 
     * @param valence Target valence (-0.5 to +0.5)
     * @param arousal Target arousal (0.0 to 1.0)
     * @param duration_ms Transition duration
     */
    void setTarget(float valence, float arousal, uint16_t duration_ms);
    
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
     * Clamp valence and arousal to valid ranges
     * 
     * @param valence Value to clamp
     * @param arousal Value to clamp
     */
    static void clamp(float* valence, float* arousal);
};

} // namespace BotiEyes

#endif // EMOTION_STATE_H
