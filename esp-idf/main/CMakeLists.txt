idf_component_register(
    SRCS "app_task.cpp" "main.cpp"
    PRIV_REQUIRES 
        led_strip 
        botieyes 
        driver 
        esp_driver_gpio 
        esp_wifi 
        nvs_flash
        esp_event
        esp_netif
        wifi_manager
        state_machine
        config_manager
        health_monitor
        hal_board
        display
    INCLUDE_DIRS "."
)
