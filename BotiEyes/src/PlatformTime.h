#ifndef BOTIEYES_PLATFORM_TIME_H
#define BOTIEYES_PLATFORM_TIME_H

#include <stdint.h>

namespace BotiEyes {
namespace platform {

// Returns a monotonic millisecond tick for interpolation and timeouts.
uint32_t nowMs();

} // namespace platform
} // namespace BotiEyes

#endif // BOTIEYES_PLATFORM_TIME_H
