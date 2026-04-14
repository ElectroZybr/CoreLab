#pragma once

#include <cstddef>
#include <vector>

#include "sim/Cache.h"
#include "sim/MemoryTransaction.h"
#include "sim/RAM.h"

namespace sim {
class Simulation {
  public:
    explicit Simulation(std::size_t ramSizeInBytes, std::size_t cacheSizeInBytes = 512);

    [[nodiscard]] RAM& getRam() {
        return m_ram;
    }
    [[nodiscard]] const RAM& getRam() const {
        return m_ram;
    }
    [[nodiscard]] Cache& getCache() {
        return m_cache;
    }
    [[nodiscard]] const Cache& getCache() const {
        return m_cache;
    }
    [[nodiscard]] Tick getCurrentTick() const {
        return m_currentTick;
    }
    [[nodiscard]] const std::vector<MemoryTransaction>& getTransactions() const {
        return m_transactions;
    }

    void advance(Tick ticks = 1);
    const MemoryTransaction& startLineFill(Address address, MemoryTransactionDurations durations = {});

  private:
    TransactionId m_nextTransactionId = 1;
    Tick m_currentTick = 0;
    RAM m_ram;
    Cache m_cache;
    std::vector<MemoryTransaction> m_transactions;
};
} // namespace sim
