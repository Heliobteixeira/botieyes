# Display Component - Multi-driver Display Adapter
#
# Provides display initialization and adapter classes (SSD1306, ST7789)
# as a bridge to nopnop2002 component drivers via Adafruit_GFX interface.

set(DISPLAY_SRCS
    "src/display_init.cpp"
)

set(DISPLAY_REQUIRES
    botieyes
    adafruit_gfx
)

# Conditionally add display driver based on menuconfig selection
if(CONFIG_DISPLAY_TYPE_ST7789_SPI)
    message(STATUS "ST7789 SPI display selected - adding sources and dependencies")
    list(APPEND DISPLAY_SRCS "src/esp_st7789.cpp")
    list(APPEND DISPLAY_REQUIRES nopnop2002__st7789)
elseif(CONFIG_DISPLAY_TYPE_SSD1306_I2C OR CONFIG_DISPLAY_TYPE_SSD1306_SPI)
    message(STATUS "SSD1306 display selected - adding sources and dependencies")
    list(APPEND DISPLAY_SRCS "src/esp_ssd1306.cpp")
    list(APPEND DISPLAY_REQUIRES ssd1306)
endif()

message(STATUS "Final DISPLAY_SRCS: ${DISPLAY_SRCS}")
message(STATUS "Final DISPLAY_REQUIRES: ${DISPLAY_REQUIRES}")

idf_component_register(
    SRCS
        ${DISPLAY_SRCS}
    INCLUDE_DIRS
        "include"
    REQUIRES
        ${DISPLAY_REQUIRES}
)

# Manual include path workaround for st7789 managed component
if(CONFIG_DISPLAY_TYPE_ST7789_SPI)
    target_include_directories(__idf_display PRIVATE 
        "${CMAKE_SOURCE_DIR}/managed_components/nopnop2002__st7789")
    target_link_libraries(__idf_display PRIVATE __idf_nopnop2002__st7789)
endif()

