#pragma once

#include <cstddef>

namespace math {
constexpr std::size_t ceilDiv(std::size_t value, std::size_t divisor) {
    if (divisor == 0) {
        return 0;
    }

    return (value + divisor - 1) / divisor;
}
} // namespace math
