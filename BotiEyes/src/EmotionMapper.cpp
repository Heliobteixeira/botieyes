#include "EmotionMapper.h"
#include <Arduino.h>

namespace BotiEyes {

void EmotionMapper::mapEmotionToExpression(float valence, float arousal, ExpressionParameters* params) {
    mapEmotionToExpressionWithAsymmetry(valence, arousal, 0.0f, params);
}

void EmotionMapper::mapEmotionToExpressionWithAsymmetry(float valence, float arousal, float asymmetry, ExpressionParameters* params) {
    // Clamp inputs
    if (valence < -0.5f) valence = -0.5f;
    if (valence > 0.5f) valence = 0.5f;
    if (arousal < 0.0f) arousal = 0.0f;
    if (arousal > 1.0f) arousal = 1.0f;
    if (asymmetry < -0.3f) asymmetry = -0.3f;
    if (asymmetry > 0.3f) asymmetry = 0.3f;
    
    // === Eye Dimensions (Arousal-Driven) ===
    // High arousal → wider, taller eyes (alert, surprised)
    // Low arousal → narrower, shorter eyes (sleepy, calm)
    params->eyeWidth = (uint8_t)(20 + arousal * 15);    // 20-35 pixels
    params->eyeHeight = (uint8_t)(22 + arousal * 18);   // 22-40 pixels
    
    // === Eyelid Coverage (Valence-Driven, Minimalist Design) ===
    if (valence < 0) {
        // Negative emotions: top lid droops (sad, tired, angry)
        params->lidTopCoverage = abs(valence);  // 0.0-0.5
        params->lidBottomCoverage = 0.0f;
    } else {
        // Positive emotions: bottom lid curves up (happy, smile effect)
        params->lidTopCoverage = 0.0f;
        params->lidBottomCoverage = valence * 0.6f;  // 0.0-0.3 (clamped)
    }
    
    // === Vertical Offset (Arousal + Valence) ===
    // High arousal + positive valence → eyes up (excited)
    // Low arousal → eyes down (tired, sad)
    int16_t offset = (int16_t)((arousal - 0.5f) * 8 + valence * 6);
    params->yOffset = (int8_t)constrain(offset, -5, 8);
    
    // === Spacing Adjustment (Arousal-Driven) ===
    // High arousal → eyes closer (intensity, focus)
    // Low arousal → eyes farther apart (relaxed)
    int16_t spacing = (int16_t)((0.5f - arousal) * 8);
    params->spacingAdjust = (int8_t)constrain(spacing, -3, 1);
    
    // === Asymmetry (Cognitive States: Confused/Thinking) ===
    params->asymmetry = asymmetry;
    
    // Final validation
    params->clamp();
}

} // namespace BotiEyes
