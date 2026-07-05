# Architecture Enforcement Strategy for ESP-IDF Development

This document explains how to maintain the professional industrial architecture defined in [004-industrial-firmware-architecture](../specs/004-industrial-firmware-architecture/spec.md) when working with AI coding assistants.

## 1. Repository Memory (Active Now ✅)

**Location**: `/memories/repo/botieyes.md`

This file is automatically loaded into Copilot's context and contains:
- Hardware constraints
- **ESP-IDF Architecture Rules** (component structure, communication patterns, layering)
- Architecture verification checklist

**When to update**: After implementing each architectural pattern from the plan.

## 2. Copilot Instructions (Active Now ✅)

**Location**: `.github/copilot-instructions.md`

Added "Architecture Enforcement" section with:
- Correct vs incorrect code patterns (✅/❌ examples)
- Component boundary rules
- Event-driven communication examples
- Copilot checklist for code review

**Copilot automatically reads this file** in every session.

## 3. How to Work with Copilot

### Before Making Changes

**Always state architectural constraints in your prompt:**

```
"Add WiFi reconnection logic following our architecture:
- Use ESP event loop, not direct callbacks
- Post reconnection events via esp_event
- Queue commands to application task
- Check all error returns
- Use hierarchical logging"
```

### When Reviewing AI Suggestions

**Use the checklist from copilot-instructions.md:**

```
Before accepting Copilot's code:
□ Uses esp_event for WiFi/system events?
□ Uses queues for inter-task communication?
□ Hardware access only in HAL layer?
□ All error returns checked?
□ Watchdog fed in loops?
□ Proper timeouts (not portMAX_DELAY)?
□ Hierarchical log tags (LAYER:COMP:MOD)?
```

### When Copilot Violates Architecture

**Reject and re-prompt with constraints:**

```
❌ Copilot suggested:
void on_wifi_connect(void) {
    wifi_connected = true;  // Global flag
    start_network_service();  // Direct call
}

✅ Re-prompt:
"Rewrite using ESP event loop instead of callback.
Post WIFI_EVENT_CONNECTED event and handle it in 
the event handler. No global flags - use event groups."
```

## 4. Component Templates (TODO)

Create component scaffolding to enforce structure:

```bash
# Future: Create script
./tools/create_component.sh wifi_manager service

# Generates:
esp-idf/components/wifi_manager/
  ├── CMakeLists.txt          # Pre-configured dependencies
  ├── Kconfig                 # Component config
  ├── include/wifi_manager/
  │   └── wifi_manager.h      # Public API template
  └── src/
      └── wifi_manager.c      # Implementation template
```

## 5. Pre-Commit Hooks (TODO - Phase 2)

Add `.pre-commit-config.yaml`:

```yaml
repos:
  - repo: local
    hooks:
      - id: check-component-deps
        name: Check component dependencies
        entry: ./tools/check_component_deps.py
        language: python
        pass_filenames: false
        
      - id: check-event-usage
        name: Verify event-driven patterns
        entry: ./tools/check_event_usage.sh
        language: script
        files: 'esp-idf/.*\.(c|cpp|h)$'
```

## 6. Static Analysis (TODO - Phase 2)

Add to `esp-idf/.clang-tidy`:

```yaml
Checks: >
  -*,
  readability-identifier-naming,
  modernize-use-nullptr,
  cppcoreguidelines-no-malloc,
  cppcoreguidelines-pro-type-vararg

CheckOptions:
  - key: readability-identifier-naming.FunctionCase
    value: lower_case
  - key: readability-identifier-naming.VariableCase
    value: lower_case
```

## 7. Architecture Review Agent (Future Enhancement)

Create custom Copilot agent for architecture review:

```markdown
# .copilot/agents/arch-review.md

You are an ESP-IDF architecture reviewer. Review code for:
1. Component boundaries (public/private separation)
2. Event-driven patterns (esp_event, queues, event groups)
3. Hardware abstraction (HAL layer)
4. Error handling (all esp_err_t checked)
5. Watchdog feeding in long tasks
6. Proper timeouts (no portMAX_DELAY in ISR)

When reviewing, cite specific violations and suggest fixes.
```

## 8. Practical Workflow Example

### Task: "Add temperature sensor reading"

**❌ Without Architecture Enforcement:**
```
You: "Add temperature sensor reading"

Copilot generates:
- Direct I2C access in main.cpp
- Global variable for temperature
- Polling loop checking sensor every 100ms
```

**✅ With Architecture Enforcement:**
```
You: "Add temperature sensor following our architecture:
- Create 'temp_sensor' component under esp-idf/components/
- HAL abstracts I2C access
- Sensor task posts SENSOR_READING_READY event via esp_event
- Application subscribes to event, receives data via queue
- Use hierarchical logging: HAL:SENSOR:TEMP"

Copilot generates:
esp-idf/components/temp_sensor/
  include/temp_sensor/temp_sensor.h  # Public API
  src/temp_sensor.c                  # Implementation with events
  CMakeLists.txt                     # Declares dependencies

main/app_main.c
  - Registers event handler
  - Subscribes to SENSOR_READING_READY
```

## 9. Training New Developers

**Onboarding checklist:**

1. Read `specs/004-industrial-firmware-architecture/spec.md`
2. Review `/memories/repo/botieyes.md` architecture rules
3. Study `.github/copilot-instructions.md` patterns
4. Run through example: "Create simple component with Copilot"
5. Practice rejecting bad suggestions and re-prompting

## 10. Monitoring Architecture Drift

**Weekly review:**

```bash
# Check for architecture violations
grep -r "gpio_set_level" esp-idf/components/*/src/ | grep -v "hal/"
# Should be empty - GPIO only in HAL

# Check for polling loops
grep -r "while.*connected" esp-idf/components/
# Should use event groups instead

# Check for missing error checks
./tools/check_error_handling.py esp-idf/components/
```

## Summary: Active Now vs Future

| Strategy | Status | Effort |
|----------|--------|--------|
| Repository memory rules | ✅ Active | Done |
| Copilot instructions | ✅ Active | Done |
| Prompt engineering | ✅ Active | Use now |
| Component templates | 📋 TODO | 2 hours |
| Pre-commit hooks | 📋 TODO | 4 hours |
| Static analysis | 📋 TODO | 2 hours |
| Architecture agent | 📋 Future | 8 hours |

**Start using immediately:**
1. Reference architecture rules in every prompt
2. Use the checklist before accepting code
3. Reject violations and re-prompt with constraints

**This maintains architecture with minimal effort while leveraging AI assistance.**
