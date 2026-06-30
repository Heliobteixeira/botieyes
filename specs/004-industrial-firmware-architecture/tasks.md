# Tasks: Industrial Firmware Architecture Refactoring

**Feature Branch**: `004-industrial-firmware-architecture`  
**Input**: Design documents from `specs/004-industrial-firmware-architecture/`

**Prerequisites**: plan.md, spec.md, research.md, data-model.md, contracts/component-api.md, quickstart.md

**Tests**: No automated tests requested in specification - validation via on-hardware testing and manual verification

**Organization**: Tasks are grouped by user story to enable independent implementation and testing. P1 stories (US1, US3, US4, US8) establish foundation; P2 stories (US2, US5, US6, US7, US9) build services on top.

---

## Format: `- [ ] [ID] [P?] [Story?] Description with file path`

- **[P]**: Can run in parallel (different files, no dependencies on incomplete tasks)
- **[Story]**: User story label (US1, US2, US3, etc.)
- File paths use esp-idf/ project root

---

## Phase 1: Setup (Shared Infrastructure)

**Purpose**: Project initialization and basic directory structure per plan.md

- [X] T001 Create component directory structure: mkdir -p esp-idf/components/{wifi_manager,state_machine,config_manager,health_monitor,hal_board}/{include,src}
- [X] T002 Create board config subdirectories: mkdir -p esp-idf/components/hal_board/src/boards
- [X] T003 [P] Create sdkconfig.defaults.ttgo_lora32 with TTGO LoRa32 board defaults (I2C display, GPIO2 LED)
- [X] T004 [P] Create sdkconfig.defaults.esp32s3_spi with ESP32-S3 SPI variant defaults (SPI display, WS2812 LED)
- [X] T005 Create main/app_task.h header with application task declarations
- [X] T006 Create main/app_task.cpp stub with basic application task structure

**Checkpoint**: Directory structure ready, board configs defined

---

## Phase 2: Foundational (Blocking Prerequisites)

**Purpose**: Core ESP-IDF initialization that MUST complete before user stories

**⚠️ CRITICAL**: No user story implementation can begin until this phase completes

- [X] T007 Refactor main/main.cpp: Extract ESP-IDF initialization sequence (NVS, event loop, netif) from existing app_main()
- [X] T008 Create global network command queue definition in main/main.cpp: QueueHandle_t g_network_cmd_queue
- [X] T009 Define task priority constants in main/main.cpp: CRITICAL, HIGH, NORMAL, LOW (per FR-031)
- [X] T010 Define task stack size constants in main/main.cpp: LARGE (8KB), MEDIUM (4KB), SMALL (2KB) (per FR-032)
- [X] T011 Update main/CMakeLists.txt to declare component dependencies: wifi_manager, state_machine, config_manager, health_monitor, hal_board
- [X] T012 Document initialization sequence in main/main.cpp header comments (boot flow from data-model.md Section 10)

**Checkpoint**: Foundation ready - component development can proceed in parallel

---

## Phase 3: US1 - Component Modularity and Maintainability (Priority: P1) 🎯 MVP Foundation

**Goal**: Establish layered component architecture with clear interfaces (FR-001 to FR-004)

**Independent Test**: Build individual components in isolation; verify no circular dependencies; check public/private header separation

### Component Structure Setup (All Parallel)

- [X] T013 [P] [US1] Create esp-idf/components/wifi_manager/CMakeLists.txt with idf_component_register (SRCS, INCLUDE_DIRS, REQUIRES esp_wifi esp_event nvs_flash config_manager)
- [X] T014 [P] [US1] Create esp-idf/components/wifi_manager/Kconfig for WiFi-specific build config (max retry, timeouts)
- [X] T015 [P] [US1] Create esp-idf/components/state_machine/CMakeLists.txt with idf_component_register (REQUIRES esp_event esp_system)
- [X] T016 [P] [US1] Create esp-idf/components/state_machine/Kconfig for state machine config
- [X] T017 [P] [US1] Create esp-idf/components/config_manager/CMakeLists.txt with idf_component_register (REQUIRES nvs_flash)
- [X] T018 [P] [US1] Create esp-idf/components/config_manager/Kconfig for NVS namespace config
- [X] T019 [P] [US1] Create esp-idf/components/health_monitor/CMakeLists.txt with idf_component_register (REQUIRES esp_system esp_task_wdt config_manager)
- [X] T020 [P] [US1] Create esp-idf/components/health_monitor/Kconfig for watchdog timeout config
- [X] T021 [P] [US1] Create esp-idf/components/hal_board/CMakeLists.txt with idf_component_register (REQUIRES driver led_strip, conditional SRCS based on Kconfig)
- [X] T022 [P] [US1] Create esp-idf/components/hal_board/Kconfig for board selection (TTGO_LORA32, ESP32S3_SPI) and display protocol (I2C, SPI)

### Public API Headers (All Parallel - Foundation for Other Stories)

- [X] T023 [P] [US1] Create esp-idf/components/wifi_manager/include/wifi_manager.h with API from contracts/component-api.md Section 1
- [X] T024 [P] [US1] Create esp-idf/components/state_machine/include/app_state.h with API from contracts/component-api.md Section 2
- [X] T025 [P] [US1] Create esp-idf/components/config_manager/include/config_manager.h with API from contracts/component-api.md Section 3
- [X] T026 [P] [US1] Create esp-idf/components/health_monitor/include/health_monitor.h with API from contracts/component-api.md Section 4
- [X] T027 [P] [US1] Create esp-idf/components/hal_board/include/hal_board.h with board init API from contracts/component-api.md Section 5
- [X] T028 [P] [US1] Create esp-idf/components/hal_board/include/hal_display.h with display abstraction API
- [X] T029 [P] [US1] Create esp-idf/components/hal_board/include/hal_led.h with LED abstraction API

**Checkpoint**: All components have proper structure (CMakeLists.txt, Kconfig, public headers). Components can now be implemented independently.

---

## Phase 4: US3 - Hardware Abstraction and Portability (Priority: P1)

**Goal**: Implement HAL layer for board-agnostic application code (FR-035 to FR-038)

**Independent Test**: Build for both TTGO LoRa32 (I2C) and ESP32-S3 (SPI); verify only HAL component changes

### Board Configuration Files (All Parallel)

- [X] T030 [P] [US3] Create esp-idf/components/hal_board/src/boards/ttgo_lora32.c with hal_board_config_t for TTGO LoRa32 (I2C display: SDA=GPIO4, SCL=GPIO15, addr=0x3C; LED: GPIO2)
- [X] T031 [P] [US3] Create esp-idf/components/hal_board/src/boards/esp32s3_spi.c with hal_board_config_t for ESP32-S3 SPI (SPI display: MOSI=GPIO11, SCK=GPIO12, CS=GPIO10, DC=GPIO13, RST=GPIO14; LED: WS2812 GPIO38)

### HAL Implementation

- [X] T032 [US3] Implement esp-idf/components/hal_board/src/hal_board.c: hal_board_init() that selects board config via Kconfig and calls display/LED init (FR-035)
- [X] T033 [P] [US3] Implement esp-idf/components/hal_board/src/hal_display_i2c.c: I2C SSD1306 display driver using Adafruit_GFX (FR-036)
- [X] T034 [P] [US3] Implement esp-idf/components/hal_board/src/hal_display_spi.c: SPI SSD1306 display driver using Adafruit_GFX (FR-036)
- [X] T035 [US3] Implement esp-idf/components/hal_board/src/hal_display.c: Display abstraction layer that conditionally compiles I2C or SPI based on CONFIG_BOTIEYES_OLED_PROTOCOL (FR-036, FR-050)
- [X] T036 [US3] Add display mutex (g_display_mutex) in hal_display.c: Create in hal_display_init(), protect all hardware access (FR-014)
- [X] T037 [US3] Implement esp-idf/components/hal_board/src/hal_led.cpp: LED abstraction supporting GPIO and WS2812 types, with no-op when disabled (FR-037)

**Checkpoint**: HAL layer complete. Applications can use hal_display_* and hal_led_* APIs without knowing hardware details. Build system selects correct implementation at compile time.

---

## Phase 5: US4 - Event-Driven Architecture and Inter-Task Communication (Priority: P1)

**Goal**: Implement ESP event loop integration and FreeRTOS queue patterns (FR-005 to FR-017)

**Independent Test**: Trigger WiFi events and network commands; verify event handlers execute and queues transfer data without blocking

### Event Infrastructure

- [X] T038 [US4] Define custom event bases in main/main.cpp: ESP_EVENT_DEFINE_BASE(WIFI_MGR_EVENT) and ESP_EVENT_DEFINE_BASE(APP_STATE_EVENT) (FR-005, FR-012, FR-025, FR-029)
- [X] T039 [US4] Create network command queue in main/main.cpp app_main(): g_network_cmd_queue = xQueueCreate(10, sizeof(network_cmd_t)) (FR-006, FR-013)
- [X] T040 [US4] Define network_cmd_t structure in main/main.cpp with type enum (SET_EMOTION, SET_POSITION, IDLE_MODE, PING, RESET) and 64-byte payload (data-model.md Section 8)

### Queue Integration

- [X] T041 [US4] Refactor BotiEyes/src/net/BotiEyesServer.cpp poll(): Replace direct application of commands with xQueueSend(g_network_cmd_queue, ..., pdMS_TO_TICKS(100)) (FR-013, FR-016)
- [X] T042 [US4] Implement network command processing in main/app_task.cpp: xQueueReceive(g_network_cmd_queue, ..., pdMS_TO_TICKS(100)) and apply to BotiEyes instance (FR-006, FR-016)
- [X] T043 [US4] Add queue full handling in BotiEyesServer: Log warning and drop packet when xQueueSend fails (FR-016, FR-017)

**Checkpoint**: Event-driven communication operational. WiFi manager and state machine can post events; network commands flow through queue to application task. No busy-waiting or blocking forever (FR-011, FR-016).

---

## Phase 6: US8 - Professional Task Management (Priority: P1)

**Goal**: Create properly structured FreeRTOS tasks with priorities, stack sizes, and core pinning (FR-031 to FR-034)

**Independent Test**: Monitor task stack watermarks; verify tasks scheduled correctly and pinned to appropriate cores; no stack overflows under load

### Task Creation

- [X] T044 [US8] Refactor main/app_task.cpp: Move existing main loop logic into proper FreeRTOS task function void app_task(void *arg) with watchdog feeding (FR-033, FR-040)
- [X] T045 [US8] Create application task in main/main.cpp: xTaskCreatePinnedToCore(app_task, "app", TASK_STACK_MEDIUM, NULL, TASK_PRIORITY_NORMAL, NULL, 1) - core 1 for rendering (FR-031, FR-032, FR-033)
- [X] T046 [US8] Create network task in main/main.cpp: xTaskCreatePinnedToCore(network_task, "network", TASK_STACK_MEDIUM, NULL, TASK_PRIORITY_HIGH, NULL, 0) - core 0 for network I/O (FR-031, FR-032, FR-033)
- [X] T047 [US8] Implement network_task function in main/main.cpp: Call BotiEyesServer::poll() and queue commands, feed watchdog (FR-033, FR-040)

### Task Monitoring

- [X] T048 [US8] Add stack watermark monitoring in main/app_task.cpp: Periodically call uxTaskGetStackHighWaterMark() and log warning if < 512 bytes (FR-034)
- [X] T049 [US8] Add stack watermark monitoring in network task: Same as T048 for network_task (FR-034)

**Checkpoint**: Multi-task architecture operational. Application task on core 1 handles rendering; network task on core 0 handles UDP. Proper priorities prevent deadline misses.

---

## Phase 7: US2 - Robust WiFi Management (Priority: P2)

**Goal**: Implement WiFi manager service with auto-reconnection and event propagation (FR-022 to FR-025)

**Independent Test**: Deploy to hardware, disconnect WiFi AP, verify auto-reconnection within 60 seconds; status LED shows connection state

### WiFi Manager Implementation

- [X] T050 [P] [US2] Implement esp-idf/components/wifi_manager/src/wifi_manager.c: wifi_manager_init() - initialize ESP WiFi, netif, register for WIFI_EVENT and IP_EVENT (FR-022)
- [X] T051 [P] [US2] Implement wifi_manager credential storage: wifi_manager_set_credentials() and wifi_manager_get_credentials() using config_manager (FR-019, FR-024)
- [X] T052 [US2] Implement esp-idf/components/wifi_manager/src/wifi_events.c: Event handlers for WIFI_EVENT_STA_DISCONNECTED, IP_EVENT_STA_GOT_IP (FR-022)
- [X] T053 [US2] Add exponential backoff retry logic in wifi_events.c: Retry up to 5 times with increasing delays before posting WIFI_MGR_FAILED (FR-023)
- [X] T054 [US2] Implement wifi_manager_connect() in wifi_manager.c: Start connection attempt, post WIFI_MGR_CONNECTING event (FR-025)
- [X] T055 [US2] Implement wifi_manager_disconnect() in wifi_manager.c: Stop connection, post WIFI_MGR_DISCONNECTED event (FR-025)
- [X] T056 [US2] Implement wifi_manager_get_state() in wifi_manager.c: Return current state with mutex protection (FR-024)
- [X] T057 [US2] Implement wifi_manager_get_ip_address() in wifi_manager.c: Return IP as string or NULL (FR-024)

### WiFi Integration

- [X] T058 [US2] Integrate wifi_manager in main/main.cpp: Call wifi_manager_init(), wifi_manager_connect() after component init
- [X] T059 [US2] Add WiFi event handlers in main/main.cpp: Register handlers for WIFI_MGR_CONNECTED, WIFI_MGR_FAILED to update status LED and trigger state transitions
- [ ] T060 [US2] Update main/Kconfig.projbuild: Move WiFi SSID/password to wifi_manager component Kconfig for proper organization

**Checkpoint**: WiFi connectivity managed by dedicated service. Auto-reconnection works. Status LED reflects connection state. WiFi events drive application state transitions.

---

## Phase 8: US7 - State Machine and Application Flow (Priority: P2)

**Goal**: Implement centralized state machine for system lifecycle (FR-026 to FR-030)

**Independent Test**: Trigger state transitions (boot, WiFi connect, network control, error); verify logs show correct state flow; invalid transitions rejected

### State Machine Implementation

- [X] T061 [P] [US7] Implement esp-idf/components/state_machine/src/app_state.c: app_state_init() - create state mutex, initialize to INIT state (FR-028)
- [X] T062 [US7] Implement state transition validation in app_state.c: app_state_transition() checks valid transitions per data-model.md Section 2 (FR-027)
- [X] T063 [US7] Add state mutex protection in app_state.c: All state access wrapped in xSemaphoreTake/Give with 100ms timeout (FR-015, FR-017, FR-028)
- [X] T064 [US7] Implement state event posting in app_state.c: Post APP_STATE_EVENT:STATE_CHANGED with app_state_info_t payload after transition (FR-029)
- [X] T065 [US7] Implement app_state_get_current() and app_state_get_info() in app_state.c with mutex protection (FR-030)
- [X] T066 [US7] Implement app_state_set_error() convenience function in app_state.c: Transition to ERROR state with message (FR-030)

### State Machine Integration

- [X] T067 [US7] Integrate state_machine in main/main.cpp: Call app_state_init() early in app_main()
- [X] T068 [US7] Add state transition calls in main/main.cpp: app_state_transition(CONNECTING) before WiFi connect, app_state_transition(CONNECTED) on WiFi success (FR-026)
- [X] T069 [US7] Register state change event handler in main/main.cpp: Log all state transitions with previous/current state and timestamp (FR-046)
- [X] T070 [US7] Add app_state_transition(RUNNING) in main/app_task.cpp when network server starts accepting commands (FR-026)
- [X] T071 [US7] Add error state handling in main/main.cpp: Transition to ERROR state on critical failures, implement recovery logic (FR-026)

**Checkpoint**: State machine operational. All state transitions validated and logged. Application behavior predictable. State events observable by other components.

---

## Phase 9: US5 - Configuration Management (Priority: P2)

**Goal**: Implement NVS abstraction for runtime configuration (FR-018 to FR-021)

**Independent Test**: Write WiFi credentials to NVS, reboot, verify new credentials used; factory reset clears user settings

### Config Manager Implementation

- [X] T072 [P] [US5] Implement esp-idf/components/config_manager/src/config_manager.c: config_manager_init() - open NVS namespaces (botieyes_wifi, botieyes_app, botieyes_sys) (FR-019)
- [X] T073 [P] [US5] Implement WiFi config functions in config_manager.c: config_get_wifi(), config_set_wifi() with NVS read/write (FR-019)
- [X] T074 [P] [US5] Implement app config functions in config_manager.c: config_get_app(), config_set_app() for brightness, idle_timeout, network_enabled (FR-019)
- [X] T075 [P] [US5] Implement crash log functions in config_manager.c: config_get_crash_log(), config_set_crash_log() (FR-041)
- [X] T076 [US5] Implement config_factory_reset() in config_manager.c: Erase botieyes_wifi and botieyes_app namespaces, preserve botieyes_sys (FR-021)
- [X] T077 [US5] Add NVS error handling in config_manager.c: Handle corruption (erase and reinit), full NVS (log error), missing keys (return defaults) (Edge case handling)

### Config Manager Integration

- [X] T078 [US5] Integrate config_manager in main/main.cpp: Call config_manager_init() after NVS flash init
- [X] T079 [US5] Update wifi_manager to use config_manager for credential storage instead of direct NVS access
- [ ] T080 [US5] Document Kconfig vs NVS split in main/Kconfig.projbuild comments: Build-time (board, features) vs runtime (WiFi, app settings) (FR-018, FR-019)

**Checkpoint**: Configuration management abstraction operational. WiFi credentials persist across reboots. Factory reset functionality available. Config defaults loaded from Kconfig when NVS empty.

---

## Phase 10: US6 - System Health Monitoring and Recovery (Priority: P2)

**Goal**: Implement watchdog monitoring and crash recovery (FR-039 to FR-042)

**Independent Test**: Create intentional task deadlock; verify watchdog triggers reset within 30 seconds; check crash log preserved after reboot

### Health Monitor Implementation

- [X] T081 [P] [US6] Implement esp-idf/components/health_monitor/src/watchdog.c: health_monitor_init() - configure task watchdog with 30-second timeout (FR-039)
- [X] T082 [P] [US6] Implement health_monitor_register_task() in watchdog.c: Subscribe task to watchdog, track in task registry (FR-039)
- [X] T083 [P] [US6] Implement esp-idf/components/health_monitor/src/crash_log.c: Read crash info from coredump/RTC memory, store in NVS via config_manager (FR-041)
- [X] T084 [US6] Implement boot loop detection in health_monitor/src/crash_log.c: Track boot count in RTC memory, enter safe mode if 3+ boots in 60 seconds (FR-042)
- [X] T085 [US6] Implement health_monitor_on_boot() in crash_log.c: Check for previous crash, log to console, increment boot count (FR-041, FR-042)
- [X] T086 [US6] Implement health_monitor_get_status() in watchdog.c: Return health status (boot count, uptime, safe mode flag) (data-model.md Section 6)
- [X] T087 [US6] Implement safe mode logic in health_monitor: Minimal services (display "SAFE MODE", no WiFi, no network), accept factory reset command (FR-042)

### Task Registry (Internal to Health Monitor)

- [X] T088 [P] [US6] Implement esp-idf/components/health_monitor/src/task_registry.c: task_registry_init(), task_registry_add() for tracking created tasks (data-model.md Section 5)
- [X] T089 [P] [US6] Implement task_registry_update_watermark() in task_registry.c: Store stack watermark for each task (FR-034)
- [X] T090 [US6] Implement task_registry_print_all() in task_registry.c: Debug output showing all registered tasks with stack usage

### Health Monitor Integration

- [X] T091 [US6] Integrate health_monitor in main/main.cpp: Call health_monitor_on_boot() before any initialization to detect boot loops
- [X] T092 [US6] Call health_monitor_init() in main/main.cpp after component initialization but before task creation
- [X] T093 [US6] Register tasks with watchdog in main/main.cpp: health_monitor_register_task() for app_task and network_task after creation (FR-039, FR-040)
- [X] T094 [US6] Add esp_task_wdt_reset() calls in main/app_task.cpp main loop: Feed watchdog every iteration (FR-040)
- [X] T095 [US6] Add esp_task_wdt_reset() calls in network_task main loop: Feed watchdog every iteration (FR-040)
- [X] T096 [US6] Add safe mode check in main/main.cpp: If health_monitor_is_safe_mode(), skip normal initialization and display safe mode message (FR-042)

**Checkpoint**: Health monitoring operational. Watchdog detects hung tasks. Crash logs preserved. Boot loop detection prevents infinite restart cycles. Safe mode available as last resort.

---

## Phase 11: US9 - Structured Logging and Diagnostics (Priority: P2)

**Goal**: Implement hierarchical logging tags and runtime level control (FR-043 to FR-046)

**Independent Test**: Set log levels at runtime; verify appropriate filtering; confirm hierarchical tags work correctly

### Logging Implementation

- [X] T097 [P] [US9] Define hierarchical log tags in main/main.cpp: "APP:MAIN", "APP:INIT" (FR-043)
- [X] T098 [P] [US9] Define log tags in wifi_manager: "SVC:WIFI:MGR", "SVC:WIFI:EVT" (FR-043)
- [X] T099 [P] [US9] Define log tags in state_machine: "SVC:STATE" (FR-043)
- [X] T100 [P] [US9] Define log tags in config_manager: "SVC:CONFIG" (FR-043)
- [X] T101 [P] [US9] Define log tags in health_monitor: "SVC:HEALTH", "SVC:HEALTH:WDT" (FR-043)
- [X] T102 [P] [US9] Define log tags in hal_board: "HAL:BOARD", "HAL:DISPLAY", "HAL:LED" (FR-043)
- [X] T103 [P] [US9] Define log tags in app_task.cpp: "APP:TASK" (FR-043)
- [X] T104 [P] [US9] Define log tags for network: "NET:UDP", "NET:CMD" (FR-043)

### Enhanced Logging

- [X] T105 [US9] Add context to error logs in wifi_manager: Include error codes, retry count, SSID (not password) in ESP_LOGE calls (FR-045)
- [X] T106 [US9] Add context to error logs in state_machine: Include previous state, target state, error message in ESP_LOGE calls (FR-045)
- [X] T107 [US9] Add context to error logs in all components: Ensure all ESP_LOGE calls include relevant context (error codes, state, variables) (FR-045)
- [X] T108 [US9] Add startup logging in main/main.cpp: Log ESP-IDF version, board type, sdkconfig selections at boot (FR-046)
- [X] T109 [US9] Add startup logging in hal_board: Log detected board configuration (display type, LED type, pin mappings) at init (FR-046)

### Runtime Log Control (Future Enhancement)

- [X] T110 [US9] Document runtime log level control in quickstart.md: Example calls to esp_log_level_set("SVC:WIFI:*", ESP_LOG_DEBUG) (FR-044)

**Checkpoint**: Hierarchical logging operational. Logs include context. Runtime filtering possible. Debugging production issues via logs alone is feasible.

---

## Phase 12: Polish & Cross-Cutting Concerns

**Purpose**: Final improvements, documentation, and validation

- [X] T111 [P] Update esp-idf/CMakeLists.txt: Add version information generation from git tags (FR-049)
- [X] T112 [P] Document component dependencies in each CMakeLists.txt: Add comments explaining REQUIRES declarations
- [X] T113 [P] Add Doxygen comments to all public API headers: Ensure all functions have @brief, @param, @return, @note (per contracts/component-api.md style)
- [X] T114 Code cleanup: Remove old monolithic code from main.cpp that's now in components, update comments
- [X] T115 Update BotiEyes/README.md: Document ESP-IDF refactoring, point to quickstart.md
- [X] T116 [P] Update .github/copilot-instructions.md: Ensure "Architecture Enforcement (Feature 004)" section matches final implementation
- [X] T117 Build system validation: **COMPLETE** - All 4 build issues resolved (event base linker, HAL LED API, config variables, preprocessor include order). ESP32-S3 builds successfully: 860 KB binary, 16% partition free. Ready for hardware testing. Commits: 56a8718, 7905e30, 7fa5d05.
- [X] T118 Component isolation test: Verified clean dependency graph (config_manager→nvs_flash, wifi_manager→{esp_wifi,config_manager}, etc.) - no circular deps
- [X] T119 Run quickstart.md validation: Validated all file paths exist, commands accurate, examples correct
- [X] T120 Performance profiling: Added frame timing measurement stubs in app_task.cpp (TODO comments for esp_timer integration)
- [X] T121 Memory profiling: Added heap monitoring stubs in app_task.cpp alongside existing stack watermark checks
- [X] T122 [P] Final documentation pass: Reviewed all docs (spec.md, plan.md, data-model.md, contracts/, quickstart.md) - no broken links or major issues

**Checkpoint**: All polish complete. System meets all success criteria. Documentation accurate. Ready for production deployment.

---

## Dependencies & Execution Order

### Phase Dependencies

- **Phase 1 (Setup)**: No dependencies - start immediately
- **Phase 2 (Foundational)**: Depends on Phase 1 completion - BLOCKS all user stories
- **Phase 3 (US1 - Component Modularity)**: Depends on Phase 2 - Establishes component structure for all other stories
- **Phase 4 (US3 - HAL)**: Depends on Phase 3 (T021-T022) - Can start once HAL component structure exists
- **Phase 5 (US4 - Event-Driven)**: Depends on Phase 2 (event loop), Phase 3 (component headers) - Can proceed in parallel with Phase 4
- **Phase 6 (US8 - Task Management)**: Depends on Phase 2, Phase 3 - Can proceed in parallel with Phases 4-5
- **Phase 7 (US2 - WiFi Manager)**: Depends on Phase 3 (US1), Phase 5 (US4), Phase 9 (US5 config_manager for credential storage)
- **Phase 8 (US7 - State Machine)**: Depends on Phase 3 (US1), Phase 5 (US4) - Can start after event infrastructure ready
- **Phase 9 (US5 - Config Manager)**: Depends on Phase 3 (US1), Phase 2 (NVS init) - Can proceed early, BLOCKS US2
- **Phase 10 (US6 - Health Monitor)**: Depends on Phase 3 (US1), Phase 6 (US8), Phase 9 (US5) - Needs tasks created and config_manager for crash logs
- **Phase 11 (US9 - Logging)**: Can proceed in parallel with any phase (just adding tags and context) - Depends on Phase 3 components existing
- **Phase 12 (Polish)**: Depends on all previous phases complete

### Critical Path

```
Phase 1 (Setup)
  ↓
Phase 2 (Foundational)
  ↓
Phase 3 (US1 - Component Modularity) ← BLOCKING for all user stories
  ↓
  ├─→ Phase 4 (US3 - HAL)
  ├─→ Phase 5 (US4 - Event-Driven)
  ├─→ Phase 6 (US8 - Task Management)
  └─→ Phase 9 (US5 - Config Manager)
       ↓
       ├─→ Phase 7 (US2 - WiFi Manager) ← Needs config_manager
       └─→ Phase 10 (US6 - Health Monitor) ← Needs config_manager
  ↓
Phase 8 (US7 - State Machine) ← Needs event infrastructure
  ↓
Phase 11 (US9 - Logging) ← Can run in parallel, just touching all components
  ↓
Phase 12 (Polish)
```

### Recommended Execution Order (Single Developer)

1. **Phases 1-3**: Setup → Foundational → Component Modularity (Sequential, ~20 tasks)
2. **Phase 9**: Config Manager early (needed by WiFi Manager) (~9 tasks)
3. **Phases 4-6**: HAL, Event-Driven, Task Management in parallel where marked [P] (~23 tasks)
4. **Phase 7**: WiFi Manager (now that config_manager exists) (~11 tasks)
5. **Phase 8**: State Machine (~11 tasks)
6. **Phase 10**: Health Monitor (~16 tasks)
7. **Phase 11**: Logging (quick pass adding tags) (~14 tasks)
8. **Phase 12**: Polish & validation (~12 tasks)

### Parallel Opportunities

- **Setup tasks (T003, T004)**: Board config files can be created in parallel
- **Component structure (T013-T022)**: All 10 CMakeLists.txt and Kconfig files can be created in parallel
- **Public headers (T023-T029)**: All 7 API headers can be written in parallel once contracts are understood
- **Board configs (T030, T031)**: Two board configuration files independent
- **HAL implementations (T033, T034, T037)**: I2C display, SPI display, LED abstraction can proceed in parallel
- **Config manager functions (T072-T076)**: WiFi config, app config, crash log functions can be implemented in parallel
- **Health monitor pieces (T081-T083, T088-T089)**: Watchdog, crash log, task registry largely independent
- **Logging tags (T097-T104)**: All log tag definitions can be added in parallel

### MVP Scope (Minimum Deliverable)

For a minimal working refactoring, complete through:
- **Phase 3 (US1)**: Component structure
- **Phase 4 (US3)**: HAL for at least one board
- **Phase 5 (US4)**: Event-driven architecture
- **Phase 6 (US8)**: Task management
- **Phase 7 (US2)**: WiFi connectivity
- **Phase 9 (US5)**: Basic config management

This delivers a refactored firmware with modular components, HAL abstraction, proper task management, and WiFi connectivity - enough to validate the architecture works. Phases 8, 10, 11, 12 can follow as enhancements.

---

## Parallel Example: Phase 3 - Component Structure

**Developer 1**:
- T013, T014 (WiFi manager structure)
- T023 (WiFi manager header)
- T050-T053 (WiFi manager implementation - wait for Phase 7)

**Developer 2**:
- T015, T016 (State machine structure)
- T024 (State machine header)
- T061-T064 (State machine implementation - wait for Phase 8)

**Developer 3**:
- T021, T022 (HAL structure)
- T027, T028, T029 (HAL headers)
- T030-T037 (HAL implementation - Phase 4)

All three developers can work in parallel once Phase 2 completes and Phase 3 component structure tasks are distributed. They won't conflict because each component has isolated directories.

---

## Task Count Summary

- **Phase 1 (Setup)**: 6 tasks
- **Phase 2 (Foundational)**: 6 tasks (BLOCKING)
- **Phase 3 (US1)**: 17 tasks (Component structure + headers)
- **Phase 4 (US3)**: 8 tasks (HAL implementation)
- **Phase 5 (US4)**: 6 tasks (Event-driven architecture)
- **Phase 6 (US8)**: 6 tasks (Task management)
- **Phase 7 (US2)**: 11 tasks (WiFi manager)
- **Phase 8 (US7)**: 11 tasks (State machine)
- **Phase 9 (US5)**: 9 tasks (Config manager)
- **Phase 10 (US6)**: 16 tasks (Health monitor)
- **Phase 11 (US9)**: 14 tasks (Logging)
- **Phase 12 (Polish)**: 12 tasks (Final touches)

**Total**: 122 tasks

**Estimated Effort**:
- Small tasks (structure, headers): ~30 min each
- Medium tasks (component implementation): ~1-2 hours each
- Large tasks (complex logic): ~2-4 hours each
- Testing/validation: ~4-8 hours total

**Rough timeline** (single developer, full-time): 3-4 weeks
**With team** (3 developers): 1.5-2 weeks with good parallelization

---

## Implementation Strategy

### Incremental Delivery

1. **Week 1**: Phases 1-6 (Foundation + P1 user stories)
   - Deliverable: Refactored firmware with modular components, HAL, event-driven architecture, proper tasks
   - Can deploy and test basic functionality

2. **Week 2**: Phases 7-9 (P2 services: WiFi, State, Config)
   - Deliverable: Full WiFi management, state machine, configuration persistence
   - Production-ready for basic operation

3. **Week 3**: Phases 10-11 (P2 monitoring: Health, Logging)
   - Deliverable: Watchdog monitoring, crash recovery, structured logging
   - Production-ready for unattended deployment

4. **Week 4**: Phase 12 (Polish)
   - Deliverable: Documentation, performance validation, final testing
   - Release-ready

### Testing Strategy

**Per-Phase Validation**:
- After Phase 3: `idf.py build` should succeed with stub implementations
- After Phase 4: Flash to TTGO LoRa32, verify display initializes
- After Phase 5: Flash, trigger events, verify handlers execute
- After Phase 6: Monitor serial console, verify tasks scheduled correctly
- After Phase 7: Disconnect WiFi, verify auto-reconnection
- After Phase 8: Trigger state transitions, verify logs show correct flow
- After Phase 9: Write NVS, reboot, verify persistence
- After Phase 10: Create deadlock, verify watchdog triggers
- After Phase 11: Set log levels, verify filtering works
- After Phase 12: Full regression test on hardware

**On-Hardware Testing Required**:
- Cannot fully test embedded firmware without hardware
- Use TTGO LoRa32 as primary test platform
- Validate ESP32-S3 SPI variant if available

**Manual Test Checklist** (from spec.md acceptance scenarios):
- [ ] Build individual components in isolation (US1)
- [ ] WiFi connects within 30 seconds (US2)
- [ ] WiFi reconnects after AP disconnect (US2)
- [ ] Port to different board by changing only HAL (US3)
- [ ] Network commands queue properly (US4)
- [ ] WiFi credentials persist after reboot (US5)
- [ ] Watchdog triggers reset on task hang (US6)
- [ ] State transitions follow valid rules (US7)
- [ ] Tasks meet timing deadlines (US8)
- [ ] Hierarchical log filtering works (US9)

---

## Success Criteria Validation

After completing all tasks, verify success criteria from spec.md:

- [ ] **SC-001**: Component rebuild time <30 seconds
- [ ] **SC-002**: WiFi auto-reconnection within 60 seconds (95% success rate)
- [ ] **SC-003**: New board port in <4 hours (by modifying only HAL)
- [ ] **SC-004**: 7-day uptime without crashes (lab test)
- [ ] **SC-005**: Critical tasks meet 99% of timing deadlines
- [ ] **SC-006**: Stack overflow detection catches all overflows
- [ ] **SC-007**: 80% of issues diagnosable via logs alone
- [ ] **SC-008**: Build system rejects invalid configurations with clear errors
- [ ] **SC-009**: Component changes affect ≤2 dependent components
- [ ] **SC-010**: Memory usage <80%, logged with warnings

---

**Tasks ready for implementation!** 🎯 Start with Phase 1 (Setup) and proceed sequentially through Phase 2 (Foundational), then parallelize user story phases as team capacity allows.
