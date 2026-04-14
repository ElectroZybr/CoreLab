#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "sim/RAM.h"

namespace sim {
struct CacheSlotInfo {
    std::size_t index = 0;
    Address baseAddress = 0;
    std::size_t sizeInBytes = 0;
    bool valid = false;
};

class Cache {
  public:
    static constexpr std::size_t kCacheLineSizeInBytes = RAM::kCacheLineSizeInBytes;

    explicit Cache(std::size_t sizeInBytes);

    [[nodiscard]] std::size_t getSizeInBytes() const {
        return m_sizeInBytes;
    }
    [[nodiscard]] std::size_t getSlotCount() const {
        return m_slots.size();
    }

    [[nodiscard]] bool contains(Address address) const;
    [[nodiscard]] std::optional<std::size_t> findSlotIndex(Address address) const;
    [[nodiscard]] std::size_t getTargetSlotIndex(Address address) const;
    [[nodiscard]] CacheSlotInfo getSlotInfo(std::size_t slotIndex) const;
    [[nodiscard]] std::span<const std::byte> readLineBytes(std::size_t slotIndex) const;
    [[nodiscard]] std::byte readByte(Address address) const;

    CacheSlotInfo loadLine(const RAM& ram, Address address);
    void clear();

  private:
    struct Slot {
        bool valid = false;
        Address baseAddress = 0;
        std::size_t sizeInBytes = 0;
        std::vector<std::byte> bytes;
    };

    [[nodiscard]] std::size_t getSlotIndexForAddress(Address address) const;
    [[nodiscard]] Address getLineBaseAddress(Address address) const;
    void validateSlotIndex(std::size_t slotIndex) const;

    std::size_t m_sizeInBytes = 0;
    std::vector<Slot> m_slots;
};
} // namespace sim
