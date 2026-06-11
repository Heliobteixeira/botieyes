# BotiEyes ESP-IDF Component Wrapper
# References the original BotiEyes library sources without copying

set(BOTIEYES_ROOT "../../../BotiEyes/src")

idf_component_register(
    SRCS
        "${BOTIEYES_ROOT}/BotiEyes.cpp"
        "${BOTIEYES_ROOT}/EmotionMapper.cpp"
        "${BOTIEYES_ROOT}/EmotionState.cpp"
        "${BOTIEYES_ROOT}/ExpressionParameters.cpp"
        "${BOTIEYES_ROOT}/EyePositionState.cpp"
        "${BOTIEYES_ROOT}/Interpolator.cpp"
        "${BOTIEYES_ROOT}/PlatformRandom.cpp"
        "${BOTIEYES_ROOT}/PlatformTime.cpp"
        "${BOTIEYES_ROOT}/RenderingHelpers.cpp"
        "${BOTIEYES_ROOT}/net/BotiEyesServer.cpp"
        "${BOTIEYES_ROOT}/net/CommandCodec.cpp"
        "${BOTIEYES_ROOT}/net/SessionManager.cpp"
    INCLUDE_DIRS
        "${BOTIEYES_ROOT}"
    REQUIRES
        esp_timer
        # Display driver dependency: will search for Adafruit GFX or alternatives
)
