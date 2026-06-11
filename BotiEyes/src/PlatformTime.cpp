#include "PlatformTime.h"

#if defined(ESP_PLATFORM)
#include "esp_timer.h"
#elif defined(ARDUINO)
#include <Arduino.h>
#else
#include <chrono>
#endif

namespace BotiEyes {
namespace platform {

uint32_t nowMs() {
#if defined(ESP_PLATFORM)
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
#elif defined(ARDUINO)
    return millis();
#else
    using namespace std::chrono;
    const auto now = steady_clock::now().time_since_epoch();
    return (uint32_t)duration_cast<milliseconds>(now).count();
#endif
}

} // namespace platform
} // namespace BotiEyes
