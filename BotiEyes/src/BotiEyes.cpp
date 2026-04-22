#include "BotiEyes.h"
#include "EmotionState.h"
#include "EyePositionState.h"
#include "ExpressionParameters.h"
#include "EmotionMapper.h"
#include "Interpolator.h"
#include "RenderingHelpers.h"
#include <Adafruit_GFX.h>

namespace BotiEyes {

// Animation state structure
struct AnimationState {
    AnimationType type;
    uint32_t startTime;
    uint16_t duration;
    float progress;
    bool active;
    
    AnimationState() : type(ANIM_NONE), startTime(0), duration(0), progress(0.0f), active(false) {}
};

// Frame context: screen dimensions captured at initialize()
static uint8_t s_screenWidth  = 128;
static uint8_t s_screenHeight = 64;

// Constructor
BotiEyes::BotiEyes()
    : displayPtr(nullptr)
    , emotionState(nullptr)
    , positionState(nullptr)
    , animationState(nullptr)
    , idleBehaviorEnabled(false)
    , lastIdleTrigger(0)
    , initialized(false) {
}

// Destructor
BotiEyes::~BotiEyes() {
    if (emotionState) delete emotionState;
    if (positionState) delete positionState;
    if (animationState) delete animationState;
}

// === Initialization ===

ErrorCode BotiEyes::initialize(const DisplayConfig& config) {
    if (initialized) return INVALID_INPUT;  // Already initialized
    
    // Validate configuration
    ErrorCode validation = validateConfig(config);
    if (validation != OK) return validation;
    
    // Allocate state structures (static allocation preferred, but using new for simplicity)
    emotionState = new EmotionState();
    positionState = new EyePositionState();
    animationState = new AnimationState();
    
    if (!emotionState || !positionState || !animationState) {
        return MEMORY_ERROR;
    }
    
    // Note: Display initialization is done externally by user
    // We just store the pointer (passed via update() or set separately)
    
    s_screenWidth  = config.width;
    s_screenHeight = config.height;

    initialized = true;
    return OK;
}

void BotiEyes::setDisplay(Adafruit_GFX* display) {
    displayPtr = (void*)display;
}

ErrorCode BotiEyes::validateConfig(const DisplayConfig& config) {
    // Validate I2C address
    if (config.protocol == PROTOCOL_I2C) {
        if (config.i2cAddress != 0x3C && config.i2cAddress != 0x3D) {
            return INVALID_INPUT;  // Invalid I2C address
        }
    }
    
    // Validate SPI pins
    if (config.protocol == PROTOCOL_SPI_HW || config.protocol == PROTOCOL_SPI_SW) {
        if (config.cs_pin < -1 || config.dc_pin < -1 || config.reset_pin < -1) {
            return INVALID_INPUT;  // Invalid pin values
        }
    }
    
    // Validate dimensions match display type
    bool validDimensions = false;
    switch (config.type) {
        case DISPLAY_SSD1306_128x64:
        case DISPLAY_SH1106_128x64:
            validDimensions = (config.width == 128 && config.height == 64);
            break;
        case DISPLAY_SSD1306_128x32:
            validDimensions = (config.width == 128 && config.height == 32);
            break;
        case DISPLAY_SSD1306_64x48:
            validDimensions = (config.width == 64 && config.height == 48);
            break;
    }
    
    if (!validDimensions) return INVALID_INPUT;
    
    return OK;
}

// === Emotion Control ===

ErrorCode BotiEyes::setEmotion(float valence, float arousal, uint16_t duration_ms) {
    if (!initialized) return INVALID_INPUT;
    
    // Clamp and validate inputs
    EmotionState::clamp(&valence, &arousal);
    
    // Set new target
    emotionState->setTarget(valence, arousal, duration_ms);
    
    return OK;
}

void BotiEyes::getCurrentEmotion(float* valence, float* arousal) {
    if (!initialized || !emotionState) {
        *valence = 0.0f;
        *arousal = 0.5f;
        return;
    }
    
    *valence = emotionState->currentValence;
    *arousal = emotionState->currentArousal;
}

// === Emotion Helpers ===

ErrorCode BotiEyes::happy(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(0.35f * intensity, 0.55f * intensity + 0.45f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::sad(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(-0.35f * intensity, 0.35f * intensity + 0.5f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::angry(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(-0.30f * intensity, 0.80f * intensity + 0.2f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::calm(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(0.0f, 0.1f * intensity + 0.5f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::excited(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(0.30f * intensity, 0.90f * intensity + 0.1f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::tired(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(0.05f * intensity, 0.10f * intensity + 0.5f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::surprised(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(0.15f * intensity, 0.85f * intensity + 0.15f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::anxious(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(-0.20f * intensity, 0.75f * intensity + 0.25f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::content(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(0.25f * intensity, 0.40f * intensity + 0.5f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::curious(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    return setEmotion(0.15f * intensity, 0.60f * intensity + 0.4f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::thinking(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    // Note: Asymmetry handled in rendering, not in setEmotion
    return setEmotion(0.0f, 0.45f * intensity + 0.5f * (1.0f - intensity), 400);
}

ErrorCode BotiEyes::confused(float intensity) {
    if (intensity < 0.0f) intensity = 0.0f;
    if (intensity > 1.0f) intensity = 1.0f;
    // Note: Asymmetry handled in rendering, not in setEmotion
    return setEmotion(-0.15f * intensity, 0.55f * intensity + 0.45f * (1.0f - intensity), 400);
}

// === Eye Position Control ===

ErrorCode BotiEyes::setEyePosition(int16_t h, int16_t v, uint16_t duration_ms) {
    if (!initialized) return INVALID_INPUT;
    
    // Clamp and validate inputs
    EyePositionState::clamp(&h, &v);
    
    // Set new target
    positionState->setTarget(h, v, duration_ms);
    
    return OK;
}

void BotiEyes::getEyePosition(int16_t* h, int16_t* v) {
    if (!initialized || !positionState) {
        *h = 0;
        *v = 0;
        return;
    }
    
    *h = positionState->horizontal;
    *v = positionState->vertical;
}

// === Position Helper Behaviors ===

ErrorCode BotiEyes::lookLeft() {
    return setEyePosition(-45, 0, 300);
}

ErrorCode BotiEyes::lookRight() {
    return setEyePosition(45, 0, 300);
}

ErrorCode BotiEyes::lookUp() {
    return setEyePosition(0, 30, 300);
}

ErrorCode BotiEyes::lookDown() {
    return setEyePosition(0, -30, 300);
}

ErrorCode BotiEyes::neutral() {
    return setEyePosition(0, 0, 300);
}

// === Animations ===

ErrorCode BotiEyes::blink(uint16_t duration_ms) {
    if (!initialized) return INVALID_INPUT;
    
    animationState->type = ANIM_BLINK;
    animationState->startTime = millis();
    animationState->duration = duration_ms;
    animationState->progress = 0.0f;
    animationState->active = true;
    
    return OK;
}

ErrorCode BotiEyes::wink(EyeSide eye, uint16_t duration_ms) {
    if (!initialized) return INVALID_INPUT;
    
    animationState->type = (eye == LEFT) ? ANIM_WINK_LEFT : ANIM_WINK_RIGHT;
    animationState->startTime = millis();
    animationState->duration = duration_ms;
    animationState->progress = 0.0f;
    animationState->active = true;
    
    return OK;
}

ErrorCode BotiEyes::enableIdleBehavior(bool enable) {
    if (!initialized) return INVALID_INPUT;
    
    idleBehaviorEnabled = enable;
    if (enable) {
        lastIdleTrigger = millis();
    }
    
    return OK;
}

// === Frame Update ===

ErrorCode BotiEyes::update() {
    if (!initialized) return INVALID_INPUT;
    
    updateInterpolation();
    updateAnimation();
    updateIdleBehavior();
    
    // Render current frame if a display was attached
    if (displayPtr != nullptr) {
        renderEyes();
    }
    
    return OK;
}

// === Private Helper Methods ===

void BotiEyes::updateInterpolation() {
    uint32_t now = millis();
    
    // Update emotion interpolation
    if (!emotionState->isComplete()) {
        uint32_t elapsed = now - emotionState->startTime;
        float t = (float)elapsed / (float)emotionState->duration;
        t = Interpolator::easeInOutCubic(t);
        emotionState->updateInterpolation(t);
    }
    
    // Update position interpolation
    if (!positionState->isComplete()) {
        uint32_t elapsed = now - positionState->startTime;
        float t = (float)elapsed / (float)positionState->duration;
        t = Interpolator::easeInOutCubic(t);
        positionState->updateInterpolation(t);
    }
}

void BotiEyes::updateAnimation() {
    if (!animationState->active) return;
    
    uint32_t now = millis();
    uint32_t elapsed = now - animationState->startTime;
    
    if (elapsed >= animationState->duration) {
        // Animation complete
        animationState->active = false;
        animationState->progress = 1.0f;
    } else {
        // Update progress
        animationState->progress = (float)elapsed / (float)animationState->duration;
    }
}

void BotiEyes::updateIdleBehavior() {
    if (!idleBehaviorEnabled) return;

    uint32_t now = millis();

    // --- Blink scheduler ---------------------------------------------------
    // Next blink time is pre-computed; re-draw only after each blink fires.
    // Distribution: average of 3 uniform samples over [MIN..MAX] gives a
    // bell-shaped (Irwin-Hall) curve centered at (MIN+MAX)/2, naturally
    // clamped, no float / sqrt / log / cos needed.
    //
    // Human spontaneous inter-blink: ~2-10 s, mean ~4 s -> use 2000..8000 ms.
    // Human blink closure:           ~100-300 ms, mean ~180 ms.
    static uint32_t nextBlinkAt = 0;

    if (nextBlinkAt == 0) {
        uint32_t a = random(2000, 8001);
        uint32_t b = random(2000, 8001);
        uint32_t c = random(2000, 8001);
        nextBlinkAt = lastIdleTrigger + (a + b + c) / 3;
    }

    if ((int32_t)(now - nextBlinkAt) >= 0) {
        uint32_t d = (random(100, 301) + random(100, 301)) / 2;  // mean ~200
        blink((uint16_t)d);
        lastIdleTrigger = now;
        nextBlinkAt = 0;  // sample a fresh interval next frame
    }

    // --- Gaze scheduler ----------------------------------------------------
    // Humans make small spontaneous gaze shifts (saccades + micro-saccades)
    // every ~0.5-4 s while at rest. Most are small drifts near center; a
    // minority are large look-around shifts that are quickly followed by a
    // return toward center. Movement itself is fast (80-150 ms saccade).
    static uint32_t nextGazeAt      = 0;
    static uint32_t lastGazeTrigger = 0;
    static bool     pendingRecenter = false;

    if (nextGazeAt == 0) {
        if (pendingRecenter) {
            // Short hold after a large look-away, then return toward center
            nextGazeAt = lastGazeTrigger + (uint32_t)random(400, 1201);
        } else {
            uint32_t a = random(500, 4001);
            uint32_t b = random(500, 4001);
            nextGazeAt = lastGazeTrigger + (a + b) / 2;  // mean ~2.25 s
        }
    }

    if ((int32_t)(now - nextGazeAt) >= 0) {
        int16_t h, v;
        if (pendingRecenter) {
            // Small drift centered on 0 -> effectively a re-center
            h = (int16_t)random(-8, 9);
            v = (int16_t)random(-4, 5);
            pendingRecenter = false;
        } else if (random(0, 100) < 80) {
            // Small drift near center: +/- 15 deg horiz, +/- 8 deg vert
            h = (int16_t)random(-15, 16);
            v = (int16_t)random(-8, 9);
        } else {
            // Larger look-around: +/- 45 deg horiz, +/- 20 deg vert
            h = (int16_t)random(-45, 46);
            v = (int16_t)random(-20, 21);
            pendingRecenter = true;  // next shift re-centers
        }
        uint16_t saccade = (uint16_t)random(80, 151);
        setEyePosition(h, v, saccade);
        lastGazeTrigger = now;
        nextGazeAt = 0;
    }
}

void BotiEyes::renderEyes() {
    if (!displayPtr || !emotionState || !positionState) return;

    Adafruit_GFX* display = static_cast<Adafruit_GFX*>(displayPtr);

    // 1. Map current emotion -> expression parameters
    ExpressionParameters params;
    EmotionMapper::mapEmotionToExpression(
        emotionState->currentValence,
        emotionState->currentArousal,
        &params);
    params.clamp();

    // 2. Blink / wink: scale vertical openness by (1 - progress curve)
    //    Uses a triangular profile: closes then reopens
    float leftOpen  = 1.0f;
    float rightOpen = 1.0f;
    if (animationState && animationState->active) {
        float p = animationState->progress;        // 0..1
        float closed = (p < 0.5f) ? (p * 2.0f) : ((1.0f - p) * 2.0f);  // 0->1->0
        if (closed > 1.0f) closed = 1.0f;
        float openness = 1.0f - closed;
        switch (animationState->type) {
            case ANIM_BLINK:      leftOpen = rightOpen = openness; break;
            case ANIM_WINK_LEFT:  leftOpen  = openness; break;
            case ANIM_WINK_RIGHT: rightOpen = openness; break;
            default: break;
        }
    }

    // 3. Eye layout on the screen
    const int16_t cx = s_screenWidth  / 2;
    const int16_t cy = s_screenHeight / 2;

    // Base spacing: eyes sit a little inside of 1/4 and 3/4 horizontally
    // (reduced from s_screenWidth/4 so the pair reads as focused/close-range)
    int16_t spacing = (s_screenWidth * 7 / 32) + params.spacingAdjust;
    int16_t leftX   = cx - spacing;
    int16_t rightX  = cx + spacing;

    // 4. Position offset from coupled gaze (angles -> pixels)
    //    Horizontal: -90..+90 -> ~-16..+16 px
    //    Vertical:   -45..+45 -> ~-10..+10 px
    int16_t gazeX = (int32_t)positionState->horizontal * 16 / 90;
    int16_t gazeY = -(int32_t)positionState->vertical   * 10 / 45;  // screen Y inverted

    // Vergence (disjunctive movement): when gaze goes off-center, simulate
    // fixation on a medium-close object -- the eye NEARER the target moves
    // less, the FARTHER eye moves more, so both converge nasally.
    // Magnitude: ~30 % of |gazeX| (stronger convergence for a close object).
    int16_t absGazeX   = gazeX < 0 ? -gazeX : gazeX;
    int16_t verge      = (int16_t)(absGazeX * 10 / 100);
    int16_t gazeXLeft  = gazeX + verge;   // left  eye shifted further right
    int16_t gazeXRight = gazeX - verge;   // right eye shifted further left

    int16_t eyeY = cy + params.yOffset + gazeY;

    // 5. Per-eye sizes (with optional asymmetry)
    uint8_t lW = params.eyeWidth;
    uint8_t lH = params.eyeHeight;
    uint8_t rW = params.eyeWidth;
    uint8_t rH = params.eyeHeight;
    if (params.asymmetry != 0.0f) {
        // Negative asymmetry -> left eye smaller, right eye bigger (Confused/Thinking)
        float a = params.asymmetry;
        lH = (uint8_t)constrain((int)(lH * (1.0f + a)), 4, 60);
        rH = (uint8_t)constrain((int)(rH * (1.0f - a)), 4, 60);
    }

    // Apply blink/wink vertical scaling (min 2px so we still see a line)
    lH = (uint8_t)max(2, (int)(lH * leftOpen));
    rH = (uint8_t)max(2, (int)(rH * rightOpen));

    // 6. Clear and draw
    display->fillScreen(0);

    // Filled eye ellipses (pupils-free shape design)
    RenderingHelpers::fillEllipse(display, leftX  + gazeXLeft,  eyeY, lW, lH, 1);
    RenderingHelpers::fillEllipse(display, rightX + gazeXRight, eyeY, rW, rH, 1);

    // Eyelid overlays drawn in background color (0) to carve shape
    if (params.lidTopCoverage > 0.0f) {
        RenderingHelpers::drawEyelidOverlay(display, leftX  + gazeXLeft,  eyeY,
                                            lW, lH, params.lidTopCoverage, true, 0);
        RenderingHelpers::drawEyelidOverlay(display, rightX + gazeXRight, eyeY,
                                            rW, rH, params.lidTopCoverage, true, 0);
    }
    if (params.lidBottomCoverage > 0.0f) {
        RenderingHelpers::drawEyelidOverlay(display, leftX  + gazeXLeft,  eyeY,
                                            lW, lH, params.lidBottomCoverage, false, 0);
        RenderingHelpers::drawEyelidOverlay(display, rightX + gazeXRight, eyeY,
                                            rW, rH, params.lidBottomCoverage, false, 0);
    }
}

} // namespace BotiEyes
