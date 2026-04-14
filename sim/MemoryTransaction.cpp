#include "sim/MemoryTransaction.h"

#include <algorithm>

namespace {
float toProgress(sim::Tick elapsed, sim::Tick duration) {
    if (duration == 0) {
        return 1.0f;
    }

    const float progress = static_cast<float>(elapsed) / static_cast<float>(duration);
    return std::clamp(progress, 0.0f, 1.0f);
}
} // namespace

namespace sim {
MemoryTransaction::MemoryTransaction(TransactionId id,
                                     MemoryTransactionKind kind,
                                     Address requestAddress,
                                     Address lineBaseAddress,
                                     std::size_t lineSizeInBytes,
                                     std::size_t targetCacheSlotIndex,
                                     Tick startTick,
                                     MemoryTransactionDurations durations)
    : m_id(id), m_kind(kind), m_requestAddress(requestAddress), m_lineBaseAddress(lineBaseAddress),
      m_lineSizeInBytes(lineSizeInBytes), m_targetCacheSlotIndex(targetCacheSlotIndex),
      m_startTick(startTick), m_durations(durations) {
}

bool MemoryTransaction::isCompleted(Tick tick) const {
    return tick >= getFinishTick();
}

MemoryTransactionPhase MemoryTransaction::getPhase(Tick tick) const {
    if (isCompleted(tick)) {
        return MemoryTransactionPhase::Completed;
    }

    const Tick elapsed = getElapsedTicks(tick);
    if (elapsed < m_durations.toRamPortTicks) {
        return MemoryTransactionPhase::ToRamPort;
    }

    if (elapsed < m_durations.toRamPortTicks + m_durations.busTransferTicks) {
        return MemoryTransactionPhase::OnBus;
    }

    return MemoryTransactionPhase::Install;
}

float MemoryTransaction::getOverallProgress(Tick tick) const {
    if (tick <= m_startTick) {
        return 0.0f;
    }

    return toProgress(getElapsedTicks(tick), m_durations.totalTicks());
}

float MemoryTransaction::getPhaseProgress(Tick tick) const {
    const Tick elapsed = getElapsedTicks(tick);

    switch (getPhase(tick)) {
    case MemoryTransactionPhase::ToRamPort:
        return toProgress(elapsed, m_durations.toRamPortTicks);

    case MemoryTransactionPhase::OnBus:
        return toProgress(elapsed - m_durations.toRamPortTicks, m_durations.busTransferTicks);

    case MemoryTransactionPhase::Install:
        return toProgress(elapsed - m_durations.toRamPortTicks - m_durations.busTransferTicks,
                          m_durations.installTicks);

    case MemoryTransactionPhase::Completed:
        return 1.0f;
    }

    return 1.0f;
}

void MemoryTransaction::markInstalledInCache() {
    m_installedInCache = true;
}

Tick MemoryTransaction::getElapsedTicks(Tick tick) const {
    if (tick <= m_startTick) {
        return 0;
    }

    return tick - m_startTick;
}
} // namespace sim
