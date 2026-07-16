# Tasks: ST7789 Display Driver Support

**Input**: Design documents from `/specs/005-st7789-display-support/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/, quickstart.md

**Target**: TTGO T-Display (240x135 ST7789) with backward compatibility for SSD1306

**Organization**: Tasks are grouped by user story to enable independent implementation and testing of each story.

---

## Phase 1: Setup (Project Initialization)

**Purpose**: Add ST7789 managed component dependency

- [X] T001 Add nopnop2002/st7789 to esp-idf/main/idf_component.yml as managed component dependency

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Build-time configuration and component dependencies that all user stories need

- [X] T002 Add DISPLAY_TYPE choice enum to esp-idf/components/hal_board/Kconfig.projbuild (SSD1306_I2C, SSD1306_SPI, ST7789_SPI)
- [X] T003 Create esp-idf/components/st7789_config/Kconfig.projbuild for ST7789 GPIO configuration (MOSI=19, SCLK=18, CS=5, DC=16, RST=23, BL=4 defaults)
- [X] T004 Add display dimensions config to st7789_config/Kconfig.projbuild (width=240, height=135, offsetx=40, offsety=52 defaults)
- [X] T005 Add SPI configuration options to st7789_config/Kconfig.projbuild (host: SPI2_HOST, clock: 40MHz, frame buffer: optional)
- [X] T006 Update esp-idf/components/display/CMakeLists.txt to conditionally REQUIRE nopnop2002__st7789 when CONFIG_DISPLAY_TYPE_ST7789_SPI

---

## Phase 3: User Story 1 - Use ST7789 Display on Hardware (P1)

**Goal**: Hardware developers can build and flash firmware configured for ST7789 displays

**Independent Test**: Flash to TTGO T-Display, verify eyes render on color display

- [X] T007 [US1] Create ESP_ST7789 class header in esp-idf/components/display/include/esp_st7789.h (inherits Adafruit_GFX)
- [X] T008 [US1] Implement ESP_ST7789 constructor in esp-idf/components/display/src/esp_st7789.cpp (width, height parameters)
- [X] T009 [US1] Implement ESP_ST7789::beginSpi() method wrapping spi_master_init() and lcdInit() from nopnop2002 st7789
- [X] T010 [P] [US1] Implement ESP_ST7789::drawPixel() method with frame buffer support (write to _buffer or call lcdDrawPixel)
- [X] T011 [P] [US1] Implement ESP_ST7789::display() method wrapping lcdDrawMultiPixels() for frame buffer flush
- [X] T012 [P] [US1] Implement ESP_ST7789::clearDisplay() method wrapping lcdFillScreen()
- [X] T013 [P] [US1] Implement ESP_ST7789::backlightOn() and backlightOff() methods controlling BL GPIO
- [X] T014 [US1] Update esp-idf/components/display/src/display_init.cpp to instantiate ESP_ST7789 when CONFIG_DISPLAY_TYPE_ST7789_SPI
- [X] T015 [US1] Add DisplayFlushable interface implementation to ESP_ST7789 for flush() compatibility
- [X] T016 [US1] Update esp-idf/main/main.cpp to handle Adafruit_GFX pointer polymorphism (no driver-specific code)

---

## Phase 4: User Story 2 - Configure Display Type at Build Time (P1)

**Goal**: Developers can select display type via menuconfig

**Independent Test**: Run idf.py menuconfig, switch between display types, verify build selects correct driver

- [X] T017 [US2] Add CONFIG_DISPLAY_TYPE_SSD1306_I2C conditional compilation guards in display_init.cpp for SSD1306 initialization
- [X] T018 [US2] Add CONFIG_DISPLAY_TYPE_ST7789_SPI conditional compilation guards in display_init.cpp for ST7789 initialization
- [X] T019 [P] [US2] Update esp-idf/components/hal_board/src/hal_display.c to read CONFIG_DISPLAY_TYPE and initialize correct driver
- [X] T020 [P] [US2] Add logging to display_init.cpp showing selected display type at boot
- [X] T021 [US2] Verify backward compatibility: build with SSD1306_I2C (default) succeeds without nopnop2002__st7789 linked

---

## Phase 5: User Story 3 - Migrate Configuration from Reference (P2)

**Goal**: Ensure botieyes uses validated configuration from nopnop2002/st7789 defaults

**Independent Test**: Compare Kconfig defaults with nopnop2002 component, flash to TTGO T-Display, verify operation

- [X] T022 [P] [US3] Document nopnop2002/st7789 validated GPIO values in st7789_config/Kconfig.projbuild comments (TTGO T-Display tested pins)
- [X] T023 [P] [US3] Verify SPI clock speed matches nopnop2002 default (20-40 MHz configurable)
- [X] T024 [P] [US3] Verify SPI host selection matches nopnop2002 default (SPI2_HOST)
- [X] T025 [P] [US3] Verify display offset parameters match nopnop2002 TTGO examples (offsetx=40, offsety=52)
- [X] T026 [US3] Update specs/005-st7789-display-support/quickstart.md with nopnop2002 component usage

---

## Phase 6: User Story 4 - Abstract Display Interface (P2)

**Goal**: Display abstraction allows future drivers without BotiEyes code changes

**Independent Test**: Examine class hierarchy, verify both adapters inherit Adafruit_GFX, switch drivers via menuconfig only

- [X] T027 [P] [US4] Document Adafruit_GFX inheritance hierarchy in esp-idf/components/display/README.md
- [X] T028 [P] [US4] Verify ESP_SSD1306 and ESP_ST7789 implement same Adafruit_GFX virtual methods (drawPixel, display, clearDisplay)
- [X] T029 [P] [US4] Add compile-time assertion in display_init.cpp that both adapters inherit from Adafruit_GFX
- [X] T030 [US4] Document adding new display drivers in specs/005-st7789-display-support/contracts/display-driver-interface.md

---

## Phase 7: Polish & Cross-Cutting Concerns

**Purpose**: Error handling, documentation, and quality improvements

- [X] T031 [P] Add error handling to ESP_ST7789::beginSpi() for SPI initialization failures (log and return false)
- [X] T032 [P] Add error handling for frame buffer allocation failure (log and fallback to direct rendering)
- [X] T033 [P] Add validation for GPIO pin conflicts in st7789_config Kconfig.projbuild
- [X] T034 [P] Update .github/copilot-instructions.md with nopnop2002/st7789 managed component
- [X] T035 [P] Add comprehensive code examples to specs/005-st7789-display-support/quickstart.md (8 examples: basic init, BotiEyes integration, color emotions, direct pixel drawing, error handling, performance monitoring, memory monitoring)
- [X] T036 [P] Update project README with ST7789 hardware requirements and TTGO T-Display setup
- [ ] T037 Verify display initialization <2 seconds on TTGO T-Display hardware
- [ ] T038 Verify animation rendering ≥10 FPS on TTGO T-Display hardware
- [ ] T039 Test emotion state changes render smoothly without artifacts
- [ ] T040 Test display persists through WiFi initialization (no crashes or visual glitches)

---

## Dependencies (User Story Completion Order)

**Foundational Phase → All Stories** (Phase 2 blocks everything)

**User Story Dependencies**:
- US1 and US2 can proceed in parallel after Phase 2
- US3 depends on US1 (validates US1 implementation matches reference)
- US4 depends on US1 and US2 (verifies abstraction across both stories)

**Parallel Execution Opportunities**:
- Phase 2: T006-T010 (different Kconfig files)
- US1: T014-T017 (different methods, no shared state)
- US2: T023-T024 (different files)
- US3: T026-T029 (documentation tasks)
- US4: T031-T033 (documentation and verification)
- Polish: T035-T040 (different files)

```
Phase 1 (Setup)
   ↓
Phase 2 (Foundational) ← Must complete before user stories
   ↓
   ├──▶ US1 (T011-T020) ← Hardware display support
   │
   ├──▶ US2 (T021-T025) ← Menuconfig integration
   │         ↓
   ├──▶ US3 (T026-T030) ← Depends on US1
   │         ↓
   └──▶ US4 (T031-T034) ← Depends on US1 + US2
             ↓
   Phase 7 (Polish) ← Integration testing
```

---

## Implementation Strategy

### MVP Scope (Minimum Viable Product)
**Recommended MVP**: User Story 1 + User Story 2 only
- Provides ST7789 hardware support (US1)
- Enables menuconfig selection (US2)
- Maintains backward compatibility
- **Deliverable**: Flash firmware to TTGO T-Display, see animated eyes in color

### Incremental Delivery
1. **Sprint 1**: Phase 1 + Phase 2 (Setup + Foundational)
2. **Sprint 2**: US1 (Hardware support) → Test on TTGO T-Display
3. **Sprint 3**: US2 (Menuconfig) → Test switching displays
4. **Sprint 4**: US3 + US4 (Configuration validation + Abstraction docs)
5. **Sprint 5**: Phase 7 (Polish + Integration testing)

### Task Estimation
- **Phase 1 (Setup)**: 15 minutes (add managed component)
- **Phase 2 (Foundational)**: 2-3 hours (Kconfig configuration)
- **US1 (Hardware)**: 6-8 hours (ESP_ST7789 adapter implementation)
- **US2 (Menuconfig)**: 2-3 hours (conditional compilation)
- **US3 (Config validation)**: 1-2 hours (documentation comparison)
- **US4 (Abstraction)**: 1-2 hours (documentation and verification)
- **Phase 7 (Polish)**: 3-4 hours (error handling + hardware testing)
- **Total**: ~17-24 hours

---

## Completion Report Summary

**Total Tasks**: 40
- Setup: 1 task (T001)
- Foundational: 5 tasks (T002-T006)
- US1 (P1): 10 tasks (T007-T016) - **MVP Critical**
- US2 (P1): 5 tasks (T017-T021) - **MVP Critical**
- US3 (P2): 5 tasks (T022-T026)
- US4 (P2): 4 tasks (T027-T030)
- Polish: 10 tasks (T031-T040)

**Parallel Opportunities**: 22 tasks marked [P]

**Independent Test Criteria**:
- US1: Flash to TTGO T-Display, verify color eyes render at ≥10 FPS
- US2: Switch display types via menuconfig, verify correct driver builds
- US3: Compare with reference config, verify identical behavior on same hardware
- US4: Verify both adapters inherit Adafruit_GFX, no BotiEyes code changes needed

**MVP Scope**: Phase 1 + Phase 2 + US1 + US2 = 22 tasks

**Hardware Requirements**:
- TTGO T-Display (ESP32-PICO-D4 + 240x135 ST7789)
- USB cable for programming
- ESP-IDF v6.0+ installed and activated

**Next Step**: Begin with Phase 1 (T001) - add nopnop2002/st7789 to idf_component.yml
