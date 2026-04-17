#include "Interpolator.h"

namespace BotiEyes {

float Interpolator::easeInOutCubic(float t) {
    // Clamp to [0.0, 1.0]
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    
    // Ease-in-out-cubic formula
    // First half (0.0 to 0.5): accelerate
    // Second half (0.5 to 1.0): decelerate
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = ((2.0f * t) - 2.0f);
        return 0.5f * f * f * f + 1.0f;
    }
}

float Interpolator::linear(float t) {
    // Clamp to [0.0, 1.0]
    if (t < 0.0f) return 0.0f;
    if (t > 1.0f) return 1.0f;
    return t;
}

} // namespace BotiEyes
