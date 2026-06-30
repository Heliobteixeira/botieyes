# Display Component - Temporary SSD1306 Adapter
#
# This component provides display initialization and SSD1306 adapter classes
# as a temporary bridge to the external nopnop2002/ssd1306 library.
#
# TODO: Eventually this will be replaced by proper hal_board/hal_display_spi.c
# implementation when the HAL layer is completed.

idf_component_register(
    SRCS
        "src/display_init.cpp"
        "src/esp_ssd1306.cpp"
    INCLUDE_DIRS
        "include"
    PRIV_REQUIRES
        botieyes
        adafruit_gfx
        ssd1306
)
