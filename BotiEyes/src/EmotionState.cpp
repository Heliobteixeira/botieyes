#include "EmotionState.h"

namespace BotiEyes {

EmotionState::EmotionState() 
    : currentValence(0.0f)
    , currentArousal(0.5f)  // Neutral expression (0.0, 0.5)
    , targetValence(0.0f)
    , targetArousal(0.5f)
    , startTime(0)
    , duration(0) {
}

void EmotionState::setTarget(float valence, float arousal, uint16_t duration_ms) {
    // Clamp inputs
    clamp(&valence, &arousal);
    
    // Set new target
    targetValence = valence;
    targetArousal = arousal;
    duration = duration_ms;
    startTime = millis();
}

void EmotionState::updateInterpolation(float t) {
    // Clamp t to [0.0, 1.0]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    // Linear interpolation (easing applied externally)
    currentValence = currentValence + t * (targetValence - currentValence);
    currentArousal = currentArousal + t * (targetArousal - currentArousal);
    
    // Ensure within bounds
    clamp(&currentValence, &currentArousal);
}

bool EmotionState::isComplete() const {
    if (duration == 0) return true;
    return (millis() - startTime) >= duration;
}

void EmotionState::clamp(float* valence, float* arousal) {
    // Valence: -0.5 to +0.5
    if (*valence < -0.5f) *valence = -0.5f;
    if (*valence > 0.5f) *valence = 0.5f;
    
    // Arousal: 0.0 to 1.0
    if (*arousal < 0.0f) *arousal = 0.0f;
    if (*arousal > 1.0f) *arousal = 1.0f;
}

} // namespace BotiEyes
