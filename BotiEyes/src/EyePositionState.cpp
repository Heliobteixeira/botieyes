#include "EyePositionState.h"

namespace BotiEyes {

EyePositionState::EyePositionState()
    : horizontal(0)
    , vertical(0)  // Neutral position (0°, 0°)
    , targetH(0)
    , targetV(0)
    , startTime(0)
    , duration(300) {  // Default 300ms transitions
}

void EyePositionState::setTarget(int16_t h, int16_t v, uint16_t duration_ms) {
    // Clamp inputs
    clamp(&h, &v);
    
    // Set new target
    targetH = h;
    targetV = v;
    duration = duration_ms;
    startTime = millis();
}

void EyePositionState::updateInterpolation(float t) {
    // Clamp t to [0.0, 1.0]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    // Linear interpolation (easing applied externally)
    horizontal = (int16_t)(horizontal + t * (targetH - horizontal));
    vertical = (int16_t)(vertical + t * (targetV - vertical));
    
    // Ensure within bounds
    clamp(&horizontal, &vertical);
}

bool EyePositionState::isComplete() const {
    if (duration == 0) return true;
    return (millis() - startTime) >= duration;
}

void EyePositionState::clamp(int16_t* h, int16_t* v) {
    // Horizontal: -90° to +90°
    if (*h < -90) *h = -90;
    if (*h > 90) *h = 90;
    
    // Vertical: -45° to +45°
    if (*v < -45) *v = -45;
    if (*v > 45) *v = 45;
}

} // namespace BotiEyes
