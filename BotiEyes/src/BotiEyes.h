#ifndef BOTIEYES_H
#define BOTIEYES_H

#include <Arduino.h>
#include "DisplayConfig.h"

namespace BotiEyes {

/**
 * Error codes returned by BotiEyes API methods
 */
enum ErrorCode {
    OK = 0,                 // Success
    INVALID_INPUT,          // Invalid parameter values
    HARDWARE_ERROR,         // Display hardware failure
    TIMEOUT,                // Operation timed out
    DISPLAY_NOT_FOUND,      // Display not responding
    MEMORY_ERROR            // Memory allocation failed
};

/**
 * Eye side for wink animations
 */
enum EyeSide {
    LEFT = 0,
    RIGHT = 1
};

/**
 * Animation types
 */
enum AnimationType {
    ANIM_NONE = 0,
    ANIM_BLINK,
    ANIM_WINK_LEFT,
    ANIM_WINK_RIGHT
};

// Forward declarations
struct EmotionState;
struct EyePositionState;
struct ExpressionParameters;
struct AnimationState;
class Interpolator;
class EmotionMapper;

} // namespace BotiEyes

class Adafruit_GFX;

namespace BotiEyes {

/**
 * BotiEyes - Main library class
 * 
 * Parametric emotion-driven eye animation library for OLED displays.
 * Provides continuous emotional expression using valence-arousal model,
 * coupled 2D eye position control, and built-in animations.
 */
class BotiEyes {
public:
    BotiEyes();
    ~BotiEyes();
    
    // === Initialization ===
    
    /**
     * Initialize display hardware and library state
     * After successful init, eyes show neutral expression (0.0, 0.5)
     * 
     * @param config Display configuration (type, protocol, pins)
     * @return ErrorCode::OK on success, error code otherwise
     */
    ErrorCode initialize(const DisplayConfig& config);
    
    /**
     * Attach an Adafruit_GFX-compatible display for rendering.
     * Must be called (after display.begin()) before update() will draw.
     *
     * @param display Pointer to initialized Adafruit_GFX display
     */
    void setDisplay(Adafruit_GFX* display);
    
    /**
     * Validate display configuration before initialization
     * 
     * @param config Configuration to validate
     * @return ErrorCode::OK if valid, INVALID_INPUT otherwise
     */
    static ErrorCode validateConfig(const DisplayConfig& config);
    
    // === Emotion Control ===
    
    /**
     * Set target emotional state with smooth transition
     * 
     * @param valence Emotion polarity: -0.5 (negative) to +0.5 (positive)
     * @param arousal Energy level: 0.0 (calm) to 1.0 (excited)
     * @param duration_ms Transition duration in milliseconds (default: 400)
     * @return ErrorCode::OK on success, INVALID_INPUT if out of bounds
     */
    ErrorCode setEmotion(float valence, float arousal, uint16_t duration_ms = 400);
    
    /**
     * Get current interpolated emotion state
     * 
     * @param valence Output: current valence value
     * @param arousal Output: current arousal value
     */
    void getCurrentEmotion(float* valence, float* arousal);
    
    // === Emotion Helpers (Simplified Facade) ===
    
    ErrorCode happy(float intensity = 1.0);     // Valence: +0.35, Arousal: 0.55
    ErrorCode sad(float intensity = 1.0);       // Valence: -0.35, Arousal: 0.35
    ErrorCode angry(float intensity = 1.0);     // Valence: -0.30, Arousal: 0.80
    ErrorCode calm(float intensity = 1.0);      // Valence: 0.0, Arousal: 0.1
    ErrorCode excited(float intensity = 1.0);   // Valence: +0.30, Arousal: 0.90
    ErrorCode tired(float intensity = 1.0);     // Valence: +0.05, Arousal: 0.10
    ErrorCode surprised(float intensity = 1.0); // Valence: +0.15, Arousal: 0.85
    ErrorCode anxious(float intensity = 1.0);   // Valence: -0.20, Arousal: 0.75
    ErrorCode content(float intensity = 1.0);   // Valence: +0.25, Arousal: 0.40
    ErrorCode curious(float intensity = 1.0);   // Valence: +0.15, Arousal: 0.60
    ErrorCode thinking(float intensity = 1.0);  // Valence: 0.0, Arousal: 0.45, Asymmetry: -0.20
    ErrorCode confused(float intensity = 1.0);  // Valence: -0.15, Arousal: 0.55, Asymmetry: -0.30
    ErrorCode neutral(float intensity = 1.0);   // Valence: 0.0,  Arousal: 0.0  (flat affect)
    
    // === Eye Position Control (Coupled - Both Eyes Together) ===
    
    /**
     * Set coupled eye gaze position with smooth transition
     * Both eyes move together to maintain parallel gaze
     * 
     * @param h Horizontal angle: -90° (left) to +90° (right)
     * @param v Vertical angle: -45° (down) to +45° (up)
     * @param duration_ms Transition duration in milliseconds (default: 300)
     * @return ErrorCode::OK on success, INVALID_INPUT if out of bounds
     */
    ErrorCode setEyePosition(int16_t h, int16_t v, uint16_t duration_ms = 300);
    
    /**
     * Get current interpolated eye position
     * 
     * @param h Output: current horizontal angle
     * @param v Output: current vertical angle
     */
    void getEyePosition(int16_t* h, int16_t* v);
    
    // === Position Helper Behaviors ===
    
    ErrorCode lookLeft();    // (-45°, 0°)
    ErrorCode lookRight();   // (+45°, 0°)
    ErrorCode lookUp();      // (0°, +30°)
    ErrorCode lookDown();    // (0°, -30°)
    ErrorCode lookNeutral(); // (0°, 0°)
    
    // === Animations ===
    
    /**
     * Blink both eyes
     * 
     * @param duration_ms Blink duration (default: 150ms)
     * @return ErrorCode::OK on success
     */
    ErrorCode blink(uint16_t duration_ms = 150);
    
    /**
     * Wink one eye
     * 
     * @param eye Which eye to wink (LEFT or RIGHT)
     * @param duration_ms Wink duration (default: 150ms)
     * @return ErrorCode::OK on success
     */
    ErrorCode wink(EyeSide eye, uint16_t duration_ms = 150);
    
    /**
     * Enable/disable idle behavior (periodic micro-blinks + subtle morphing)
     * 
     * @param enable True to enable, false to disable
     * @return ErrorCode::OK on success
     */
    ErrorCode enableIdleBehavior(bool enable = true);
    
    // === Frame Update ===
    
    /**
     * Update emotion/position interpolation and render current frame
     * Must be called at target FPS (e.g., 20 Hz = every 50ms)
     * 
     * @return ErrorCode::OK on success
     */
    ErrorCode update();
    
private:
    // Private implementation (to be implemented in Phase 3)
    void* displayPtr;  // Adafruit_GFX* display (void* to avoid header dependency)
    
    EmotionState* emotionState;
    EyePositionState* positionState;
    AnimationState* animationState;
    
    bool idleBehaviorEnabled;
    uint32_t lastIdleTrigger;

    // Baseline emotion (last user-requested target) used by idle morphing
    // so subtle drifts return toward a stable anchor rather than compounding.
    float baselineValence;
    float baselineArousal;

    bool initialized;
    
    // Helper methods
    void renderEyes();
    void updateInterpolation();
    void updateAnimation();
    void updateIdleBehavior();
};

} // namespace BotiEyes

#endif // BOTIEYES_H
