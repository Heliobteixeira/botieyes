# Managing Postponed Features

This document explains how to track, prioritize, and eventually implement postponed feature work in the BotiEyes project.

## Current Postponed Items (Feature 004)

### 🔧 Technical Debt

| Item | Status | Priority | Effort | Impact |
|------|--------|----------|--------|--------|
| HAL Display Implementation | STUB (42 lines) | Medium | 6-8h | High - enables multi-board support |
| Display Component | TEMPORARY | Medium | 3-4h | Medium - architectural cleanup |
| Kconfig Documentation (T060) | DEFERRED | Low | 1h | Low - documentation only |
| Config Documentation (T080) | DEFERRED | Low | 1h | Low - documentation only |

### 📍 Location of Postponed Work

#### 1. **HAL Display SPI/I2C Implementation**
- **File**: `esp-idf/components/hal_board/src/hal_display_spi.c` (42 lines, stub)
- **File**: `esp-idf/components/hal_board/src/hal_display_i2c.c` (40 lines, stub)
- **Current Workaround**: `components/display/` temporary component
- **Why Postponed**: Time/complexity - working solution in place, not blocking
- **To Complete**:
  ```cpp
  // TODO in hal_display_spi.c:
  // - Configure SPI master bus
  // - Initialize SSD1306 controller
  // - Create Adafruit_GFX display object
  // - Return proper display handle
  ```

#### 2. **Tasks T060, T080 - Kconfig Documentation**
- **File**: `specs/004-industrial-firmware-architecture/tasks.md`
- **Tasks**:
  - `[ ] T060`: Move WiFi SSID/password to wifi_manager component Kconfig
  - `[ ] T080`: Document Kconfig vs NVS split in comments
- **Why Postponed**: Non-blocking documentation improvements
- **Priority**: Low - doesn't affect functionality

## Tracking Mechanisms

### 1. **Task Files** (Primary)
Location: `specs/[feature-number]-[feature-name]/tasks.md`

**Format**:
```markdown
- [X] T001 Completed task
- [ ] T060 Postponed task description
```

**When to Use**: 
- Feature-specific tasks
- Implementation checklists
- Track completion status across feature development

### 2. **Code Comments** (In-File)
**Formats**:
```cpp
// TODO: Description of future work
// FIXME: Known issue that needs addressing  
// XXX: Critical problem requiring attention
```

**When to Use**:
- Mark specific locations needing work
- Quick context for developers reading code
- Keep close to the code that needs changing

**Example** (from `hal_display_spi.c`):
```cpp
// TODO: Implement SPI display initialization
// - Configure SPI master
// - Initialize SSD1306 controller
// - Create Adafruit_GFX display object
```

### 3. **Component READMEs** (Documentation)
Location: `components/[component-name]/README.md`

**When to Use**:
- Explain temporary/workaround components
- Document architectural intentions
- Provide context for future developers

**Example** (`components/display/README.md`):
```markdown
## Status: ⚠️ Temporary Bridge Component

This component should eventually be **removed** once 
`components/hal_board/src/hal_display_spi.c` is fully implemented.
```

### 4. **Feature Spec Future Work** (Planning)
Location: `specs/[feature]/spec.md` - Future Work section

**When to Use**:
- Major enhancements beyond current scope
- Ideas for next iterations
- Cross-feature dependencies

## Best Practices

### ✅ DO

1. **Document WHY Postponed**
   ```markdown
   - [ ] T060 Kconfig organization
     Reason: Non-blocking documentation improvement
     Priority: Low
   ```

2. **Provide Clear Next Steps**
   ```cpp
   // TODO: Complete HAL display implementation
   // Steps:
   // 1. Initialize SPI bus with CONFIG_* parameters
   // 2. Call ssd1306_init() from external library
   // 3. Wrap in Adafruit_GFX adapter class
   // Estimated effort: 4-6 hours
   ```

3. **Mark Temporary Solutions**
   ```cpp
   // TEMPORARY: Using display component bridge
   // See components/display/README.md for migration path
   #include "display_init.h"  // Legacy (hal_display_spi.c incomplete)
   ```

4. **Track in Version Control**
   ```bash
   git commit -m "feat: add display component (temporary)
   
   This is a pragmatic workaround until HAL implementation
   is complete. See components/display/README.md.
   
   Related: T034 (hal_display_spi.c stub)"
   ```

### ❌ DON'T

1. **Leave Unmarked Incomplete Code**
   ```cpp
   // ❌ BAD: Silent failure with no documentation
   return NULL;  // Why? When will this be fixed?
   
   // ✅ GOOD: Explicit temporary status
   ESP_LOGW(TAG, "SPI display init stub - not yet implemented");
   return NULL;  // TODO: Return proper handle (see T034)
   ```

2. **Create Separate "TODO.md" Files**
   - TODOs scattered in separate files get lost
   - Use task files for trackable items
   - Use code comments for inline context

3. **Postpone Without Tracking**
   ```markdown
   ❌ Just skip implementation
   ✅ Mark task with [ ] and add "DEFERRED" note
   ```

## Converting Postponed Items to New Features

When postponed work grows large enough, create a new feature:

### Step 1: Assess Scope
```bash
# Gather all related TODOs
grep -r "TODO.*HAL.*display" esp-idf/components/

# Count affected files
find esp-idf/components/hal_board -name "hal_display*" | wc -l
```

### Step 2: Create Feature Spec
```bash
# Create new feature directory
mkdir -p specs/005-hal-display-completion/

# Generate spec using Spec Kit
/speckit.specify "Complete HAL display layer implementation 
to remove dependency on temporary display component bridge"
```

### Step 3: Link to Previous Work
In new `specs/005-hal-display-completion/spec.md`:
```markdown
## Context

This feature completes work deferred from Feature 004:
- Tasks T033, T034 were implemented as stubs
- Temporary workaround: `components/display/` bridge component
- See: commit 90214b2 (display component creation)

## Success Criteria

- Remove `components/display/` component entirely
- Complete `hal_display_spi.c` and `hal_display_i2c.c`
- Update `main.cpp` to use HAL APIs directly
- Build passes on both TTGO LoRa32 and ESP32-S3
```

### Step 4: Update Task Statuses
In `specs/004-industrial-firmware-architecture/tasks.md`:
```markdown
- [ ] T034 [US3] Implement hal_display_spi.c
  **Status**: DEFERRED to Feature 005 - HAL Display Completion
  **Workaround**: Using components/display/ temporary bridge
```

## Priority Guidelines

### 🔴 High Priority - Address Soon
- Blocks other features
- Security/stability issues
- Performance bottlenecks
- User-facing functionality

### 🟡 Medium Priority - Next Iteration
- Architectural improvements
- Technical debt with workarounds in place
- Nice-to-have enhancements

**Example**: HAL display implementation (Medium)
- Workaround exists and functions correctly
- Completing it improves architecture but doesn't add functionality
- Can wait until next major feature or refactoring cycle

### 🟢 Low Priority - Backlog
- Documentation improvements
- Code organization (non-functional)
- Optional optimizations
- Future "nice-to-haves"

**Example**: Kconfig documentation (Low)
- Doesn't affect functionality
- Developers can understand code without it
- Polish for future maintainability

## Review Cycle

### Monthly Review
Check all postponed items:
```bash
# Find all TODO comments
grep -rn "TODO\|FIXME\|XXX" esp-idf/components/ esp-idf/main/

# Check incomplete tasks
grep -n "^\- \[ \]" specs/**/tasks.md

# List temporary components
find esp-idf/components -name "README.md" -exec grep -l "TEMPORARY\|temporary" {} \;
```

### Before Major Release
1. **Audit**: List all postponed items
2. **Prioritize**: Decide what must be done vs can wait
3. **Document**: Update this file with current status
4. **Communicate**: Document known limitations in release notes

## Example: Current Feature 004 Status

**Completed**: 120/122 tasks (98.4%)

**Postponed**:
```markdown
1. T060 - Kconfig WiFi config organization (LOW, 1h)
   - Non-blocking documentation
   - Current setup works correctly
   - Can be done in future polish pass

2. T080 - Kconfig vs NVS documentation (LOW, 1h)  
   - Non-blocking documentation
   - Developers understand the split from code
   - Nice-to-have for onboarding

3. HAL Display Implementation (MEDIUM, 6-8h)
   - Stub implementation with TODOs
   - Workaround: components/display/ bridge (commit 90214b2)
   - Deferred to potential Feature 005
   - Build works, multi-board supported via temporary solution
```

**Decision**: Ship Feature 004 with these items postponed
- All critical functionality works
- Workarounds are documented and stable
- Can address in future iterations

---

## Quick Reference

**Creating a postponed item**:
1. Mark task `[ ]` in tasks.md with reason
2. Add TODO comment at code location
3. Document workaround (if any)
4. Update component README if needed

**Reviewing postponed items**:
```bash
# All incomplete tasks
grep "^\- \[ \]" specs/**/tasks.md

# All code TODOs
grep -rn "TODO" esp-idf/

# Temporary components
find esp-idf/components -name "README.md" -exec grep -l "temporary\|TEMPORARY" {} \;
```

**Promoting to new feature**:
1. Gather related postponed items
2. Run `/speckit.specify` with description
3. Link back to original feature/tasks
4. Update original tasks with "DEFERRED to Feature XXX"
