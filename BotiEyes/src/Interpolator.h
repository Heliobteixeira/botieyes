#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

namespace BotiEyes {

/**
 * Interpolator - Smooth transition utilities
 * 
 * Provides easing functions for smooth animations.
 * Uses ease-in-out-cubic for natural-looking transitions.
 */
class Interpolator {
public:
    /**
     * Ease-in-out-cubic easing function
     * 
     * Smooth acceleration at start, smooth deceleration at end.
     * 
     * @param t Normalized time [0.0, 1.0]
     * @return Eased value [0.0, 1.0]
     */
    static float easeInOutCubic(float t);
    
    /**
     * Linear interpolation (no easing)
     * 
     * @param t Normalized time [0.0, 1.0]
     * @return Same as input (linear)
     */
    static float linear(float t);
};

} // namespace BotiEyes

#endif // INTERPOLATOR_H
