# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Conditionally exclude conflicting display drivers
# Only include the driver selected in menuconfig to avoid symbol conflicts
if(CONFIG_DISPLAY_TYPE_ST7789_SPI)
    list(APPEND EXCLUDE_COMPONENTS ssd1306 ssd1306_config)
endif()
if(CONFIG_DISPLAY_TYPE_SSD1306_I2C OR CONFIG_DISPLAY_TYPE_SSD1306_SPI)
    list(APPEND EXCLUDE_COMPONENTS nopnop2002__st7789 st7789_config)
endif()
if(EXCLUDE_COMPONENTS)
    idf_build_set_property(EXCLUDE_COMPONENTS "${EXCLUDE_COMPONENTS}")
endif()

project(botieyes-esp-idf)

# Generate version from git tags (T111, FR-049)
execute_process(
    COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_compile_definitions(PROJECT_VER="${GIT_VERSION}")
