#include "sim/simulation.h"

namespace sim {
Simulation::Simulation(std::size_t ramSizeInBytes, std::size_t cacheSizeInBytes)
    : m_ram(ramSizeInBytes), m_cache(cacheSizeInBytes) {
}

void Simulation::advance(Tick ticks) {
    m_currentTick += ticks;
}

const MemoryTransaction& Simulation::startLineFill(Address address, MemoryTransactionDurations durations) {
    const RamLineInfo ramLine = m_ram.getLineForAddress(address);

    m_transactions.emplace_back(m_nextTransactionId++,
                                MemoryTransactionKind::LineFill,
                                address,
                                ramLine.baseAddress,
                                ramLine.sizeInBytes,
                                m_cache.getTargetSlotIndex(address),
                                m_currentTick,
                                durations);

    return m_transactions.back();
}
} // namespace sim
