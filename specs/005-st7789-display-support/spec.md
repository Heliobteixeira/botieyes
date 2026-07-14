# Feature Specification: ST7789 Display Driver Support

**Feature Branch**: `005-st7789-display-support`

**Created**: 2026-07-06

**Status**: Draft

**Input**: User description: "Check the repo below current implementation of a new display driver st7789 and adapt the current project to support it (bring also the configured values as the repo has been successfully tested on the target board): https://github.com/nopnop2002/esp-idf-st7789 (community component, same author as existing SSD1306 driver)"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Use ST7789 Display on Hardware (Priority: P1)

Hardware developers can build and flash the botieyes firmware configured for ST7789 color displays (default 240x135 for TTGO T-Display, also supports 240x240 and other resolutions) on ESP32 devices with ST7789-based TFT screens, allowing the animated eyes to render on color displays instead of monochrome OLED.

**Why this priority**: Enables botieyes to work on widely available color TFT displays, expanding hardware compatibility. ST7789 is a popular, low-cost color display controller used in many ESP32 development kits.

**Independent Test**: Connect an ESP32 board with ST7789 display, configure the project for ST7789 via menuconfig, build and flash. The animated eyes should render correctly on the color display with proper colors and refresh rates.

**Acceptance Scenarios**:

1. **Given** an ESP32 board with ST7789 240x135 SPI display connected (e.g., TTGO T-Display), **When** firmware is built with ST7789 configuration and flashed, **Then** the display initializes successfully and shows the animated eyes in color
2. **Given** the firmware is running on ST7789 display, **When** emotion state changes, **Then** the eye animation updates smoothly without visual artifacts
3. **Given** ST7789 display configuration in menuconfig, **When** building the project, **Then** the build system selects the ST7789 driver component and excludes SSD1306

---

### User Story 2 - Configure Display Type at Build Time (Priority: P1)

Developers can select between SSD1306 (I2C/SPI monochrome) and ST7789 (SPI color) display types through menuconfig, allowing the same codebase to target different hardware configurations without code changes.

**Why this priority**: Essential for maintaining backward compatibility with existing SSD1306 hardware while adding ST7789 support. Build-time configuration is the standard ESP-IDF pattern.

**Independent Test**: Run `idf.py menuconfig`, navigate to "Display Configuration", select display type (SSD1306 or ST7789), configure GPIO pins, build successfully. The correct driver component is included based on selection.

**Acceptance Scenarios**:

1. **Given** a fresh project checkout, **When** developer runs `idf.py menuconfig` and navigates to "Display Configuration", **Then** they see options to select display type (SSD1306_I2C, SSD1306_SPI, ST7789_SPI)
2. **Given** ST7789 is selected in menuconfig, **When** configuring GPIO pins, **Then** the menu shows ST7789-specific pin options (MOSI, SCLK, CS, DC, RESET, BL) with sensible defaults
3. **Given** display type and pins are configured, **When** running `idf.py build`, **Then** only the selected display driver component is compiled and linked

---

### User Story 3 - Use Validated Configuration from nopnop2002 Component (Priority: P2)

Developers can use the tested ST7789 configuration values from the nopnop2002/st7789 community component (same author as existing SSD1306 driver) to ensure the botieyes implementation uses proven GPIO pins, SPI settings, and display parameters that work on common hardware like TTGO T-Display.

**Why this priority**: Reduces risk of integration issues by using validated configuration from a widely-used community component. Important for first successful deployment but not blocking for overall ST7789 support.

**Independent Test**: Compare Kconfig defaults in botieyes with nopnop2002 component examples. Flash firmware to TTGO T-Display. Verify display works as expected.

**Acceptance Scenarios**:

1. **Given** the nopnop2002/st7789 component has tested GPIO pin assignments for TTGO T-Display, **When** ST7789 configuration is added to botieyes, **Then** the default GPIO values in Kconfig match nopnop2002 TTGO examples (MOSI=19, SCLK=18, CS=5, DC=16, RST=23, BL=4)
2. **Given** the nopnop2002 component uses specific SPI host and clock speed, **When** integrating ST7789 driver, **Then** the botieyes configuration uses the same SPI host (SPI2_HOST) and compatible clock speed settings (20-80 MHz configurable)
3. **Given** the nopnop2002 component supports multiple display resolutions with offset parameters, **When** configuring ST7789 in botieyes, **Then** these parameters are available in menuconfig with TTGO T-Display values as defaults (240x135, offsetx=40, offsety=52)

---

### User Story 4 - Abstract Display Interface (Priority: P2)

The display abstraction layer allows the botieyes rendering logic to work with any display driver without modification, enabling future display types to be added by implementing a common interface.

**Why this priority**: Architectural quality improvement that prevents vendor lock-in and simplifies future display additions. Not critical for ST7789 MVP but important for long-term maintainability.

**Independent Test**: Examine the display interface definition. Verify that both SSD1306 and ST7789 implementations satisfy the same interface. Switch between display types by changing menuconfig without modifying application code.

**Acceptance Scenarios**:

1. **Given** the display component defines a common interface (e.g., `display_init()`, `display_draw_pixel()`, `display_flush()`), **When** examining SSD1306 and ST7789 implementations, **Then** both implement the same function signatures
2. **Given** the botieyes application uses the display abstraction, **When** switching display type via menuconfig, **Then** no application code changes are required
3. **Given** a developer wants to add a new display driver, **When** they implement the common interface, **Then** the new driver integrates without modifying existing botieyes code

---

### Edge Cases

- What happens when ST7789 display is not physically connected but firmware is configured for ST7789? (System should detect initialization failure and log error without crashing)
- How does the system handle invalid GPIO pin assignments in menuconfig? (Build system validates GPIO ranges; runtime initialization returns error code)
- What happens if SPI bus is already in use by another component? (Initialization should fail gracefully with descriptive error message)
- How does the system behave when display resolution exceeds frame buffer memory? (Frame buffer allocation fails; fallback to direct rendering or reduced resolution)

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST support ST7789 SPI color display driver as an additional display option alongside existing SSD1306 support
- **FR-002**: System MUST provide Kconfig menu options to select display type (SSD1306_I2C, SSD1306_SPI, ST7789_SPI) at build time
- **FR-003**: System MUST allow configuration of ST7789-specific parameters through menuconfig: screen width, screen height, X offset, Y offset, MOSI GPIO, SCLK GPIO, CS GPIO, DC GPIO, RESET GPIO, Backlight GPIO, SPI host selection (CS and Backlight GPIO support -1 to disable)
- **FR-004**: System MUST use the nopnop2002/st7789 managed component from the ESP Component Registry (same author as existing nopnop2002 SSD1306 driver for API consistency)
- **FR-005**: System MUST provide default GPIO pin values in Kconfig that match the tested TTGO T-Display configuration from nopnop2002 component examples
- **FR-006**: System MUST maintain backward compatibility with existing SSD1306 display configurations (both I2C and SPI modes)
- **FR-007**: System MUST initialize the selected display driver during boot and provide error handling for initialization failures
- **FR-008**: Display component MUST provide a common abstraction interface that works for both SSD1306 and ST7789 displays
- **FR-009**: System MUST support RGB565 color format for ST7789 displays
- **FR-010**: System MUST support the existing BotiEyes rendering logic to work unchanged with both SSD1306 (monochrome) and ST7789 (color) displays
- **FR-011**: System MUST integrate ST7789 driver with the existing HAL board architecture defined in feature 004

### Key Entities

- **DisplayDriver**: Abstract interface for all display types (SSD1306, ST7789, future drivers); provides init, draw_pixel, flush, clear operations
- **ST7789Component**: nopnop2002/st7789 managed component from ESP Component Registry containing ST7789 driver code (st7789.c, st7789.h, fontx files)
- **DisplayConfig**: Configuration structure containing display type, resolution, GPIO pins, SPI settings selected via menuconfig
- **ColorFormat**: Representation for pixel colors (monochrome bitmap for SSD1306, RGB565 for ST7789)

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Botieyes firmware successfully builds with ST7789 display type selected in menuconfig without compilation errors
- **SC-002**: Firmware flashed to ESP32 with ST7789 display initializes the display within 2 seconds of boot
- **SC-003**: Animated eyes render on ST7789 240x135 display with smooth animation (minimum 10 FPS visible frame rate)
- **SC-004**: Developers can switch between SSD1306 and ST7789 display configurations by changing menuconfig settings alone (zero code changes required)
- **SC-005**: ST7789 configuration uses the same GPIO pin defaults as the nopnop2002 TTGO T-Display examples that have been successfully tested on target hardware
- **SC-006**: Display abstraction allows future display drivers to be added by implementing common interface without modifying application code

## Assumptions

- Target hardware has an ESP32 board connected to an ST7789-based TFT display via SPI interface
- The nopnop2002/st7789 component (https://github.com/nopnop2002/esp-idf-st7789) has been tested on TTGO T-Display and other common ST7789 hardware
- Display resolution is 240x135 pixels for TTGO T-Display (configurable for other sizes like 240x240, 240x320)
- Project uses ESP-IDF v5.0 or later (as required by the nopnop2002/st7789 component)
- The nopnop2002/st7789 managed component is available via ESP Component Registry
- The ST7789 driver will be integrated as a managed component, automatically fetched during build
- Color rendering will use RGB565 format (16-bit color depth) as used in the nopnop2002/st7789 component
- SPI communication will use the same host (SPI2_HOST/SPI3_HOST) and clock speed as the nopnop2002 component
- The existing BotiEyes rendering logic will be adapted to work with both monochrome (SSD1306) and color (ST7789) displays through an abstraction layer
- Frame buffer usage will be configurable via menuconfig as in the nopnop2002 component (65KB for 240x135 is reasonable for ESP32, 12.5% of SRAM)
- The HAL layer architecture from feature 004 will be extended to support multiple display drivers
