#include "PlatformRandom.h"

#if defined(ESP_PLATFORM)
#include "esp_random.h"
#elif defined(ARDUINO)
#include <Arduino.h>
#else
#include <cstdlib>
#endif

namespace BotiEyes {
namespace platform {

int32_t randomRange(int32_t minInclusive, int32_t maxExclusive) {
    if (maxExclusive <= minInclusive) {
        return minInclusive;
    }
    const uint32_t span = (uint32_t)(maxExclusive - minInclusive);

#if defined(ESP_PLATFORM)
    return minInclusive + (int32_t)(esp_random() % span);
#elif defined(ARDUINO)
    return (int32_t)random(minInclusive, maxExclusive);
#else
    return minInclusive + (int32_t)(std::rand() % span);
#endif
}

} // namespace platform
} // namespace BotiEyes
