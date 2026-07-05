# Health Monitor Component
# System health monitoring: watchdog management, crash recovery, boot loop detection

# Component Dependencies (T112):
# - esp_system: Task watchdog APIs, coredump info, reset reason detection
# - esp_timer: High-resolution timer for uptime tracking
# - config_manager: Crash log persistence in NVS
# - freertos: FreeRTOS task APIs (task registry, stack watermark monitoring)

idf_component_register(
    SRCS 
        "src/watchdog.c"
        "src/crash_log.c"
        "src/task_registry.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        esp_system
        esp_timer
        config_manager
        freertos
)
