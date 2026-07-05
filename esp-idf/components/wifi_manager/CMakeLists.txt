# WiFi Manager Component
# Manages WiFi connectivity lifecycle: connection, reconnection, status monitoring, event propagation

# Component Dependencies (T112):
# - esp_wifi: WiFi initialization and management APIs
# - esp_event: Event loop for WiFi/IP event propagation
# - nvs_flash: NVS storage initialization (transitive from config_manager)
# - config_manager: Credential persistence in NVS (get/set WiFi SSID/password)

idf_component_register(
    SRCS 
        "src/wifi_manager.c"
        "src/wifi_events.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        esp_wifi 
        esp_event 
        nvs_flash 
        config_manager
    PRIV_REQUIRES
        esp_netif
        lwip
)
