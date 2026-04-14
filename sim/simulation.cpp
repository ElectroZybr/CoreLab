#include "sim/simulation.h"

#include <stdexcept>

namespace sim {
Simulation::Simulation(std::size_t ramSizeInBytes, std::size_t cacheSizeInBytes)
    : m_ram(ramSizeInBytes), m_cache(cacheSizeInBytes) {
}

void Simulation::advance(Tick ticks) {
    m_currentTick += ticks;
    installCompletedTransactions();
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

ByteLoadResult Simulation::loadByte(Address address, MemoryTransactionDurations durations) {
    installCompletedTransactions();

    const RamLineInfo ramLine = m_ram.getLineForAddress(address);
    const std::size_t targetCacheSlotIndex = m_cache.getTargetSlotIndex(address);

    if (m_cache.contains(address)) {
        return {address,
                ramLine.baseAddress,
                targetCacheSlotIndex,
                true,
                true,
                false,
                std::nullopt,
                m_cache.readByte(address)};
    }

    if (MemoryTransaction* activeTransaction = findActiveLineFill(ramLine.baseAddress)) {
        return {address,
                ramLine.baseAddress,
                activeTransaction->getTargetCacheSlotIndex(),
                false,
                false,
                false,
                activeTransaction->getId(),
                std::nullopt};
    }

    const MemoryTransaction& transaction = startLineFill(address, durations);
    return {address,
            ramLine.baseAddress,
            transaction.getTargetCacheSlotIndex(),
            false,
            false,
            true,
            transaction.getId(),
            std::nullopt};
}

FloatLoadResult Simulation::loadFloat(Address address, MemoryTransactionDurations durations) {
    const Address lastByteAddress = address + RAM::kFloatSizeInBytes - 1;
    const RamLineInfo firstLine = m_ram.getLineForAddress(address);
    const RamLineInfo lastLine = m_ram.getLineForAddress(lastByteAddress);

    if (firstLine.baseAddress != lastLine.baseAddress) {
        throw std::invalid_argument("Float load across cache line boundary is not supported yet");
    }

    const ByteLoadResult firstByteLoad = loadByte(address, durations);
    if (!firstByteLoad.ready) {
        return {address,
                firstByteLoad.lineBaseAddress,
                firstByteLoad.cacheSlotIndex,
                firstByteLoad.hit,
                false,
                firstByteLoad.startedTransaction,
                firstByteLoad.transactionId,
                std::nullopt};
    }

    std::array<std::byte, RAM::kFloatSizeInBytes> bytes{};
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        bytes[index] = m_cache.readByte(address + index);
    }

    const std::uint32_t rawValue =
        static_cast<std::uint32_t>(bytes[0]) | (static_cast<std::uint32_t>(bytes[1]) << 8) |
        (static_cast<std::uint32_t>(bytes[2]) << 16) | (static_cast<std::uint32_t>(bytes[3]) << 24);

    return {address,
            firstByteLoad.lineBaseAddress,
            firstByteLoad.cacheSlotIndex,
            firstByteLoad.hit,
            true,
            false,
            std::nullopt,
            std::bit_cast<float>(rawValue)};
}

void Simulation::installCompletedTransactions() {
    for (MemoryTransaction& transaction : m_transactions) {
        if (transaction.isInstalledInCache()) {
            continue;
        }

        if (!transaction.isCompleted(m_currentTick)) {
            continue;
        }

        if (transaction.getKind() == MemoryTransactionKind::LineFill) {
            m_cache.loadLine(m_ram, transaction.getLineBaseAddress());
            transaction.markInstalledInCache();
        }
    }
}

MemoryTransaction* Simulation::findActiveLineFill(Address lineBaseAddress) {
    for (MemoryTransaction& transaction : m_transactions) {
        if (transaction.getKind() != MemoryTransactionKind::LineFill) {
            continue;
        }

        if (transaction.getLineBaseAddress() != lineBaseAddress) {
            continue;
        }

        if (transaction.isInstalledInCache()) {
            continue;
        }

        if (transaction.isCompleted(m_currentTick)) {
            continue;
        }

        return &transaction;
    }

    return nullptr;
}

const MemoryTransaction* Simulation::findActiveLineFill(Address lineBaseAddress) const {
    for (const MemoryTransaction& transaction : m_transactions) {
        if (transaction.getKind() != MemoryTransactionKind::LineFill) {
            continue;
        }

        if (transaction.getLineBaseAddress() != lineBaseAddress) {
            continue;
        }

        if (transaction.isInstalledInCache()) {
            continue;
        }

        if (transaction.isCompleted(m_currentTick)) {
            continue;
        }

        return &transaction;
    }

    return nullptr;
}
} // namespace sim
