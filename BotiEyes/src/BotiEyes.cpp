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
    
    initialized = true;
    return OK;
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
    
    // Note: Actual rendering to display is done externally by user after update()
    // This just updates internal state
    
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
    uint32_t elapsed = now - lastIdleTrigger;
    
    // Trigger micro-blink every 3-5 seconds (random)
    uint32_t interval = 3000 + (random(2000));  // 3000-5000ms
    
    if (elapsed >= interval) {
        blink(100);  // Quick micro-blink (100ms)
        lastIdleTrigger = now;
    }
}

void BotiEyes::renderEyes() {
    // This method will be implemented to actually draw to display
    // For now, it's a placeholder that will be filled in with rendering logic
    // Actual rendering should be done by user after calling update()
}

} // namespace BotiEyes
