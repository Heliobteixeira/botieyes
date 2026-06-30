# Hardware Abstraction Layer (HAL) Board Component
# Abstracts board-specific hardware (display interface, LED type, pin mappings)

# Component Dependencies (T112):
# - driver: Generic driver APIs (GPIO, I2C, SPI)
# - led_strip: WS2812 RGB LED driver (for ESP32-S3 variant)
# Private dependencies:
# - esp_driver_gpio: GPIO initialization and control
# - esp_driver_i2c: I2C bus initialization (TTGO LoRa32 display)
# - esp_driver_spi: SPI bus initialization (ESP32-S3 display)

set(SRCS
    "src/hal_board.c"
    "src/hal_display.c"
    "src/hal_led.cpp"
)

# Conditional compilation based on Kconfig board selection
if(CONFIG_BOTIEYES_BOARD_TTGO_LORA32)
    list(APPEND SRCS "src/boards/ttgo_lora32.c")
endif()

if(CONFIG_BOTIEYES_BOARD_ESP32S3_SPI)
    list(APPEND SRCS "src/boards/esp32s3_spi.c")
endif()

# Conditional display driver based on protocol
if(CONFIG_BOTIEYES_OLED_PROTOCOL_I2C)
    list(APPEND SRCS "src/hal_display_i2c.c")
endif()

if(CONFIG_BOTIEYES_OLED_PROTOCOL_SPI)
    list(APPEND SRCS "src/hal_display_spi.c")
endif()

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        driver
        led_strip
    PRIV_REQUIRES
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_spi
)
