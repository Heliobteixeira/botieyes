#ifndef BOTIEYES_PLATFORM_RANDOM_H
#define BOTIEYES_PLATFORM_RANDOM_H

#include <stdint.h>

namespace BotiEyes {
namespace platform {

// Returns a random integer in [minInclusive, maxExclusive).
int32_t randomRange(int32_t minInclusive, int32_t maxExclusive);

} // namespace platform
} // namespace BotiEyes

#endif // BOTIEYES_PLATFORM_RANDOM_H
