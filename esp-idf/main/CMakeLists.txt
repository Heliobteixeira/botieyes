idf_component_register(
    SRCS "main.cpp" "display_init.cpp" "wifi_init.cpp"
    PRIV_REQUIRES led_strip botieyes driver esp_driver_gpio esp_wifi nvs_flash
    INCLUDE_DIRS "."
)
