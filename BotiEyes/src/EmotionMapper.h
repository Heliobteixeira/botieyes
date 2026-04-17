#ifndef EMOTION_MAPPER_H
#define EMOTION_MAPPER_H

#include "EmotionState.h"
#include "ExpressionParameters.h"

namespace BotiEyes {

/**
 * EmotionMapper - Valence-Arousal to Expression Parameters
 * 
 * Maps continuous valence-arousal coordinates to visual expression parameters.
 * Uses shape-based finalized design (production-approved).
 * 
 * Valence: -0.5 (negative) to +0.5 (positive)
 * Arousal: 0.0 (calm) to 1.0 (excited)
 * 
 * Mapping Formula (shape-based):
 * - Eye dimensions: Arousal-driven (high arousal = wider/taller)
 * - Eyelid coverage: Valence-driven (negative = top droop, positive = smile curve)
 * - Position: Combined arousal + valence
 * - Asymmetry: Reserved for Confused/Thinking (set explicitly via intensity, not here)
 */
class EmotionMapper {
public:
    /**
     * Map valence-arousal to expression parameters
     * 
     * @param valence Emotion polarity: -0.5 to +0.5
     * @param arousal Energy level: 0.0 to 1.0
     * @param params Output: computed expression parameters
     */
    static void mapEmotionToExpression(float valence, float arousal, ExpressionParameters* params);
    
    /**
     * Map with asymmetry override (for Confused/Thinking emotions)
     * 
     * @param valence Emotion polarity
     * @param arousal Energy level
     * @param asymmetry Asymmetry value (-0.3 to +0.3)
     * @param params Output: computed expression parameters
     */
    static void mapEmotionToExpressionWithAsymmetry(float valence, float arousal, float asymmetry, ExpressionParameters* params);
};

} // namespace BotiEyes

#endif // EMOTION_MAPPER_H
