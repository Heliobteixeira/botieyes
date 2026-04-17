#include "ExpressionParameters.h"

namespace BotiEyes {

ExpressionParameters::ExpressionParameters()
    : eyeWidth(28)              // Neutral width
    , eyeHeight(30)             // Neutral height
    , lidTopCoverage(0.0f)
    , lidBottomCoverage(0.0f)
    , yOffset(0)
    , spacingAdjust(0)
    , asymmetry(0.0f) {
}

bool ExpressionParameters::isValid() const {
    if (eyeWidth < 20 || eyeWidth > 35) return false;
    if (eyeHeight < 22 || eyeHeight > 40) return false;
    if (lidTopCoverage < 0.0f || lidTopCoverage > 0.5f) return false;
    if (lidBottomCoverage < 0.0f || lidBottomCoverage > 0.5f) return false;
    if (yOffset < -5 || yOffset > 8) return false;
    if (spacingAdjust < -3 || spacingAdjust > 1) return false;
    if (asymmetry < -0.3f || asymmetry > 0.3f) return false;
    
    return true;
}

void ExpressionParameters::clamp() {
    // Eye dimensions
    if (eyeWidth < 20) eyeWidth = 20;
    if (eyeWidth > 35) eyeWidth = 35;
    if (eyeHeight < 22) eyeHeight = 22;
    if (eyeHeight > 40) eyeHeight = 40;
    
    // Eyelid coverage
    if (lidTopCoverage < 0.0f) lidTopCoverage = 0.0f;
    if (lidTopCoverage > 0.5f) lidTopCoverage = 0.5f;
    if (lidBottomCoverage < 0.0f) lidBottomCoverage = 0.0f;
    if (lidBottomCoverage > 0.5f) lidBottomCoverage = 0.5f;
    
    // Positioning
    if (yOffset < -5) yOffset = -5;
    if (yOffset > 8) yOffset = 8;
    if (spacingAdjust < -3) spacingAdjust = -3;
    if (spacingAdjust > 1) spacingAdjust = 1;
    
    // Asymmetry
    if (asymmetry < -0.3f) asymmetry = -0.3f;
    if (asymmetry > 0.3f) asymmetry = 0.3f;
}

} // namespace BotiEyes
