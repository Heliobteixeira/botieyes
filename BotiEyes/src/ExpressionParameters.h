#ifndef EXPRESSION_PARAMETERS_H
#define EXPRESSION_PARAMETERS_H

#include <Arduino.h>

namespace BotiEyes {

/**
 * ExpressionParameters - Derived visual parameters
 * 
 * Computed from EmotionState by EmotionMapper.
 * Shape-based finalized design (production-approved):
 * - NO pupils, NO highlights, NO eyebrows
 * - Pure shape morphing (width/height modulation)
 * - Eyelid overlays create curvature
 * - Asymmetry only for Confused/Thinking
 * 
 * Achieved 8.4/10 expressiveness, 8.5/10 cuteness with this approach.
 */
struct ExpressionParameters {
    // Core shape dimensions (arousal-driven)
    uint8_t eyeWidth;           // Horizontal: 20-35 pixels (base ellipse)
    uint8_t eyeHeight;          // Vertical: 22-40 pixels (base ellipse)
    
    // Eyelid coverage (valence-driven, overlay shapes)
    float lidTopCoverage;       // 0.0 (none) to 0.5 (heavy droop)
    float lidBottomCoverage;    // 0.0 (none) to 0.5 (smile curve)
    
    // Positioning (emotion-driven)
    int8_t yOffset;             // Vertical shift: -5 to +8 pixels
    int8_t spacingAdjust;       // Inter-eye spacing: -3 to +1 pixels
    
    // Special effects (cognitive states only)
    float asymmetry;            // 0.0 (symmetric) to ±0.3 (asymmetric)
                                // Only for Confused (-0.30), Thinking (-0.20)
    
    /**
     * Initialize to default neutral expression
     */
    ExpressionParameters();
    
    /**
     * Validate that all parameters are within valid ranges
     * 
     * @return True if valid
     */
    bool isValid() const;
    
    /**
     * Clamp all parameters to valid ranges
     */
    void clamp();
};

} // namespace BotiEyes

#endif // EXPRESSION_PARAMETERS_H
