#pragma once

#include <cstddef>

#include "sim/RAM.h"

namespace sim {
class Simulation {
  public:
    explicit Simulation(std::size_t ramSizeInBytes);

    [[nodiscard]] RAM& getRam() {
        return m_ram;
    }
    [[nodiscard]] const RAM& getRam() const {
        return m_ram;
    }

  private:
    RAM m_ram;
};
} // namespace sim
