# Feature Specification: Emotion-Driven Bot Eyes Library

**Feature Branch**: `001-emotion-driven-eyes`  
**Created**: 2026-04-16  
**Status**: Draft  
**Input**: User description: "Build a library to draw of bot eyes on oled display. The project should be and improved version of the repo below. Yet instead i want the expressions to be parametric using valence-arousal model so it can be dyanically connected and used by AI. Also I want the expressions to be fully dynamic (for example the happiness can vary from soft to high). Also for convenience we should have the ability of emulating it locally on pc. https://github.com/FluxGarage/RoboEyes"

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Set Bot Emotion via Valence-Arousal (Priority: P1)

A developer integrates the library into their robot project and controls eye expressions by setting valence (positive/negative emotion) and arousal (calm/excited energy) coordinates. The eyes smoothly animate to reflect the emotional state.

**Why this priority**: Core value proposition. Without parametric emotion control, the library doesn't deliver on its primary innovation over RoboEyes.

**Independent Test**: Can be fully tested by setting various (valence, arousal) coordinates and verifying smooth eye animation transitions without requiring AI integration or PC emulator.

**Acceptance Scenarios**:

1. **Given** library initialized with OLED display, **When** developer sets emotion(+0.3, 0.5) for happy-moderate, **Then** eyes display happy expression with moderate openness
2. **Given** eyes in neutral state, **When** emotion changes to (-0.2, 0.7) for anxious, **Then** eyes smoothly transition to narrow, alert expression
3. **Given** eyes in any state, **When** emotion set to (0.0, 0.1) for calm-neutral, **Then** eyes transition to relaxed, half-open neutral expression

---

### User Story 2 - AI-Driven Development Feedback (Priority: P2)

Copilot and AI agents verify rendering implementations by capturing emulator frames and inspecting expression state. Enables self-correcting development where AI sees results and refines code accordingly.

**Why this priority**: Essential for AI-driven workflow. Without visual feedback, AI codes blind and cannot verify aesthetic quality or correctness.

**Independent Test**: AI implements rendering, captures frame, analyzes output, identifies issues, adjusts code, re-verifies.

**Acceptance Scenarios**:

1. **Given** emulator running, **When** AI calls captureFrame(), **Then** returns PNG viewable by multimodal AI
2. **Given** expression rendered, **When** AI queries getExpressionState(), **Then** returns JSON with valence, arousal, pupil size, eyelid openness, eye positions
3. **Given** AI identifies issue, **When** AI adjusts code and re-runs, **Then** AI compares before/after frames to verify improvement

---

### User Story 3 - Dynamic Expression Intensity (Priority: P3)

Expression intensity varies continuously based on emotion coordinates from subtle to extreme variations.

**Why this priority**: Enables nuanced emotional communication essential for AI-driven interactions.

**Independent Test**: Vary valence/arousal independently, observe gradual intensity changes in expression parameters.

**Acceptance Scenarios**:

1. **Given** arousal fixed at 0.5, **When** valence varies -0.4→+0.4, **Then** expression transitions sad→happy smoothly
2. **Given** valence fixed at 0.0, **When** arousal varies 0.1→0.9, **Then** eye openness and pupil dilation increase proportionally

---

### User Story 4 - Independent Eye Position Control (Priority: P4)

Developers control each eye independently in 2D space (horizontal + vertical). Eyes can converge, diverge, look up/down, or execute custom animations.

**Why this priority**: Enables directional gaze and spatial awareness essential for interactive robots.

**Independent Test**: Command individual eye positions in H/V axes, verify independent movement.

**Acceptance Scenarios**:

1. **Given** both eyes neutral, **When** set left(-30°,0°) right(+30°,0°), **Then** eyes converge toward center
2. **Given** eyes neutral, **When** set both(0°,+20°), **Then** eyes look up together
3. **Given** independent positions set, **When** emotion changes, **Then** emotion applies to both eyes preserving relative positions

---

### User Story 5 - PC Emulation for Development (Priority: P5)

Developers iterate on expressions using PC-based emulator without physical hardware. Includes real-time controls and frame capture.

**Why this priority**: Accelerates development and lowers barrier to entry.

**Independent Test**: Run emulator, adjust controls, verify animations match target hardware.

**Acceptance Scenarios**:

1. **Given** emulator launched, **When** developer adjusts valence/arousal sliders, **Then** eyes update in real-time
2. **Given** emulator running, **When** developer exports animation, **Then** same sequence plays identically on OLED hardware

---

### User Story 6 - AI Runtime Integration (Priority: P6)

AI systems dynamically control bot emotions through simple API calls or serial commands.

**Why this priority**: Enables runtime AI control for production robotics.

**Independent Test**: Send emotion coordinates via serial/API, verify eyes update accordingly.

**Acceptance Scenarios**:

1. **Given** library listening on serial, **When** AI sends "EMO:0.3,0.6", **Then** eyes update to happy-excited expression
2. **Given** rapid updates (>10 Hz), **Then** library smoothly interpolates without jitter

---

### Edge Cases

- **Out-of-bounds values**: Clamp valence to [-0.5, 0.5], arousal to [0.0, 1.0], angles to valid ranges
- **Display init failure**: Return error code, allow retry or fallback to emulator
- **Update rate > refresh rate**: Queue updates, interpolate smoothly
- **Animation + emotion conflict**: Animation takes precedence; emotion applies after completion; new animation interrupts current
- **Impossible eye positions**: Allow (useful for cartoon effects)
- **Asymmetric displays**: Adapt proportions, maintain aesthetic balance

---

## Clarifications

### Session 2026-04-17

**Q1: Geometric Primitives per Eye (FR-013)**
- A: **Minimal (5 primitives)**: Outer eye ellipse, pupil circle, upper eyelid arc, lower eyelid arc, highlight dot
- Rationale: Balance aesthetic quality with rendering performance on constrained displays

**Q2: Display Configuration Strategy (FR-004)**
- A: **Manual configuration**: Developer must explicitly specify display type (SSD1306/SH1106), protocol (I2C/SPI), pins, and resolution in initialize(config)
- Rationale: Explicit configuration avoids detection complexity and code overhead

**Q3: Custom Animation Format (FR-012)**
- A: **Built-in animations only**: Library provides predefined sequences (blink, wink, roll) with configurable parameters; no custom animation creation
- Rationale: Simplifies implementation; meets core requirements without custom animation complexity

**Q4: PC Emulator Technology (FR-005)**
- A: **Python + Pygame**: Cross-platform GUI, easy PNG export, simple slider controls
- Rationale: Rapid development, trivial frame capture, Python-native for AI/ML workflows, minimal dependencies

**Q5: Error Handling Strategy (FR-007)**
- A: **Enum error codes**: Methods return ErrorCode enum (OK, INVALID_INPUT, HARDWARE_ERROR, TIMEOUT)
- Rationale: Explicit, type-safe, suitable for embedded; avoids exceptions and global state complexity

**Q6: Valence-Arousal Ranges (FR-001)**
- A: **Valence [-0.5, +0.5], Arousal [0, 1]**: Negative valence for negative emotions, positive for positive, zero for neutral; arousal from calm to excited
- Rationale: Semantically clearer than [0,1] for both; aligns with psychological models (Russell's circumplex); zero-centered valence more intuitive

**Q7: Initialization Default State (FR-007)**
- A: **Neutral expression (0.0, 0.5)**: Eyes display neutral emotion at moderate arousal immediately after successful initialization
- Rationale: Provides immediate visual feedback that initialization succeeded; predictable starting point; confirms display working

**Q8: Memory Allocation Strategy (FR-018)**
- A: **Static allocation only**: All buffers (framebuffer, state, animation queues) allocated at compile-time or during initialize()
- Rationale: Predictable memory footprint; prevents fragmentation; avoids runtime heap exhaustion; embedded best practice

**Q9: Serial Protocol Error Responses (FR-015)**
- A: **Error response string**: Invalid/malformed commands return "ERROR:<code>" (e.g., "ERROR:INVALID_COMMAND", "ERROR:PARSE_FAILED")
- Rationale: Explicit feedback enables AI/developers to detect and debug integration issues quickly

**Q10: Concurrent Emotion and Position Updates (FR-007)**
- A: **Independent and additive**: Emotion controls expression (pupil, eyelids, brow); position controls direction (H/V angles); both apply simultaneously
- Rationale: Maximum flexibility; orthogonal concerns compose naturally (e.g., "happy eyes looking left")

**Q11: Debug Output and Diagnostics (FR-009)**
- A: **No debug output**: Library runs silently; developers use getExpressionState() JSON for state inspection and troubleshooting
- Rationale: Minimal memory/bandwidth overhead; simplifies implementation; JSON state provides sufficient diagnostic capability

**Q12: Transition Duration Configuration (FR-003)**
- A: **Per-call parameter**: setEmotion(valence, arousal, duration_ms) with optional duration parameter (default: 400ms)
- Rationale: Maximum flexibility; different emotions may warrant different timing; context-specific control

**Q13: Update() Call Frequency (FR-016)**
- A: **Must match target FPS**: Developer calls update() at target rate (e.g., every 33ms for 30 FPS); library advances animation state proportionally
- Rationale: Predictable behavior; simple implementation; developer controls rendering loop timing

**Q14: Animation Interruption Behavior (FR-012)**
- A: **Interrupt immediately**: New animation cancels current animation and starts from current state
- Rationale: Responsive to real-time control; prevents animation queue buildup; AI can react immediately to context changes

**Q15: Eye Position Transition Smoothing (FR-010)**
- A: **Smooth with fixed duration**: setEyePosition() always interpolates over fixed 300ms duration using easing functions
- Rationale: Consistent with emotion transitions; maintains aesthetic quality; prevents jarring mechanical movements

**Q16: Emotion Mapping Table Customization (FR-017)**
- A: **Not customizable**: Mapping formulas (pupil_dilation, eyelid_openness, brow_angle) are hardcoded; developers use provided baseline
- Rationale: Aligns with Pragmatic Simplicity; reduces API complexity; baseline formulas work across use cases; v1 scope control

**Q17: Implementation Architecture (FR-004, FR-005)**
- A: **Separate C++ library for Arduino IDE**: Standalone C++ library (not Arduino sketches) compiled by Arduino IDE; minimal dependencies (Adafruit GFX only); MVP approach prioritizes basic rendering + PNG export for AI feedback loop
- Rationale: Reusable across projects; standard Arduino workflow; lean codebase; enables AI-driven iterative development from start

---

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: Library MUST accept valence in range [-0.5, 0.5] (negative=negative emotions, positive=positive emotions) and arousal in range [0.0, 1.0] (calm to excited)
- **FR-002**: Library MUST map valence-arousal coordinates to eye animation parameters (pupil size, eyelid position, eye shape, brow angle)
- **FR-003**: Library MUST support smooth interpolation between emotional states with per-call configurable transition duration (default: 400ms)
- **FR-004**: Library MUST render animated eyes on OLED displays (I2C/SPI) using Adafruit GFX as sole rendering dependency
- **FR-005**: Library MUST provide PC emulator (Python + Pygame) with PNG export as highest priority feature for AI feedback; emulator must be functional in MVP before full Arduino implementation
- **FR-006**: Library MUST support Arduino (Nano, Uno) and ESP32 platforms with appropriate resource constraints
- **FR-007**: Library MUST expose API with ErrorCode returns: initialize(config) [sets default state to (0.0, 0.5)], setEmotion(valence, arousal, duration_ms=400), setEyePosition(leftH, leftV, rightH, rightV) [300ms interpolation], update() [called at target FPS], getCurrentEmotion(), getEyePosition(); emotion and position updates are independent and additive
- **FR-008**: Emulator MUST provide captureFrame() returning PNG image viewable by multimodal AI
- **FR-009**: Library MUST provide getExpressionState() returning JSON with current emotion coordinates and derived visual parameters
- **FR-010**: Library MUST support independent 2D eye position control per eye (horizontal -90° to +90°, vertical -45° to +45°) with smooth interpolation over fixed 300ms duration
- **FR-011**: Library MUST provide predefined behaviors: converge(), diverge(), lookLeft(), lookRight(), lookUp(), lookDown(), neutral()
- **FR-012**: Library MUST support built-in animation sequences: blink(duration), wink(eye, duration), roll(direction, speed) with configurable parameters; new animations interrupt current animation immediately
- **FR-013**: Library MUST render eyes using 5 geometric primitives per eye: outer ellipse, pupil circle, upper eyelid arc, lower eyelid arc, highlight dot
- **FR-014**: Library MUST use smooth easing functions (ease-in-out-cubic baseline; context-aware variants for emotion types)
- **FR-015**: Library MUST provide serial interface ("EMO:v,a", "POS:lh,lv,rh,rv", "QUERY" returns JSON state); invalid commands return "ERROR:<code>" strings
- **FR-016**: Library MUST maintain target FPS: ESP32 ≥30 FPS, Arduino Mega ≥20 FPS, PC emulator ≥60 FPS; developer MUST call update() at target frame rate for proper animation timing
- **FR-017**: Library MUST use explicit emotion mapping (see Emotion Mapping Table) with 10+ defined emotion anchors; mapping formulas are hardcoded and not customizable in v1
- **FR-018**: Library MUST implement platform profiles with appropriate resource constraints per platform; use static memory allocation only (no dynamic allocation)
- **FR-019**: Library MUST require explicit display configuration in initialize(): display type (SSD1306/SH1106), protocol (I2C/SPI), I2C address or SPI pins, resolution (width, height)
- **FR-020**: PC emulator MUST be implemented in Python using Pygame for graphics, enabling rapid development and easy integration with AI workflows
- **FR-021**: Library MUST define ErrorCode enum with values: OK, INVALID_INPUT, HARDWARE_ERROR, TIMEOUT, DISPLAY_NOT_FOUND, MEMORY_ERROR
- **FR-022**: Library MUST initialize eyes to neutral expression (0.0, 0.5) immediately after successful initialize() call
- **FR-023**: Library MUST apply emotion (expression parameters) and position (H/V angles) independently and simultaneously; changes to one do not reset the other
- **FR-024**: Library MUST be structured as standalone C++ library compilable by Arduino IDE (not Arduino sketch); header (.h) and implementation (.cpp) files following Arduino library conventions
- **FR-025**: Library MUST minimize dependencies: Adafruit GFX only for Arduino/ESP32; Python + Pygame only for emulator; no other external libraries

### Emotion Mapping Table

Explicit valence-arousal coordinates for 10+ recognizable emotions:

| Emotion | Valence | Arousal | Pupil | Eyelid | Brow | Notes |
|---------|---------|---------|-------|--------|------|-------|
| Happy | +0.35 | 0.55 | 0.65 | 0.80 | +0.4 | Wide, bright |
| Sad | -0.35 | 0.35 | 0.35 | 0.40 | -0.3 | Drooping |
| Angry | -0.30 | 0.80 | 0.50 | 0.55 | -0.5 | Narrow, intense |
| Calm | +0.20 | 0.25 | 0.45 | 0.60 | 0.0 | Relaxed |
| Anxious | -0.20 | 0.75 | 0.70 | 0.50 | -0.2 | Alert, tense |
| Surprised | +0.15 | 0.85 | 0.75 | 0.95 | +0.3 | Very wide |
| Tired | +0.05 | 0.10 | 0.30 | 0.35 | -0.1 | Heavy lids |
| Excited | +0.30 | 0.90 | 0.80 | 0.90 | +0.5 | Energetic |
| Content | +0.25 | 0.40 | 0.50 | 0.70 | +0.2 | Gentle |
| Curious | +0.15 | 0.60 | 0.60 | 0.75 | +0.3 | Attentive |

**Mapping Functions** (baseline; configurable):
```
pupil_dilation = 0.3 + (arousal × 0.5) + (max(0, valence) × 0.4)
eyelid_openness = 0.4 + (arousal × 0.5) + ((valence + 0.5) × 0.2)
brow_angle = valence × 2.0  // Range: -1.0 (sad) to +1.0 (happy)
eye_squint = max(0, (0.5 - arousal) × 0.4) if valence < 0 else 0
```

### Platform Profiles

| Platform | SRAM | Target FPS | Features | Notes |
|----------|------|------------|----------|-------|
| **Arduino Nano** | 2 KB | 15-20 | Core emotions, basic animations | Primary target (tight memory) |
| **Arduino Mega 2560** | 8 KB | 20-25 | Full feature set | Comfortable memory |
| **ESP32** | 520 KB | 30-60 | Full + WiFi/BLE | High performance |
| **PC Emulator** | >1 GB | 60+ | Full + debugging, frame capture | Development platform |

*Note: Arduino Nano requires careful memory management (~400 bytes user code headroom)*

### Key Entities

- **Emotion State**: Current (valence, arousal) coordinates and interpolation target
- **Eye Position State**: Independent 2D position per eye (horizontal/vertical angles, animation state)
- **Expression Parameters**: Derived visuals (pupil dilation, eyelid openness, brow angle, pupil position, eyelid curve)
- **Animation Interpolator**: Smooth transitions using easing functions (emotion-specific)
- **Display Adapter**: Platform-specific interface (OLED I2C/SPI, PC window) with captureFrame() for emulator
- **Emotion Mapper**: Converts (valence, arousal) → expression parameters using mapping table
- **Eye Renderer**: Draws 5-8 geometric primitives per eye optimized for low-resolution displays
- **State Inspector**: Serializes expression state to JSON for AI feedback and debugging

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Developers integrate library into robot project in under 15 minutes using provided examples
- **SC-002**: Eyes display 10+ distinct recognizable emotions (happy, sad, angry, calm, surprised, curious, tired, excited, anxious, content) using valence-arousal coordinates
- **SC-003**: Expression transitions complete smoothly in 300-500ms without jarring jumps
- **SC-004**: Library runs on Arduino Mega 2560 (8KB RAM) with >2KB free for application code
- **SC-005**: PC emulator enables developers to iterate without physical hardware
- **SC-006**: Copilot can implement rendering, capture frames, verify output, and refine code in iterative loop
- **SC-007**: AI verifies aesthetic quality of 10+ emotions by analyzing captured frames from emulator
- **SC-008**: AI integration requires <10 lines of code for runtime emotion control (excluding setup)
- **SC-009**: Library maintains 30+ FPS on ESP32, 20+ FPS on Arduino Mega, 60+ FPS on PC emulator
- **SC-010**: Expression intensity varies noticeably across valence/arousal ranges (pixel-level distinguishable)
- **SC-011**: Eye shapes remain aesthetically pleasing on displays as small as 64x48 pixels per eye
- **SC-012**: Independent eye movements (H/V) complete in 200-400ms with smooth interpolation
- **SC-013**: Users identify gaze direction (left/right/up/down/converge/diverge) with >90% accuracy
- **SC-014**: Library compiles successfully as Arduino library in Arduino IDE for Mega 2560 and ESP32 targets
- **SC-015**: Library has zero dependencies beyond Adafruit GFX for embedded platforms
- **SC-016**: MVP emulator demonstrates basic rendering with PNG export functional before Arduino implementation begins

## Assumptions

- **Display**: Monochrome OLED (SSD1306, SH1106), 0.96"-1.3", 64x48 to 128x64 pixels per eye; grayscale/color future
- **Configuration**: Manual display setup required (type, protocol, pins, resolution specified in initialize())
- **Platform**: Arduino Mega 2560 minimum (8KB SRAM); ESP32 recommended; Nano not supported due to memory
- **Developers**: Basic Arduino/C++ knowledge; AI-driven development primary workflow
- **Architecture**: Standalone C++ library (not sketch); compiled by Arduino IDE; follows Arduino library conventions (.h/.cpp structure)
- **Dependencies**: Minimal - Adafruit GFX only for embedded platforms; Python + Pygame for emulator
- **MVP Priority**: PC emulator with PNG export implemented first to enable AI visual feedback loop before full Arduino feature set
- **Emulator**: Python 3.8+ with Pygame for cross-platform compatibility (Windows/macOS/Linux); trivial PNG capture and AI integration
- **Rendering**: 5 primitives per eye (outer ellipse, pupil, upper lid, lower lid, highlight) with anti-aliasing via dithering; Adafruit GFX foundation
- **Animations**: Built-in only (blink, wink, roll with configurable parameters); no custom animation creation
- **Transitions**: Smooth (100-200ms latency acceptable) prioritized over real-time; easing functions emotion-specific; default 400ms for emotions, 300ms for positions
- **Mapping**: Hardcoded baseline formulas; not customizable in v1; sensible defaults provided (see Emotion Mapping Table)
- **Timing**: Developer calls update() at target FPS; library does not use interrupts or internal timing
- **Serial**: 115200 baud sufficient for AI control; bidirectional (query state via QUERY command)
- **Error Handling**: All API methods return ErrorCode enum; no exceptions or global state; serial errors return "ERROR:<code>" strings
- **Memory**: Static allocation only (no malloc/new); all buffers sized at compile-time or initialize()
- **Debug**: No runtime debug output; use getExpressionState() JSON for diagnostics and troubleshooting
- **Initialization**: Eyes display neutral (0.0, 0.5) immediately after successful initialize()
- **Compatibility**: RoboEyes backward compatibility not required; migration guide provided
- **Gaze**: Both eyes move together by default (parallel); independent control when explicitly commanded
