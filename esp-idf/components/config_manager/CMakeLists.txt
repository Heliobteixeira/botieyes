# Configuration Manager Component
# NVS abstraction for runtime configuration management

# Component Dependencies (T112):
# - nvs_flash: Non-volatile storage APIs for reading/writing configuration
#   (WiFi credentials, app settings, crash logs)

idf_component_register(
    SRCS 
        "src/config_manager.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        nvs_flash
)
