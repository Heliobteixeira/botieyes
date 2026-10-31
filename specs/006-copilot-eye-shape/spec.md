# Feature Specification: Copilot-style Eye Shape

**Feature Branch**: `006-copilot-eye-shape`

**Created**: 2026-10-31

**Status**: Draft

**Tracking Issue**: #4

**Input**: User description: "Change the eye shape from the ellipse to a shape like GitHub Copilot eyes. Also drop the emulator logic."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Copilot-style pill eyes on hardware (Priority: P1)

The eye body is drawn as a tall, vertical rounded rectangle ("pill"), similar to the GitHub Copilot mascot, instead of a filled ellipse. Emotions, blinks, gaze, and asymmetry continue to work on the new shape.

**Why this priority**: This is the main visual change requested.

**Independent Test**: Build and flash an example to the TTGO LoRa32 (SSD1306 128×64) and to an ST7789 240×135 board. Cycle through all emotion presets and blink/wink, then move the gaze.

**Acceptance Scenarios**:

1. **Given** the default Neutral emotion, **When** the eyes render, **Then** each eye is a vertical pill (height > width) with fully rounded top and bottom ends.
2. **Given** any of the 12 emotion presets, **When** eyelid overlays are applied, **Then** the lid edge carves cleanly into the pill without stray corner pixels.
3. **Given** a blink or wink, **When** the eye height shrinks to its 2 px minimum, **Then** the eye renders as a thin line without glitches.
4. **Given** the Thinking or Confused emotion, **When** asymmetry is applied, **Then** both eyes remain pills of different heights.
5. **Given** an ST7789 display, **When** the eyes render, **Then** the pills use the existing cyan eye color.

---

### User Story 2 - Remove the PC emulator (Priority: P2)

The Python/Pygame emulator is removed from the repository so there is a single renderer: the C++ library.

**Why this priority**: The emulator duplicates rendering logic, has already drifted from the firmware, and would need a parallel update for the new shape.

**Independent Test**: `emulator/` no longer exists, and `git grep -i emulator -- ':!specs'` returns only intentional historical mentions.

**Acceptance Scenarios**:

1. **Given** the repository, **When** listing the top-level folders, **Then** `emulator/` is absent.
2. **Given** the READMEs, library metadata, architecture docs, `.gitignore`, and Copilot instructions, **When** reading them, **Then** they no longer describe or reference running the emulator.

---

### Edge Cases

- Eye width or height at range limits: the corner radius `min(w, h) / 2` must never exceed half of either dimension.
- Height reduced to 2 px during a blink: the radius collapses to 1 and `fillRoundRect` must still render.
- Eyelid coverage at its maximum (0.5) on a flat-sided pill: there must be no leftover sliver at the straight edges.

## Requirements *(mandatory)*

### Functional Requirements

- **FR-001**: The library MUST render each eye body as a filled rounded rectangle with corner radius `min(width, height) / 2`.
- **FR-002**: The ellipse MUST NOT remain as an eye-body option. There is no runtime shape selection.
- **FR-003**: Eyelid overlays, blink/wink scaling, gaze offset, vergence, and asymmetry MUST keep their current behavior when applied to the new shape.
- **FR-004**: The default emotion parameters SHOULD produce eyes whose height is greater than their width.
- **FR-005**: The `emulator/` directory and its non-historical documentation references MUST be removed.
- **FR-006**: Rendering MUST go only through `Adafruit_GFX` primitives, with no driver-specific code, so that SSD1306 and ST7789 continue to work.

### Out of Scope

- Goggle/visor outline around the eyes
- Runtime-selectable eye shapes
- Changes to the network protocol or public API

## Success Criteria *(mandatory)*

- **SC-001**: The eyes are visually confirmed as Copilot-style pills on both the SSD1306 and ST7789 hardware.
- **SC-002**: All 12 emotion presets render without eyelid artifacts.
- **SC-003**: The ESP-IDF firmware builds cleanly, and the frame rate does not regress noticeably compared with the ellipse renderer.
- **SC-004**: There is no emulator code and no emulator usage instructions outside `specs/`.
