# BotiEyes ESP-IDF Component
# Makes the BotiEyes library available as a native ESP-IDF component via EXTRA_COMPONENT_DIRS

# Override component name (would default to "src" from directory name)
set(COMPONENT_NAME "botieyes")

idf_component_register(
    SRCS
        "BotiEyes.cpp"
        "EmotionMapper.cpp"
        "EmotionState.cpp"
        "ExpressionParameters.cpp"
        "EyePositionState.cpp"
        "Interpolator.cpp"
        "PlatformRandom.cpp"
        "PlatformTime.cpp"
        "RenderingHelpers.cpp"
        "net/BotiEyesServer.cpp"
        "net/CommandCodec.cpp"
        "net/SessionManager.cpp"
    INCLUDE_DIRS
        "."
    REQUIRES
        # Will be added based on display driver choice (Adafruit GFX or alternatives)
)
