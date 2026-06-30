# State Machine Component
# Centralized application state management with validated transitions

# Component Dependencies (T112):
# - esp_event: Event loop for APP_STATE_EVENT propagation (state changes)
# - esp_system: ESP system APIs (logging, error types)

idf_component_register(
    SRCS 
        "src/app_state.c"
        "src/state_events.c"
    INCLUDE_DIRS 
        "include"
    REQUIRES 
        esp_event 
        esp_system
)
