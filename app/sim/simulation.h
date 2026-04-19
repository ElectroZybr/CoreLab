#pragma once

#include <bit>
#include <cstddef>
#include <optional>
#include <array>
#include <vector>

#include "sim/Cache.h"
#include "sim/MemoryTransaction.h"
#include "sim/RAM.h"

namespace sim {
struct ByteLoadResult {
    Address address = 0;
    Address lineBaseAddress = 0;
    std::size_t cacheSlotIndex = 0;
    bool hit = false;
    bool ready = false;
    bool startedTransaction = false;
    std::optional<TransactionId> transactionId;
    std::optional<std::byte> value;
};

struct FloatLoadResult {
    Address address = 0;
    Address lineBaseAddress = 0;
    std::size_t cacheSlotIndex = 0;
    bool hit = false;
    bool ready = false;
    bool startedTransaction = false;
    std::optional<TransactionId> transactionId;
    std::optional<float> value;
};

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
    ByteLoadResult loadByte(Address address, MemoryTransactionDurations durations = {});
    FloatLoadResult loadFloat(Address address, MemoryTransactionDurations durations = {});

  private:
    void installCompletedTransactions();
    MemoryTransaction* findActiveLineFill(Address lineBaseAddress);
    const MemoryTransaction* findActiveLineFill(Address lineBaseAddress) const;

    TransactionId m_nextTransactionId = 1;
    Tick m_currentTick = 0;
    RAM m_ram;
    Cache m_cache;
    std::vector<MemoryTransaction> m_transactions;
};
} // namespace sim
