#pragma once

#include <cstddef>
#include <cstdint>

#include "sim/RAM.h"

namespace sim {
using Tick = std::uint64_t;
using TransactionId = std::uint64_t;

enum class MemoryTransactionKind {
    LineFill,
};

enum class MemoryTransactionPhase {
    ToRamPort,
    OnBus,
    Install,
    Completed,
};

struct MemoryTransactionDurations {
    Tick toRamPortTicks = 3;
    Tick busTransferTicks = 20;
    Tick installTicks = 2;

    [[nodiscard]] Tick totalTicks() const {
        return toRamPortTicks + busTransferTicks + installTicks;
    }
};

class MemoryTransaction {
  public:
    MemoryTransaction(TransactionId id,
                      MemoryTransactionKind kind,
                      Address requestAddress,
                      Address lineBaseAddress,
                      std::size_t lineSizeInBytes,
                      std::size_t targetCacheSlotIndex,
                      Tick startTick,
                      MemoryTransactionDurations durations = {});

    [[nodiscard]] TransactionId getId() const {
        return m_id;
    }
    [[nodiscard]] MemoryTransactionKind getKind() const {
        return m_kind;
    }
    [[nodiscard]] Address getRequestAddress() const {
        return m_requestAddress;
    }
    [[nodiscard]] Address getLineBaseAddress() const {
        return m_lineBaseAddress;
    }
    [[nodiscard]] std::size_t getLineSizeInBytes() const {
        return m_lineSizeInBytes;
    }
    [[nodiscard]] std::size_t getTargetCacheSlotIndex() const {
        return m_targetCacheSlotIndex;
    }
    [[nodiscard]] Tick getStartTick() const {
        return m_startTick;
    }
    [[nodiscard]] Tick getFinishTick() const {
        return m_startTick + m_durations.totalTicks();
    }
    [[nodiscard]] const MemoryTransactionDurations& getDurations() const {
        return m_durations;
    }
    [[nodiscard]] bool isInstalledInCache() const {
        return m_installedInCache;
    }

    [[nodiscard]] bool isCompleted(Tick tick) const;
    [[nodiscard]] MemoryTransactionPhase getPhase(Tick tick) const;
    [[nodiscard]] float getOverallProgress(Tick tick) const;
    [[nodiscard]] float getPhaseProgress(Tick tick) const;

    void markInstalledInCache();

  private:
    [[nodiscard]] Tick getElapsedTicks(Tick tick) const;

    TransactionId m_id = 0;
    MemoryTransactionKind m_kind = MemoryTransactionKind::LineFill;
    Address m_requestAddress = 0;
    Address m_lineBaseAddress = 0;
    std::size_t m_lineSizeInBytes = 0;
    std::size_t m_targetCacheSlotIndex = 0;
    Tick m_startTick = 0;
    MemoryTransactionDurations m_durations;
    bool m_installedInCache = false;
};
} // namespace sim
