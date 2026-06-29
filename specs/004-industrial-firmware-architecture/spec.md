# Feature Specification: Industrial Firmware Architecture Refactoring

**Feature Branch**: `004-industrial-firmware-architecture`

**Created**: 2026-06-29

**Status**: Draft

**Input**: User description: "Lets refactor the current project to attend the professional ESP-IDF industrial firmware architecture based on best practices."

## User Scenarios & Testing *(mandatory)*

### User Story 1 - Component Modularity and Maintainability (Priority: P1)

As a firmware developer, I need the codebase organized into clear, independent components with well-defined interfaces so that I can modify, test, and maintain individual subsystems without affecting others.

**Why this priority**: Foundation for all other improvements. Without proper modularity, implementing other features becomes exponentially harder. This delivers immediate value by making the codebase navigable and maintainable.

**Independent Test**: Can be fully tested by attempting to build individual components in isolation and verifying clean compilation without circular dependencies. Delivers immediate value through improved code navigation and reduced cognitive load.

**Acceptance Scenarios**:

1. **Given** the refactored component structure, **When** a developer modifies the WiFi manager component, **Then** no changes are required to the display or application components
2. **Given** the new component structure, **When** running the build system, **Then** each component compiles independently with clear dependency declarations
3. **Given** the layered architecture, **When** viewing any component's public header, **Then** the interface is documented and implementation details are hidden

---

### User Story 2 - Robust WiFi Management (Priority: P2)

As a device operator, I need reliable WiFi connectivity with automatic reconnection and clear status reporting so that the device maintains network functionality without manual intervention.

**Why this priority**: Core functionality for network-controlled devices. Without reliable WiFi, the network control features are unusable. Essential for production deployments.

**Independent Test**: Can be tested by deploying the device, disconnecting WiFi, and verifying automatic reconnection within configured timeout. Status LED and logs confirm connection state changes.

**Acceptance Scenarios**:

1. **Given** the device is powered on, **When** WiFi credentials are configured, **Then** the device connects automatically within 30 seconds and indicates status via LED
2. **Given** the device is connected to WiFi, **When** the access point becomes unavailable, **Then** the device attempts reconnection with exponential backoff for up to 5 minutes
3. **Given** the device has failed WiFi connection, **When** a valid access point reappears, **Then** the device reconnects automatically and resumes network services
4. **Given** runtime WiFi operation, **When** requesting connection status, **Then** the system reports IP address, signal strength, and connection duration

---

### User Story 3 - Hardware Abstraction and Portability (Priority: P1)

As a firmware developer, I need a hardware abstraction layer (HAL) that isolates board-specific code so that I can port the firmware to different hardware variants without modifying application logic.

**Why this priority**: Critical for product scalability. Different hardware revisions or product variants must use the same application code. HAL enables testing on different boards and future-proofs the design.

**Independent Test**: Can be tested by defining a second board configuration and verifying that only HAL files need changes while application components remain untouched.

**Acceptance Scenarios**:

1. **Given** the HAL layer is implemented, **When** porting to a different display interface (I2C vs SPI), **Then** only the HAL component needs modification
2. **Given** multiple board configurations, **When** building for a different target, **Then** the HAL selects correct pin mappings and initialization sequences automatically
3. **Given** the application code, **When** examining any non-HAL component, **Then** no direct GPIO pin numbers or hardware-specific constants are visible

---

### User Story 4 - Event-Driven Architecture and Inter-Task Communication (Priority: P1)

As a firmware developer, I need proper event-driven architecture using ESP-IDF events and FreeRTOS queues so that tasks communicate efficiently, system events are handled consistently, and the architecture scales without tight coupling.

**Why this priority**: Foundation for scalable, maintainable multi-task systems. Without proper event/queue patterns, code becomes tightly coupled with direct function calls and shared state, leading to race conditions and maintenance issues. This is a core ESP-IDF pattern.

**Independent Test**: Can be tested by triggering WiFi events, network commands, and application events while monitoring that appropriate handlers execute and tasks communicate through queues without blocking or data loss.

**Acceptance Scenarios**:

1. **Given** the ESP-IDF event loop is initialized, **When** WiFi connects or disconnects, **Then** registered event handlers execute and appropriate tasks are notified via event groups
2. **Given** network commands arrive via UDP, **When** the network task receives them, **Then** commands are queued to the application task without blocking the network receive
3. **Given** multiple tasks need to communicate, **When** one task sends data to another, **Then** FreeRTOS queues are used with proper timeout handling and no busy-waiting
4. **Given** application-level events (e.g., state changes, errors), **When** these events occur, **Then** they are posted to the ESP event loop and handled by registered callbacks
5. **Given** synchronization is needed between tasks, **When** waiting for conditions, **Then** FreeRTOS event groups are used instead of polling or blocking delays

---
6
### User Story 5 - Configuration Management (Priority: P2)

As a product engineer, I need clear separation between build-time configuration (board variants, feature flags) and runtime configuration (WiFi credentials, operational parameters) so that I can ship firmware that adapts to different deployment scenarios without recompilation.

**Why this priority**: Essential for production deployment flexibility. Allows same firmware binary to be configured for different environments through runtime settings rather than requiring separate builds.

**Independent Test**: Can be tested by loading firmware, changing WiFi credentials via NVS, rebooting, and verifying the new settings are applied without reflashing.

**Acceptance Scenarios**:

1. **Given** the Kconfig system, **When** building firmware, **Then** developers can select board type, display interface, and optional features through menuconfig
2. **Given** deployed firmware, **When** writing WiFi credentials to NVS, **Then** the device uses new credentials after reboot without reflashing
3. **Given** runtime configuration, **When** factory reset is triggered, **Then** all user settings revert to defaults while firmware remains unchanged
4. **Given** configuration defaults, **When** building for a new board variant, **Then** appropriate defaults are selected via sdkconfig.defaults.{board}

---

### User Story 6 - System Health Monitoring and Recovery (Priority: P2)

As a device operator, I need automatic watchdog monitoring and crash recovery so that the device self-recovers from software faults and maintains uptime in production environments.

**Why this priority**: Critical for unattended operation. Prevents device lockups from requiring manual intervention. Essential for deployment reliability.

**Independent Test**: Can be tested by intentionally creating a task deadlock and verifying the watchdog triggers reset within configured timeout, followed by successful reboot and operation.

**Acceptance Scenarios**:

1. **Given** the system is running, **When** a task stops responding for more than 30 seconds, **Then** the watchdog triggers a restart with logged fault information
2. **Given** a system crash occurs, **When** the device reboots, **Then** crash logs are preserved in NVS and accessible for diagnostics
3. **Given** watchdog monitoring is active, **When** all tasks are healthy, **Then** watchdog is fed regularly and no false resets occur
4. **Given** a boot loop scenario, **When** three consecutive crashes occur within 1 minute, **Then** the system enters safe mode with minimal services

---

### User Story 7 - State Machine and Application Flow (Priority: P2)

As a firmware developer, I need a centralized state machine managing system lifecycle so that application behavior is predictable and state transitions are properly sequenced and logged.

**Why this priority**: Prevents undefined behavior and race conditions. Makes debugging significantly easier. Essential for understanding system behavior in production.

**Independent Test**: Can be tested by triggering various state transitions (boot, WiFi connect, network control, error, shutdown) and verifying logs show correct state flow and all transitions are valid.

**Acceptance Scenarios**:

1. **Given** the system state machine, **When** the device boots, **Then** states progress through INIT → CONNECTING → CONNECTED → RUNNING in order
2. **Given** a running system, **When** a critical error occurs, **Then** state transitions to ERROR and error handling procedures execute
3. **Given** any state, **When** examining system status, **Then** current state, previous state, and transition timestamp are available
4. **Given** state transitions, **When** any transition occurs, **Then** the transition is logged with context information

---

### User Story 8 - Professional Task Management (Priority: P1)

As a firmware developer, I need properly structured FreeRTOS tasks with clear priorities, stack allocations, and core affinity so that real-time requirements are met and system resources are efficiently utilized.

**Why this priority**: Foundation for reliable multi-tasking operation. Improper task management causes crashes and performance issues. Must be correct from the start.

**Independent Test**: Can be tested by monitoring task stack watermarks and CPU usage, verifying no stack overflows occur and critical tasks meet timing requirements under load.

**Acceptance Scenarios**:

1. **Given** the task structure, **When** system is running, **Then** critical tasks (sensor reading, network handling) meet their timing deadlines
2. **Given** FreeRTOS operation, **When** monitoring stack usage, **Then** no task exceeds 80% of allocated stack under normal operation
3. **Given** dual-core ESP32, **When** tasks are created, **Then** compute-intensive tasks (rendering) are pinned to core 1 and network tasks to core 0
4. **Given** task priorities, **When** multiple tasks are runnable, **Then** higher priority tasks preempt lower priority tasks as expected

---

### User Story 9 - Structured Logging and Diagnostics (Priority: P2)

As a developer or operator, I need structured, hierarchical logging with configurable levels so that I can diagnose issues efficiently without being overwhelmed by irrelevant information.

**Why this priority**: Essential for debugging production issues. Poor logging makes problems nearly impossible to diagnose remotely.

**Independent Test**: Can be tested by setting different log levels and verifying appropriate messages appear, and that hierarchical filtering works (e.g., setting ESP:WIFI:* to DEBUG shows only WiFi-related debug messages).

**Acceptance Scenarios**:

1. **Given** the logging system, **When** examining logs, **Then** each message includes timestamp, component tag, log level, and message
2. **Given** hierarchical tags like "APP:WIFI:MGR", **When** setting log level for "APP:WIFI:*", **Then** all WiFi-related components respect the level
3. **Given** production operation, **When** errors occur, **Then** ERROR logs include context (error codes, state, relevant variables)
4. **Given** development builds, **When** enabling DEBUG level, **Then** detailed execution flow is visible without recompiling

---

### Edge Cases

- What happens when NVS is corrupted or full? (System should erase and reinitialize with factory defaults)
- How does the system handle task stack overflow? (Watchdog should detect and restart with logging)
- How does WiFi manager handle rapidly changing network conditions? (Implements exponential backoff to prevent connection thrashing)
- What happens when a queue becomes full during high load? (Sender should timeout gracefully or oldest messages dropped depending on queue type)
- How does the system handle event handler registration failures? (Log error, fall back to polling if critical)
- What happens when multiple tasks try to access the display simultaneously? (Mutex protection ensures exclusive access, with reasonable timeout)
- How does the system behave when heap memory is exhausted? (Critical allocations are checked and system degrades gracefully or restarts)
- What happens when both I2C and SPI display configurations are accidentally enabled? (Build system should error with clear message, not compile invalid configuration)
- How does the system handle corrupt configuration in NVS? (Detect corruption, log error, fall back to compiled defaults)
- What happens when WiFi credentials are invalid or AP is permanently unavailable? (System continues operation in offline mode, periodic retry with backoff)

## Requirements *(mandatory)*

### Functional Requirements

#### Architecture & Structure

- **FR-001**: System MUST organize code into layered components: Application Layer → Service Layer → HAL → Driver Layer → ESP-IDF
- **FR-002**: Each component MUST have clearly defined interfaces with public headers in include/ and private implementation in src/
- **FR-003**: Component dependencies MUST be declared explicitly in CMakeLists.txt with no circular dependencies
- **FR-004**: System MUST separate board-specific code (HAL) from application logic to enable multi-board support

#### Inter-Task Communication & Synchronization

**Use the right tool for the job:**

- **FR-005**: ESP event loop (`esp_event`) MUST be used for system-wide events that multiple components need to observe (WiFi events, IP events, system state changes)
- **FR-006**: FreeRTOS queues MUST be used for passing data between tasks (network packets, commands, sensor readings)
- **FR-007**: Mutexes MUST be used to protect shared resources accessed by multiple tasks (display hardware, state machine, NVS)
- **FR-008**: Binary semaphores or task notifications SHOULD be used for simple task-to-task signaling (work ready, operation complete)
- **FR-009**: Event groups MAY be used when a task needs to wait on multiple conditions simultaneously (e.g., WiFi connected AND display initialized AND config loaded)
- **FR-010**: Direct function calls SHOULD be used within the same component when no task synchronization is needed
- **FR-011**: Busy-waiting and polling loops are PROHIBITED except for hardware register polling with timeouts

**Specific patterns:**

- **FR-012**: WiFi/IP events MUST use ESP event loop, not custom callbacks
- **FR-013**: Network receive callbacks MUST queue data for processing in application task, not process inline
- **FR-014**: Display access MUST be protected by mutex to prevent concurrent writes
- **FR-015**: State machine access MUST be protected by mutex with transitions validated atomically
- **FR-016**: Queue send operations MUST use reasonable timeouts (100-1000ms), never portMAX_DELAY in time-critical tasks
- **FR-017**: Mutex/semaphore operations MUST include timeout handling and log failures

#### Configuration Management

- **FR-018**: System MUST provide Kconfig-based build-time configuration for board type, display interface, optional features, and log levels
- **FR-019**: System MUST support runtime configuration storage in NVS for WiFi credentials, operational parameters, and user preferences
- **FR-020**: System MUST provide sdkconfig.defaults files for each supported board variant (e.g., sdkconfig.defaults.ttgo_lora32)
- **FR-021**: System MUST support factory reset functionality that clears NVS user settings while preserving firmware

#### WiFi Management

- **FR-022**: WiFi manager MUST register handlers with ESP event loop for WIFI_EVENT and IP_EVENT
- **FR-023**: WiFi manager MUST retry connection at least 5 times with exponential backoff before reporting permanent failure
- **FR-024**: WiFi manager MUST provide synchronous API for querying connection state (IP address, signal strength, connection status)
- **FR-025**: WiFi manager MUST post custom application events for high-level state changes (WIFI_MGR_CONNECTED, WIFI_MGR_FAILED)

#### State Management

- **FR-026**: System MUST implement centralized state machine with defined states: INIT, CONNECTING, CONNECTED, RUNNING, ERROR, SAFE_MODE, SHUTDOWN
- **FR-027**: State machine MUST validate all state transitions and reject invalid transitions with logging
- **FR-028**: State machine MUST use mutex protection for thread-safe access
- **FR-029**: State machine MUST post state change events to ESP event loop for observers
- **FR-030**: State machine MUST provide synchronous query API (get_current_state) that does not block

#### Task Management

- **FR-031**: System MUST create FreeRTOS tasks with explicit priorities: CRITICAL (configMAX_PRIORITIES-1), HIGH, NORMAL, LOW
- **FR-032**: System MUST allocate appropriate stack sizes: LARGE (8KB), MEDIUM (4KB), SMALL (2KB) based on measured usage + 25% margin
- **FR-033**: On dual-core systems, tasks MUST be pinned to appropriate cores: network/WiFi to core 0, rendering/compute to core 1
- **FR-034**: Task stack watermarks MUST be monitored and logged periodically to detect near-overflow conditions

#### Hardware Abstraction Layer

- **FR-035**: HAL MUST provide board initialization function that configures all GPIO pins, peripherals, and interfaces
- **FR-036**: HAL MUST abstract display interface, supporting both I2C and SPI variants through same API
- **FR-037**: HAL MUST abstract status LED control, supporting different LED types (GPIO, WS2812, etc.) or disabling when not present
- **FR-038**: HAL MUST provide board-specific pin mapping configuration selectable at build time

#### System Health & Recovery

- **FR-039**: System MUST configure task watchdog with 30-second timeout for critical tasks
- **FR-040**: System MUST feed watchdog regularly in all long-running tasks to prove liveness
- **FR-041**: System MUST log watchdog triggers, task stack overflows, and crashes to NVS for post-mortem analysis
- **FR-042**: System MUST implement boot loop detection (3+ crashes in 60 seconds) and enter safe mode if detected

#### Logging & Diagnostics

- **FR-043**: System MUST use hierarchical logging tags in format "LAYER:COMPONENT:MODULE" (e.g., "SVC:WIFI:MGR")
- **FR-044**: System MUST support runtime log level configuration per-component via esp_log_level_set()
- **FR-045**: System MUST log errors with full context including error codes, state, and relevant parameters
- **FR-046**: System MUST log startup sequence, version information, and board configuration on boot

#### Build System & Tooling

- **FR-047**: Project MUST use standard ESP-IDF component structure with proper CMakeLists.txt in each component
- **FR-048**: Build system MUST enforce component isolation (no cross-component includes except through public headers)
- **FR-049**: Build system MUST generate version information from git tags and embed in firmware binary
- **FR-050**: System MUST support build-time selection of display interface (I2C or SPI) through Kconfig

### Key Entities *(include if feature involves data)*

- **WiFi Manager Service**: Manages WiFi connectivity lifecycle, credentials storage (NVS), connection state, auto-reconnection logic; registers with ESP event loop and posts high-level application events
- **State Machine**: Maintains system state, validates transitions with mutex protection, provides thread-safe query API, posts state change events to ESP event loop
- **HAL Configuration**: Board-specific structure defining GPIO pin mappings, peripheral configurations, display interface type, LED type, and initialization sequences  
- **Config Manager**: Abstracts NVS operations for storing/retrieving WiFi credentials, user preferences, operational parameters, and crash logs
- **Task Registry**: Tracks created tasks with handle, priority, stack size, core affinity, and stack watermark for monitoring
- **Watchdog Monitor**: Manages task watchdog subscriptions, timeout configurations, and recovery actions on watchdog triggers
- **Display Driver**: Abstracts SSD1306 display operations (I2C or SPI mode) with mutex-protected access and Adafruit_GFX compatibility
- **Command Queue**: FreeRTOS queue carrying network control commands from network task to application task (defined message structure, 10 item depth)
- **Display Mutex**: Protects display hardware from concurrent access by multiple tasks
- **State Mutex**: Protects state machine data structure during query and transition operations

## Success Criteria *(mandatory)*

### Measurable Outcomes

- **SC-001**: Developers can modify any single component and rebuild in under 30 seconds due to improved modularity
- **SC-002**: System automatically recovers from WiFi disconnection within 60 seconds without manual intervention in 95% of cases
- **SC-003**: Firmware can be ported to a new board variant by modifying only HAL files (target: under 4 hours of effort)
- **SC-004**: System uptime exceeds 7 days of continuous operation without watchdog resets or crashes in lab testing
- **SC-005**: Critical tasks meet 99% of timing deadlines under normal operational load
- **SC-006**: Stack overflow detection catches and logs all stack overflow scenarios before system corruption
- **SC-007**: Developers can diagnose production issues using logs alone in 80% of cases without requiring additional instrumentation
- **SC-008**: Build system detects and rejects invalid configurations (e.g., conflicting interfaces) with clear error messages before compilation
- **SC-009**: Component interfaces are stable enough that changes to one component require recompilation of at most 2 dependent components
- **SC-010**: Memory usage (heap and stack) is tracked and logged, with warnings when approaching limits (>80% utilization)

## Assumptions

- Current firmware is based on ESP-IDF framework and targets ESP32/ESP32-S3 series MCUs
- FreeRTOS is available and used for task management
- The BotiEyes library core logic (emotion rendering, eye animation) is sound and does not require refactoring
- Development targets the TTGO LoRa32 board as primary hardware platform with SSD1306 display
- WiFi infrastructure (access points) uses WPA2 security; WEP and open networks are not primary targets
- Current toolchain is ESP-IDF v5.0 or later with CMake-based build system
- Developers have access to ESP-IDF development environment and standard debugging tools (JTAG, serial console)
- The network control protocol (UDP-based) is working and does not require changes; only architectural structure changes needed
- Existing display initialization code (esp_ssd1306, display_init) provides working SPI/I2C drivers that can be abstracted
- Development focuses on single-language codebase (C/C++ for ESP-IDF); Python emulator remains separate and unchanged
- Initial deployments are USB-powered; battery operation and power optimization are future considerations
- The project maintains compatibility with Arduino IDE library structure for the BotiEyes component
- OTA updates and remote firmware management are out of scope for this refactoring; focus is on architectural improvements to existing functionality