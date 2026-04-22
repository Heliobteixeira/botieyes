# Tasks: Emotion-Driven Bot Eyes Library

**Feature Branch**: `001-emotion-driven-eyes`  
**Date**: 2026-04-17  
**Input**: Design documents from `/specs/001-emotion-driven-eyes/`

## Format: `[ID] [P?] [Story] Description`

- **[P]**: Can run in parallel (different files, no dependencies)
- **[Story]**: Which user story this task belongs to (e.g., US1, US2)
- Include exact file paths in descriptions

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic Arduino library structure

- [X] T001 Create Arduino library directory structure: BotiEyes/src/, BotiEyes/examples/, BotiEyes/ (root files)
- [X] T002 Create library.properties with metadata: name=BotiEyes, version=1.0.0, author, sentence, dependencies=Adafruit GFX Library
- [X] T003 [P] Create keywords.txt with Arduino IDE syntax highlighting for API methods
- [X] T004 [P] Create README.md with library overview, installation instructions, basic usage example
- [X] T005 [P] Create emulator/ directory structure: emulator/botieyes_emulator.py, emulator/emotion_mapper.py, emulator/eye_renderer.py, emulator/ui_controls.py
- [X] T006 [P] Create tests/ directory structure: tests/native/, tests/desktop/, tests/embedded/, tests/golden/
- [X] T007 Create .gitignore for Arduino build artifacts, Python __pycache__, PlatformIO .pio/

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core data structures and utilities that ALL user stories depend on

**⚠️ CRITICAL**: No user story work can begin until this phase is complete

- [X] T008 [P] Create DisplayConfig.h with enums (DisplayType, Protocol) and DisplayConfig struct in BotiEyes/src/
- [X] T009 [P] Create ErrorCode enum in BotiEyes/src/BotiEyes.h (OK, INVALID_INPUT, HARDWARE_ERROR, TIMEOUT, DISPLAY_NOT_FOUND, MEMORY_ERROR)
- [X] T010 [P] Create EmotionState.h with struct (currentValence, currentArousal, targetValence, targetArousal, startTime, duration) in BotiEyes/src/
- [X] T011 [P] Implement EmotionState.cpp with clamping (valence [-0.5, 0.5], arousal [0.0, 1.0]) in BotiEyes/src/
- [X] T012 [P] Create EyePositionState.h with struct (horizontal, vertical, targetH, targetV, startTime, duration) in BotiEyes/src/
- [X] T013 [P] Implement EyePositionState.cpp with angle clamping (H: -90 to +90, V: -45 to +45) in BotiEyes/src/
- [X] T014 [P] Create ExpressionParameters.h with struct (eyeWidth, eyeHeight, lidTopCoverage, lidBottomCoverage, yOffset, spacingAdjust, asymmetry) in BotiEyes/src/
- [X] T015 Create Interpolator.h with easeInOutCubic() function declaration in BotiEyes/src/
- [X] T016 Implement Interpolator.cpp with ease-in-out-cubic easing function in BotiEyes/src/
- [X] T017 Create EmotionMapper.h with mapEmotionToExpression(valence, arousal) → ExpressionParameters in BotiEyes/src/
- [X] T018 Implement EmotionMapper.cpp with shape-based mapping formulas (eyeWidth, eyeHeight, lid coverage, yOffset, spacing, asymmetry) per finalized design in BotiEyes/src/
- [X] T019 Implement custom drawEllipse() helper function in BotiEyes/src/ using Bresenham algorithm for eye shapes
- [X] T020 Implement drawEyelidOverlay() helper function in BotiEyes/src/ for top/bottom eyelid shapes using fillTriangle() or arc approximation

**Checkpoint**: Foundation ready - user story implementation can now begin in parallel

---

## Phase 3: User Story 1 - Set Bot Emotion via Valence-Arousal (Priority: P1) 🎯 MVP

**Goal**: Enable developers to control eye expressions using continuous valence-arousal coordinates with smooth transitions

**Independent Test**: Set various (valence, arousal) coordinates, verify smooth eye animation transitions without requiring AI integration or PC emulator

### Implementation for User Story 1

- [X] T021 [P] [US1] Create BotiEyes.h with class declaration, public API methods, private members in BotiEyes/src/
- [X] T022 [US1] Implement initialize(DisplayConfig) in BotiEyes.cpp with display initialization, framebuffer allocation, neutral expression (0.0, 0.5) in BotiEyes/src/
- [X] T023 [US1] Implement validateConfig(DisplayConfig) static method in BotiEyes.cpp with bounds checking (I2C address, SPI pins, dimensions) in BotiEyes/src/
- [X] T024 [US1] Implement setEmotion(valence, arousal, duration) in BotiEyes.cpp with input clamping, target setting, interpolation start in BotiEyes/src/
- [X] T025 [US1] Implement getCurrentEmotion(valence*, arousal*) in BotiEyes.cpp returning current interpolated values in BotiEyes/src/
- [X] T026 [P] [US1] Implement emotion helper happy(intensity) in BotiEyes.cpp mapping to valence +0.35, arousal 0.55 in BotiEyes/src/
- [X] T027 [P] [US1] Implement emotion helper sad(intensity) in BotiEyes.cpp mapping to valence -0.35, arousal 0.35 in BotiEyes/src/
- [X] T028 [P] [US1] Implement emotion helper angry(intensity) in BotiEyes.cpp mapping to valence -0.30, arousal 0.80 in BotiEyes/src/
- [X] T029 [P] [US1] Implement emotion helper calm(intensity) in BotiEyes.cpp mapping to valence 0.0, arousal 0.1 in BotiEyes/src/
- [X] T030 [P] [US1] Implement emotion helper excited(intensity) in BotiEyes.cpp mapping to valence +0.30, arousal 0.90 in BotiEyes/src/
- [X] T031 [P] [US1] Implement emotion helper tired(intensity) in BotiEyes.cpp mapping to valence +0.05, arousal 0.10 in BotiEyes/src/
- [X] T032 [P] [US1] Implement emotion helper surprised(intensity) in BotiEyes.cpp mapping to valence +0.15, arousal 0.85 in BotiEyes/src/
- [X] T033 [P] [US1] Implement emotion helper anxious(intensity) in BotiEyes.cpp mapping to valence -0.20, arousal 0.75 in BotiEyes/src/
- [X] T034 [P] [US1] Implement emotion helper content(intensity) in BotiEyes.cpp mapping to valence +0.25, arousal 0.40 in BotiEyes/src/
- [X] T035 [P] [US1] Implement emotion helper curious(intensity) in BotiEyes.cpp mapping to valence +0.15, arousal 0.60 in BotiEyes/src/
- [X] T036 [P] [US1] Implement emotion helper thinking(intensity) in BotiEyes.cpp mapping to valence 0.0, arousal 0.45, asymmetry -0.20 in BotiEyes/src/
- [X] T037 [P] [US1] Implement emotion helper confused(intensity) in BotiEyes.cpp mapping to valence -0.15, arousal 0.55, asymmetry -0.30 in BotiEyes/src/
- [X] T038 [US1] Implement update() method in BotiEyes.cpp with emotion interpolation, expression mapping, rendering in BotiEyes/src/
- [X] T039 [US1] Implement renderEyes() private method in BotiEyes.cpp drawing base ellipse + eyelid overlays per ExpressionParameters in BotiEyes/src/
- [X] T040 [US1] Create BasicEmotion.ino example in BotiEyes/examples/BasicEmotion/ demonstrating initialize(), emotion helpers, update() loop

**Checkpoint**: User Story 1 complete - parametric emotion control fully functional and testable independently

---

## Phase 4: User Story 4 - Coupled Eye Position Control (Priority: P4)

**Goal**: Enable developers to control coupled 2D eye gaze direction (both eyes move together) with smooth transitions

**Design Note**: v1 uses coupled control for memory efficiency; independent per-eye control deferred to v2

**Independent Test**: Command eye gaze positions in H/V axes, verify smooth coupled movement with both eyes coordinated

### Implementation for User Story 4

- [X] T041 [US4] Implement setEyePosition(h, v, duration) in BotiEyes.cpp with angle clamping, target setting, interpolation start in BotiEyes/src/
- [X] T042 [US4] Implement getEyePosition(h*, v*) in BotiEyes.cpp returning current interpolated angles in BotiEyes/src/
- [X] T043 [P] [US4] Implement lookLeft() in BotiEyes.cpp calling setEyePosition(-45, 0) in BotiEyes/src/
- [X] T044 [P] [US4] Implement lookRight() in BotiEyes.cpp calling setEyePosition(+45, 0) in BotiEyes/src/
- [X] T045 [P] [US4] Implement lookUp() in BotiEyes.cpp calling setEyePosition(0, +30) in BotiEyes/src/
- [X] T046 [P] [US4] Implement lookDown() in BotiEyes.cpp calling setEyePosition(0, -30) in BotiEyes/src/
- [X] T047 [P] [US4] Implement neutral() in BotiEyes.cpp calling setEyePosition(0, 0) in BotiEyes/src/
- [X] T048 [US4] Update renderEyes() in BotiEyes.cpp to apply H/V angle transformations to eye positions in BotiEyes/src/
- [X] T049 [US4] Update update() in BotiEyes.cpp to interpolate eye position alongside emotion in BotiEyes/src/
- [X] T050 [US4] Create EyePosition.ino example in BotiEyes/examples/EyePosition/ demonstrating position control and predefined behaviors

**Checkpoint**: User Story 4 complete - eye position control fully functional, integrates with emotion from US1

---

## Phase 5: User Story 3 - Dynamic Expression Intensity (Priority: P3)

**Goal**: Enable continuous expression intensity variations by supporting intensity parameters on emotion helpers

**Independent Test**: Vary intensity parameter on emotion helpers, observe gradual changes in expression parameters

### Implementation for User Story 3

- [ ] T051 [US3] Update emotion helper methods in BotiEyes.cpp to scale valence/arousal by intensity parameter (0.0-1.0) in BotiEyes/src/
- [ ] T052 [US3] Add intensity clamping (0.0-1.0) in emotion helper methods in BotiEyes.cpp in BotiEyes/src/
- [ ] T053 [US3] Update BasicEmotion.ino example to demonstrate intensity variations (e.g., happy(0.3), happy(0.7), happy(1.0)) in BotiEyes/examples/BasicEmotion/

**Checkpoint**: User Story 3 complete - dynamic intensity variations functional, enhances US1 emotion control

---

## Phase 6: User Story 5 - PC Emulation for Development (Priority: P5)

**Goal**: Provide Python-based PC emulator for rapid iteration without hardware, matching embedded rendering

**Independent Test**: Run emulator, adjust controls, verify animations match expected behavior

### Implementation for User Story 5

- [ ] T054 [P] [US5] Create requirements.txt in emulator/ with pygame dependency
- [ ] T055 [P] [US5] Implement emotion_mapper.py in emulator/ porting C++ EmotionMapper logic (valence-arousal → expression parameters)
- [ ] T056 [US5] Implement eye_renderer.py in emulator/ with Pygame drawing (ellipse, eyelid overlays matching C++ renderEyes())
- [ ] T057 [US5] Implement ui_controls.py in emulator/ with sliders for valence, arousal, H angle, V angle
- [ ] T058 [US5] Implement botieyes_emulator.py in emulator/ with main loop, event handling, display window (128x64 resolution)
- [ ] T059 [US5] Add emotion helper buttons in ui_controls.py for quick emotion selection (happy, sad, angry, etc.)
- [ ] T060 [US5] Add position helper buttons in ui_controls.py for eye position behaviors (lookLeft, lookRight, etc.)

**Checkpoint**: User Story 5 complete - PC emulator fully functional for development without hardware

---

## Phase 7: User Story 2 - AI-Driven Development Feedback (Priority: P2)

**Goal**: Enable AI agents to verify rendering by capturing frames and inspecting expression state via JSON

**Independent Test**: AI captures frame, analyzes output, identifies issues, adjusts code, re-verifies

### Implementation for User Story 2

- [ ] T061 [US2] Implement captureFrame() method in botieyes_emulator.py returning PNG byte array using pygame.image.tostring() in emulator/
- [ ] T062 [US2] Implement getExpressionState() method in botieyes_emulator.py returning JSON with current valence, arousal, expression parameters, eye position in emulator/
- [ ] T063 [US2] Add keyboard shortcut (e.g., 'C') to save current frame to file in botieyes_emulator.py in emulator/
- [ ] T064 [US2] Add display overlay showing current state (valence, arousal, H, V) in botieyes_emulator.py in emulator/

**Checkpoint**: User Story 2 complete - AI can capture frames and inspect state for iterative development

---

## Phase 8: User Story 6 - AI Runtime Integration (Priority: P6)

**Goal**: Enable AI systems to dynamically control bot emotions via serial commands in production

**Independent Test**: Send emotion coordinates via serial, verify eyes update accordingly without jitter

### Implementation for User Story 6

- [ ] T065 [US6] Create SerialControl.ino example skeleton in BotiEyes/examples/SerialControl/
- [ ] T066 [US6] Implement serial command parser in SerialControl.ino supporting "EMO:v,a" format
- [ ] T067 [US6] Implement serial command parser in SerialControl.ino supporting "POS:h,v" format
- [ ] T068 [P] [US6] Add emotion helper commands to SerialControl.ino ("HAPPY", "SAD", "ANGRY", "CALM", "EXCITED", "TIRED", "SURPRISED", "ANXIOUS", "CONTENT", "CURIOUS", "THINKING", "CONFUSED")
- [ ] T069 [P] [US6] Add position helper commands to SerialControl.ino ("LEFT", "RIGHT", "UP", "DOWN", "NEUTRAL")
- [ ] T070 [P] [US6] Add animation commands to SerialControl.ino ("BLINK", "WINK:L", "WINK:R")
- [ ] T071 [US6] Add error responses in SerialControl.ino for invalid commands with clear error messages
- [ ] T072 [US6] Add command queue in SerialControl.ino to handle rapid updates (>10 Hz) without jitter

**Checkpoint**: User Story 6 complete - AI can control emotions via serial protocol for production use

---

## Phase 9: Additional Features (Cross-Story Enhancements)

**Purpose**: Built-in animations and idle behaviors that enhance multiple user stories

**Note**: Conversational AI emotion helpers (thinking, confused) implemented in Phase 3 with core emotion helpers

- [ ] T073 [P] Create AnimationState enum in BotiEyes.h (NONE, BLINKING, WINKING_LEFT, WINKING_RIGHT) in BotiEyes/src/
- [ ] T074 [P] Create EyeSide enum in BotiEyes.h (LEFT, RIGHT) in BotiEyes/src/
- [ ] T075 Implement blink(duration) in BotiEyes.cpp with animation state machine in BotiEyes/src/
- [ ] T076 Implement wink(eye, duration) in BotiEyes.cpp with per-eye animation state in BotiEyes/src/
- [ ] T077 Update update() in BotiEyes.cpp to process animation state machine alongside interpolation in BotiEyes/src/
- [ ] T078 Implement enableIdleBehavior(enable) in BotiEyes.cpp with periodic micro-blinks and subtle morphing in BotiEyes/src/
- [ ] T079 Add idle behavior timer in BotiEyes.cpp triggering random micro-blinks every 3-5 seconds in BotiEyes/src/

**Checkpoint**: All animation and conversational AI features implemented

---

## Phase 10: Testing (Quality Assurance)

**Purpose**: Comprehensive testing across unit, visual, and hardware levels

- [ ] T081 [P] Setup PlatformIO test environment in platformio.ini with native, desktop, embedded targets
- [ ] T082 [P] Create test_emotion_mapper.cpp in tests/native/ with Tier 1 tests (boundary values, clamping, mapping correctness)
- [ ] T083 [P] Create test_interpolator.cpp in tests/native/ with easing function tests (linear, cubic, boundary conditions)
- [ ] T084 [P] Create test_state_transitions.cpp in tests/native/ with emotion transition tests (setEmotion during interpolation, concurrent updates)
- [ ] T085 [P] Create test_geometry.cpp in tests/native/ with ellipse and eyelid overlay math tests
- [ ] T086 [P] Create test_api_coverage.cpp in tests/native/ with tests for all public API methods (initialize, setEmotion, getCurrentEmotion, helpers, position methods)
- [ ] T087 Create MockDisplay.h in tests/desktop/ implementing Adafruit_GFX interface with virtual canvas
- [ ] T088 Create test_rendering_visual.cpp in tests/desktop/ with golden image checksum tests for all 12 emotions
- [ ] T089 Generate golden image checksums in tests/golden/ for each emotion at 128x64 resolution
- [ ] T090 Create test_smoke.cpp in tests/embedded/ with basic hardware initialization and rendering smoke test
- [ ] T091 Add memory usage validation in platformio.ini checking compiled size ≤1.0KB for Nano target

**Checkpoint**: All automated tests implemented and passing

---

## Phase 11: Polish & Cross-Cutting Concerns

**Purpose**: Documentation, CI/CD, performance validation, and final quality checks

- [ ] T091 [P] Create detailed API documentation in README.md with all public methods, parameters, examples
- [ ] T092 [P] Create QUICKSTART.md in BotiEyes/ with 15-minute getting started guide matching quickstart.md spec
- [ ] T093 [P] Create CONTRIBUTING.md in BotiEyes/ with development setup, testing instructions, PR guidelines
- [ ] T094 [P] Create examples/README.md in BotiEyes/examples/ with overview of all example sketches
- [ ] T095 Create .github/workflows/arduino-compile.yml with CI for all targets (Nano, Mega, ESP32), memory checks
- [ ] T096 Create .github/workflows/emulator-tests.yml with CI for Python emulator tests and visual regression
- [ ] T097 Run performance validation on Arduino Nano hardware verifying ≥28 FPS with actual 18-20ms render + 15ms I2C
- [ ] T098 Run memory profiling on Arduino Nano verifying ≤1.0KB library RAM, ≥1000 bytes user code available
- [ ] T099 Validate all 12 emotions render correctly on real hardware (Nano, Mega, or ESP32) matching emulator output
- [ ] T100 Run quickstart.md validation end-to-end with fresh Arduino IDE setup
- [ ] T101 Create LICENSE file in BotiEyes/ (suggest MIT or Apache 2.0 for open source)
- [ ] T102 Create CHANGELOG.md in BotiEyes/ documenting v1.0.0 features and design decisions

**Checkpoint**: All documentation, CI/CD, and validation complete - ready for release

---

## Dependencies & Execution Order

### Phase Dependencies

- **Setup (Phase 1)**: No dependencies - can start immediately
- **Foundational (Phase 2)**: Depends on Setup completion - BLOCKS all user stories
- **User Stories (Phases 3-8)**: All depend on Foundational phase completion
  - User stories can then proceed in parallel (if staffed)
  - Or sequentially in priority order: US1 (P1) → US4 (P4) → US3 (P3) → US5 (P5) → US2 (P2) → US6 (P6)
- **Additional Features (Phase 9)**: Depends on US1 complete (needs core BotiEyes class)
- **Testing (Phase 10)**: Can start after Foundational phase for unit tests; visual tests need US1 complete
- **Polish (Phase 11)**: Depends on all desired user stories being complete

### User Story Dependencies

- **User Story 1 (P1)** 🎯 MVP: Can start after Foundational (Phase 2) - No dependencies on other stories
- **User Story 4 (P4)**: Can start after US1 complete (needs BotiEyes class and update() loop) - Integrates with US1
- **User Story 3 (P3)**: Can start after US1 complete (modifies emotion helpers) - Enhances US1
- **User Story 5 (P5)**: Can start after Foundational phase (independent emulator) - No dependencies on C++ code
- **User Story 2 (P2)**: Depends on US5 complete (needs emulator structure) - Extends US5
- **User Story 6 (P6)**: Depends on US1 complete (needs core emotion control) - Uses US1 API

### Within Each User Story

**User Story 1 (P1)**:
1. BotiEyes class skeleton (T021)
2. Core methods: initialize, validateConfig, setEmotion, getCurrentEmotion (T022-T025) - sequential
3. Emotion helpers including conversational AI (T026-T037) - ALL parallelizable (12 helpers)
4. update() and renderEyes() (T038-T039) - depends on helpers
5. Example sketch (T040) - depends on update()

**User Story 4 (P4)**:
1. Position methods: setEyePosition, getEyePosition (T041-T042) - sequential
2. Position behaviors (T043-T047) - ALL parallelizable
3. Update renderEyes() and update() (T048-T049) - sequential
4. Example sketch (T050) - last

### Parallel Opportunities

**Setup Phase (1)**:
- T003, T004, T005, T006 can run in parallel (different directories)

**Foundational Phase (2)**:
- T008-T014 (all .h file creation) can run in parallel
- T010-T011 (EmotionState) must be sequential
- T012-T013 (EyePositionState) must be sequential
- T019, T020 (rendering helpers) can run in parallel with data structures

**User Story 1 (P1)**:
- All emotion helper implementations (T026-T037) can run in parallel - 12 independent methods including conversational AI (thinking, confused)

**User Story 4 (P4)**:
- All position behavior implementations (T043-T047) can run in parallel - 5 independent methods

**User Story 5 (P5)**:
- T054, T055 can run in parallel (requirements.txt independent of mapper)

**User Story 6 (P6)**:
- T068, T069, T070 (command types) can run in parallel after parser complete

**Additional Features (Phase 9)**:
- T073, T074 (enums) can run in parallel

**Testing (Phase 10)**:
- All test file creation (T081-T085, T089) can run in parallel - independent test suites

**Polish (Phase 11)**:
- All documentation (T091-T094, T101-T102) can run in parallel
- T095, T096 (CI workflows) can run in parallel

---

## Parallel Execution Examples

### Example 1: Foundational Phase (Maximize Parallelism)

```bash
# Team of 3 developers, after T001-T007 complete:

# Developer 1: Data structures
git checkout -b feature/data-structures
# T008, T010, T012, T014 (create all .h files)
# T011 (implement EmotionState clamping)
# T013 (implement EyePositionState clamping)

# Developer 2: Mapping & interpolation
git checkout -b feature/mapping
# T015-T018 (Interpolator + EmotionMapper)

# Developer 3: Rendering helpers
git checkout -b feature/rendering
# T019 (custom ellipse)
# T020 (eyelid overlays)

# All merge back to feature branch after completion
```

### Example 2: User Story 1 - Emotion Helpers (Maximize Parallelism)

```bash
# After T021-T025 complete (core BotiEyes API), team of 6:

# Developer 1:
# T026 happy()
# T027 sad()

# Developer 2:
# T028 angry()
# T029 calm()

# Developer 3:
# T030 excited()
# T031 tired()

# Developer 4:
# T032 surprised()
# T033 anxious()

# Developer 5:
# T034 content()
# T035 curious()

# Developer 6:
# T036 thinking()
# T037 confused()

# All 12 emotion helpers can be implemented simultaneously
```

### Example 3: Complete MVP (US1) Solo Developer

```bash
# Sequential path to MVP (User Story 1 only):
# Day 1: Setup + Foundational
T001 → T002 → T007 → T008 → T009 → T010 → T011 → T012 → T013 → T014 → T015 → T016 → T017 → T018 → T019 → T020

# Day 2: User Story 1 Core
T021 → T022 → T023 → T024 → T025

# Day 3: User Story 1 Emotion Helpers (batch)
T026-T037 (implement all 12 emotion helpers in sequence)

# Day 4: User Story 1 Rendering & Example
T038 → T039 → T040

# MVP COMPLETE: Basic emotion control with 12 emotions functional
```

---

## Implementation Strategy

**MVP Definition**: User Story 1 (P1) only
- Parametric emotion control via valence-arousal
- 12 emotion helpers including conversational AI (happy, sad, angry, calm, excited, tired, surprised, anxious, content, curious, thinking, confused)
- Smooth transitions with interpolation
- Basic example sketch

**Note on SC-016 (MVP Emulator Priority)**: Success Criterion SC-016 states "MVP emulator demonstrates basic rendering with PNG export functional before Arduino implementation begins". If strict compliance with SC-016 is required, consider implementing **Phase 6 (User Story 5 - PC Emulator)** before or in parallel with **Phase 3 (User Story 1 - Arduino Implementation)** to enable AI visual feedback loop from the start.

**Recommended Order**:
1. **Phase 1-2**: Setup + Foundational (MUST complete first)
2. **Phase 3**: User Story 1 (MVP) - get emotion control working
3. **Phase 6**: User Story 5 (PC emulator) - enable hardware-free iteration
4. **Phase 4**: User Story 4 (Eye position) - add directional gaze
5. **Phase 5**: User Story 3 (Intensity) - enhance emotion control
6. **Phase 7**: User Story 2 (AI feedback) - enable AI-driven development
7. **Phase 8**: User Story 6 (Serial control) - production runtime control
8. **Phase 9**: Additional Features - animations and idle behaviors
9. **Phase 10**: Testing - quality assurance
10. **Phase 11**: Polish - documentation and CI/CD

**Incremental Delivery**:
- After Phase 3: Library usable for basic projects (emotion control only)
- After Phase 6: Developers can iterate without hardware
- After Phase 4: Full eye control (emotion + position)
- After Phase 8: Production-ready for AI robotics

---

## Task Count Summary

- **Setup**: 7 tasks
- **Foundational**: 13 tasks (BLOCKING)
- **User Story 1 (P1)**: 20 tasks 🎯 MVP (includes conversational AI helpers: thinking, confused)
- **User Story 4 (P4)**: 10 tasks
- **User Story 3 (P3)**: 3 tasks
- **User Story 5 (P5)**: 7 tasks
- **User Story 2 (P2)**: 4 tasks
- **User Story 6 (P6)**: 8 tasks
- **Additional Features**: 8 tasks (animations, idle behaviors)
- **Testing**: 11 tasks
- **Polish**: 12 tasks

**Total**: 103 tasks

**Parallel Opportunities**: 37+ tasks marked [P] can run in parallel within their phases (12 emotion helpers, 5 position behaviors, others)

**Suggested MVP Scope**: Phases 1-3 only (40 tasks) delivers core emotion control with all 12 emotion helpers
