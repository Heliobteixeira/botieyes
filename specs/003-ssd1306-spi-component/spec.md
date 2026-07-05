# Feature Specification: SSD1306 Display Driver Migration to nopnop2002 Component

**Feature Branch**: `003-ssd1306-spi-component`

**Created**: 2026-06-13

**Status**: Draft

**Summary**: Migrate SSD1306 hardware communication from custom I2C driver to nopnop2002/esp-idf-ssd1306 component, supporting both I2C and SPI protocols with build-time configuration.

**Input**: User description: "Migrate SSD1306 display hardware communication to the **nopnop2002/esp-idf-ssd1306** component for both I2C and SPI protocols, replacing the existing custom I2C driver, while keeping the existing **Adafruit_GFX**-based graphic rendering layer unchanged. The component is integrated via `idf_component.yml` and configured at build time through Kconfig/menuconfig, covering interface selection (I2C or SPI), pin assignment for MOSI, SCK, CS, DC, and RST (with sensible defaults for SPI: GPIO11, GPIO12, GPIO10, GPIO9, GPIO8), and uses `SPI2_HOST` with DMA at up to 10 MHz for SPI mode. Any initialization failure must log an error, halt, and set the status LED to red. Runtime protocol switching and other display controllers are out of scope."

## Clarifications

### Session 2026-06-13

- Q: Pin validation strategy - when incorrect pin numbers are entered in menuconfig (e.g., GPIO >= 49 on ESP32-S3), how should the system respond? → A: Accept any pin number; attempt initialization and log warning if pin appears invalid
- Q: Initialization timeout behavior - how long should the system wait for SSD1306 to respond during initialization before timing out? → A: 2 second timeout
- Q: DMA allocation failure recovery - what should happen if DMA allocation fails during SPI bus initialization? → A: Treat as initialization failure: log error, halt, set LED red (DMA is required for performance)
- Q: Status LED conflict handling - how should the system behave if the status LED pin (GPIO38) is already in use by other hardware? → A: Attempt LED initialization; if it fails, log warning but continue (LED is optional, display is primary)

## User Scenarios & Testing *(mandatory)*

### User Story 1 - SPI Hardware Configuration (Priority: P1)

As an embedded developer integrating BotiEyes with an ESP32-S3 board, I want to configure SPI pins for the SSD1306 display through menuconfig, so that I can match my hardware wiring without modifying source code.

**Why this priority**: Configuration is the foundation - without proper pin configuration, the display cannot initialize. This is the first step any developer must complete before the display can work.

**Independent Test**: Can be fully tested by running `idf.py menuconfig`, navigating to BotiEyes configuration, selecting SPI protocol, setting pin numbers (MOSI, SCK, CS, DC, RST), saving, and building the project successfully. Delivers the capability to configure hardware without code changes.

**Acceptance Scenarios**:

1. **Given** a fresh ESP-IDF project with BotiEyes, **When** developer runs `idf.py menuconfig`, **Then** BotiEyes configuration menu is visible with SPI protocol option
2. **Given** SPI protocol is selected in menuconfig, **When** developer views pin configuration options, **Then** MOSI, SCK, CS, DC, and RST pin fields are visible with sensible defaults (GPIO11, GPIO12, GPIO10, GPIO9, GPIO8 respectively)
3. **Given** custom pin assignments entered in menuconfig, **When** developer saves and builds, **Then** build completes successfully without errors
4. **Given** default pin values in menuconfig, **When** developer builds without changes, **Then** build uses default values (MOSI=11, SCK=12, CS=10, DC=9, RST=8, Clock=10MHz)

---

### User Story 2 - SPI Display Initialization and Rendering (Priority: P2)

As a developer, I want the BotiEyes library to initialize the SSD1306 display via SPI and render graphics using Adafruit_GFX, so that my eye animations work on SPI-connected displays.

**Why this priority**: After configuration, actual display functionality is the core value. This story delivers the end-to-end capability from initialization to visible output.

**Independent Test**: Can be fully tested by flashing firmware to ESP32-S3 with a correctly wired SSD1306 SPI display, observing the display initialize, and verifying that text/graphics appear on screen. Delivers visible output and validates the complete graphics pipeline.

**Acceptance Scenarios**:

1. **Given** ESP32-S3 with SSD1306 connected via SPI using configured pins, **When** firmware boots and calls display initialization, **Then** SSD1306 initializes successfully and displays turn on
2. **Given** initialized SSD1306 display, **When** BotiEyes renders an emotion expression using Adafruit_GFX calls, **Then** graphics are visible on the OLED display
3. **Given** SPI communication at 10 MHz with DMA enabled, **When** display.display() is called repeatedly, **Then** screen updates occur smoothly without visible lag or corruption
4. **Given** a working I2C display configuration, **When** developer switches to SPI in menuconfig and rebuilds, **Then** the same BotiEyes rendering code works without modification

---

### User Story 3 - Initialization Error Handling (Priority: P3)

As a developer debugging hardware issues, I want clear visual and logging feedback when display initialization fails, so that I can quickly identify wiring or configuration problems.

**Why this priority**: Error handling improves developer experience but is not required for successful operation. It becomes valuable during development and troubleshooting.

**Independent Test**: Can be fully tested by intentionally creating error conditions (wrong pins, disconnected wires, wrong I2C address for SPI mode) and verifying that: (1) error message appears in serial console, (2) status LED turns red, (3) firmware does not proceed past initialization. Delivers troubleshooting capability.

**Acceptance Scenarios**:

1. **Given** SSD1306 display is disconnected or pins are miswired, **When** firmware attempts display initialization, **Then** error message "SSD1306 SPI initialization failed" is logged to serial console
2. **Given** display initialization fails, **When** error is detected, **Then** status LED (GPIO38) turns red and remains red
3. **Given** display initialization fails, **When** error is detected, **Then** firmware halts execution and does not attempt to render graphics
4. **Given** successful display initialization, **When** firmware proceeds to render, **Then** status LED is not red and normal operation continues

---

### Edge Cases

- System accepts any pin number in menuconfig; if pins are out of valid range (e.g., GPIO >= 49 on ESP32-S3) or cause conflicts, initialization will fail and log a warning indicating invalid pin configuration
- System waits up to 2 seconds for SSD1306 to respond during initialization; if timeout occurs, system logs error, halts, and sets status LED to red
- DMA allocation failure during SPI bus initialization is treated as a critical error: system logs error message, halts execution, and sets status LED to red (DMA is required for performance targets)
- If status LED pin (GPIO38) is already in use or conflicts with other hardware, system attempts LED initialization but continues on failure with a warning (display functionality is primary, LED is optional diagnostic)

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: System MUST integrate nopnop2002/esp-idf-ssd1306 component via `idf_component.yml` for both I2C and SPI hardware communication, replacing the existing custom I2C driver (esp_ssd1306.cpp/h)
- **FR-002**: System MUST preserve existing Adafruit_GFX-based rendering layer without modification to graphics code
- **FR-003**: System MUST provide Kconfig/menuconfig options for SPI interface selection
- **FR-004**: System MUST provide Kconfig/menuconfig fields for MOSI, SCK, CS, DC, and RST pin assignment with defaults (GPIO11, GPIO12, GPIO10, GPIO9, GPIO8)
- **FR-005**: System MUST configure SPI communication using SPI2_HOST with DMA enabled at up to 10 MHz clock speed
- **FR-006**: System MUST use SPI clock speed of 10 MHz as the default, configurable via menuconfig
- **FR-007**: System MUST log an error message to serial console when display initialization fails
- **FR-008**: System MUST halt firmware execution when display initialization fails
- **FR-009**: System MUST set status LED (GPIO38) to red when display initialization fails
- **FR-010**: System MUST support ESP32-S3 platform as the primary target
- **FR-011**: System MUST perform all display configuration at build time via Kconfig (no runtime protocol switching)
- **FR-012**: System MUST limit scope to SSD1306 controller only (other display controllers out of scope)
- **FR-013**: System MUST accept any pin number value in menuconfig; invalid pins will cause initialization failure with a warning message indicating the pin configuration issue
- **FR-014**: System MUST timeout initialization after 2 seconds if SSD1306 does not respond, then trigger error handling (log error, halt, set LED red)
- **FR-015**: System MUST treat DMA allocation failure as an initialization failure, triggering error handling (log error, halt, set LED red) since DMA is required for performance targets
- **FR-016**: System MUST attempt to initialize status LED on GPIO38; if LED initialization fails due to pin conflict, system logs a warning but continues normal operation (display is primary, LED is optional)

### Key Entities

- **SSD1306 Display**: External OLED display controller connected via 4-wire SPI interface, requiring MOSI (data), SCK (clock), CS (chip select), DC (data/command), and RST (reset) signals
- **SPI Configuration**: Build-time settings captured in Kconfig/sdkconfig, including pin assignments and clock speed
- **Display Buffer**: Internal framebuffer managed by nopnop2002/esp-idf-ssd1306 component for SPI transfer, compatible with Adafruit_GFX drawing operations
- **Status LED**: Visual indicator (GPIO38) for initialization success/failure

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Developer can configure SPI pins via menuconfig in under 2 minutes without referring to source code
- **SC-002**: BotiEyes library successfully initializes SSD1306 display over SPI on first attempt when hardware is correctly wired
- **SC-003**: Display refresh rate maintains 25 FPS (40ms frame time) for smooth eye animations when using 10 MHz SPI with DMA
- **SC-004**: Initialization failure is detected and reported within 500ms of firmware boot
- **SC-005**: Zero code changes required in existing BotiEyes rendering logic when switching from I2C to SPI configuration
- **SC-006**: Build time increases by less than 10 seconds when nopnop2002/esp-idf-ssd1306 component is added

## Assumptions

- Hardware is ESP32-S3 with sufficient GPIO pins available for 5 SPI signals (MOSI, SCK, CS, DC, RST)
- Developer has physical access to serial console for viewing error messages
- Status LED hardware is connected to GPIO38 as per existing project convention
- SPI2_HOST is available and not already in use by other peripherals in the developer's hardware design
- Display is a standard SSD1306 128x64 OLED compatible with the nopnop2002/esp-idf-ssd1306 component
- Developer is using ESP-IDF v5.0 or later with idf_component.yml support
- I2C protocol support will remain available as an alternative configuration option, but will use the nopnop2002/esp-idf-ssd1306 component instead of the custom driver
- Adafruit_GFX library is already integrated and functional in the project
- Default SPI pin assignments (MOSI=11, SCK=12, CS=10, DC=9, RST=8) are based on the user's ESP32-S3 hardware layout documented in `esp-idf/ssd1306_esp32s3.md`
