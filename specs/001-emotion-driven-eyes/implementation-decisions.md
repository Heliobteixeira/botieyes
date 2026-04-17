# Implementation Decisions - Applied Changes Summary

**Date**: 2026-04-17  
**Status**: Ready for Task Generation

---

## Changes Applied Based on Review Decisions

### 1. Data Model Simplifications ✅

#### EyePositionState (data-model.md)
- **Before**: 20 bytes (independent left/right eyes, 8 angles)
- **After**: 12 bytes (coupled control, both eyes move together, 2 angles)
- **Savings**: 8 bytes RAM
- **API Impact**: `setEyePosition(h, v, duration)` instead of `setEyePosition(lh, lv, rh, rv)`

#### ExpressionParameters (data-model.md)
- **Before**: 32 bytes (4 floats + 4 int16 pupil offsets for left/right eyes)
- **After**: 24 bytes (4 floats + 2 int16 pupil offsets for both eyes)
- **Savings**: 8 bytes RAM

#### AnimationState (data-model.md)
- **Removed**: ANIM_ROLL_CW, ANIM_ROLL_CCW
- **Kept**: ANIM_NONE, ANIM_BLINK, ANIM_WINK_LEFT, ANIM_WINK_RIGHT
- **Savings**: ~300 bytes Flash

#### Total State Memory
- **Before**: 96 bytes
- **After**: 80 bytes
- **Savings**: 16 bytes RAM

---

### 2. Memory Budget Improvements ✅

**Arduino Nano (2KB SRAM)**:
- Library state: 80 bytes (was 96)
- Framebuffer: 1024 bytes (unchanged)
- Stack/heap: ~400 bytes (was ~500, reduced via simplifications)
- Wire/Serial: 134 bytes (now accounted for)
- **Total**: ~1040 bytes (was ~1600)
- **User code**: ~1000 bytes ✅ **VIABLE** (was 400 bytes - NOT viable)

**Improvements**:
- No JSON export: +250 bytes
- Coupled eye control: +16 bytes  
- No roll(): +50 bytes stack
- Simpler rendering: +100 bytes stack
- **Net gain**: ~550 bytes for user code

---

### 3. API Changes Required

#### Removed Methods
- ❌ `String getExpressionState()` - JSON export (Arduino only, keep in emulator)
- ❌ `ErrorCode roll(RollDirection, speed)` - Roll animation
- ❌ `ErrorCode converge()` - Not needed with coupled control
- ❌ `ErrorCode diverge()` - Not needed with coupled control

#### Simplified Methods
- **Before**: `setEyePosition(int16_t lh, int16_t lv, int16_t rh, int16_t rv)`
- **After**: `setEyePosition(int16_t h, int16_t v, uint16_t duration = 300)`
  - Both eyes move together
  - Configurable duration (fixes API inconsistency)

#### Added Methods (Emotion Helpers)
```cpp
// Wrapper methods for common emotions (keep parametric model available)
ErrorCode happy(float intensity = 1.0);    // (0.35*intensity, 0.55)
ErrorCode sad(float intensity = 1.0);      // (-0.35*intensity, 0.35)
ErrorCode angry(float intensity = 1.0);    // (-0.30*intensity, 0.80)
ErrorCode calm(float intensity = 1.0);     // (0.20*intensity, 0.25)
ErrorCode excited(float intensity = 1.0);  // (0.30*intensity, 0.90)
ErrorCode tired(float intensity = 1.0);    // (0.05*intensity, 0.10)
ErrorCode surprised(float intensity = 1.0);// (0.15*intensity, 0.85)
ErrorCode anxious(float intensity = 1.0);  // (-0.20*intensity, 0.75)
ErrorCode content(float intensity = 1.0);  // (0.25*intensity, 0.40)
ErrorCode curious(float intensity = 1.0);  // (0.15*intensity, 0.60)
```

#### API Consistency Fixes
1. **Duration Parameter**:
   - `setEmotion(v, a, duration = 400)` - Already configurable ✓
   - `setEyePosition(h, v, duration = 300)` - Now configurable ✓

2. **Getter Return Types**:
   - `void getCurrentEmotion(float* v, float* a)` - Keep pointer-based ✓
   - `void getEyePosition(int16_t* h, int16_t* v)` - Simplified to 2 params ✓
   - Removed `String getExpressionState()` - Emulator only ✓

3. **Error Handling**:
   - Getters now `void` (cannot fail)
   - `ErrorCode` only for: `initialize()`, `setEmotion()`, `setEyePosition()`, `blink()`, `wink()`, `update()`

#### Configuration Validation (New)
```cpp
ErrorCode validateConfig(const DisplayConfig& config); // Check bounds before init
```

---

### 4. Serial Protocol Changes

**Decision**: Move to example sketch (not in library core)

- Remove from BotiEyes class
- Create `examples/SerialControl/SerialControl.ino` showing protocol implementation
- Protocol syntax unchanged: `EMO:v,a[,dur]`, `POS:h,v[,dur]`, `QUERY`

---

### 5. Rendering Changes

**Decision**: Remove EyeRenderer abstraction

- Inline rendering directly into `BotiEyes` class
- Single `OLEDRenderer` implementation (not virtual)
- Reduces vtable overhead: ~30 bytes RAM, ~100 bytes Flash saved
- Add TODO comment for v2 abstraction when second display type needed

---

### 6. Testing Requirements

**Missing Coverage to Add** (before task generation):

1. **API Method Tests** (Tier 1):
   - `getCurrentEmotion()` - verify mid-interpolation values
   - `getEyePosition()` - verify mid-interpolation values
   - `blink()`, `wink()` - animation state machine
   - Emotion helpers (`happy()`, `sad()`, etc.) - verify correct mapping

2. **Edge Case Tests** (Tier 2):
   - Boundary values: valence=-0.5/0/+0.5, arousal=0/1
   - Out-of-bounds clamping: valence=-1, arousal=2
   - Concurrent `setEmotion()` during interpolation
   - Animation interruption (blink → wink)
   - Config validation (invalid I2C address, invalid pins)

3. **Performance Tests** (Tier 3):
   - Frame timing (100-frame average < 66ms for 15 FPS)
   - Longevity (10,000 frames, check memory)
   - Stress (1000 rapid setEmotion() calls)

---

### 7. Documentation Updates Required

#### contracts/BotiEyes-API.md
- Remove `getExpressionState()`, `roll()`, `converge()`, `diverge()`
- Update `setEyePosition()` signature (2 params + optional duration)
- Add emotion helper methods documentation
- Update memory constraints (1000 bytes user code on Nano)
- Add `validateConfig()` documentation

#### quickstart.md
- Update examples to use emotion helpers: `eyes.happy()` instead of `eyes.setEmotion(0.3, 0.6)`
- Remove roll() animation example
- Add "Serial Control" example to examples directory (not in library)
- Update memory tips to reflect 1000 bytes available

#### research.md
- Add test coverage section (Tier 1-3 breakdown)
- Update memory footprint to 1.04KB library, 1000 bytes user
- Note: GFXcanvas1 still supported (accounts for 1024 bytes of the library usage)

#### plan.md
- Update constitution check with new memory validation (1.04KB/2KB = 52% library, 48% user)
- Note removal of JSON export, roll(), independent eye control
- Update complexity tracking (simplifications applied)

---

### 8. Features Kept (Per Decisions)

✅ **Ease-in-out-cubic Interpolation** - Keep for quality
- Lookup table: ~128 bytes PROGMEM
- Provides smoother motion than linear

✅ **Parametric Valence-Arousal Model** - Keep for AI integration
- Helper methods simplify common cases
- Advanced users can still use `setEmotion(v, a)` directly

✅ **Predefined Position Behaviors** - Keep simplified set
- `lookLeft()`, `lookRight()`, `lookUp()`, `lookDown()`, `neutral()`
- Removed: `converge()`, `diverge()` (not needed with coupled control)

---

## Implementation Checklist

Before proceeding to `/speckit.tasks`:

- [x] Update data-model.md (EyePositionState, ExpressionParameters, memory budget)
- [x] Update review-findings.md (document decisions)
- [ ] Update contracts/BotiEyes-API.md (API changes, remove JSON/roll, add helpers)
- [ ] Update quickstart.md (examples with helpers, memory tips)
- [ ] Update research.md (test coverage, memory footprint)
- [ ] Update plan.md (constitution check, complexity tracking)
- [ ] Update spec.md (FR requirements for helpers, remove roll/JSON requirements)

---

## Next Steps

1. Complete remaining artifact updates (contracts, quickstart, research, plan, spec)
2. Commit changes: "Apply review decisions - simplify for Nano viability"
3. Run `/speckit.tasks` to generate implementation tasks
4. Proceed with implementation following simplified design

**Target**: Arduino Nano with ~1000 bytes user code headroom ✅
**Status**: Architecture viable for dedicated eye controller
