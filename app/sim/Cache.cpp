#include "sim/Cache.h"

#include "sim/Math.h"

#include <algorithm>
#include <stdexcept>

namespace sim {
Cache::Cache(std::size_t sizeInBytes) {
    if (sizeInBytes == 0) {
        throw std::invalid_argument("Cache size must be greater than zero");
    }

    m_sizeInBytes = math::ceilDiv(sizeInBytes, kCacheLineSizeInBytes) * kCacheLineSizeInBytes;
    m_slots.resize(m_sizeInBytes / kCacheLineSizeInBytes);

    for (Slot& slot : m_slots) {
        slot.bytes.resize(kCacheLineSizeInBytes, std::byte{0});
    }
}

bool Cache::contains(Address address) const {
    const std::optional<std::size_t> slotIndex = findSlotIndex(address);
    return slotIndex.has_value();
}

std::optional<std::size_t> Cache::findSlotIndex(Address address) const {
    if (m_slots.empty()) {
        return std::nullopt;
    }

    const std::size_t slotIndex = getSlotIndexForAddress(address);
    const Slot& slot = m_slots[slotIndex];

    if (!slot.valid) {
        return std::nullopt;
    }

    if (address < slot.baseAddress) {
        return std::nullopt;
    }

    const std::size_t offset = static_cast<std::size_t>(address - slot.baseAddress);
    if (offset >= slot.sizeInBytes) {
        return std::nullopt;
    }

    return slotIndex;
}

std::size_t Cache::getTargetSlotIndex(Address address) const {
    return getSlotIndexForAddress(address);
}

CacheSlotInfo Cache::getSlotInfo(std::size_t slotIndex) const {
    validateSlotIndex(slotIndex);

    const Slot& slot = m_slots[slotIndex];
    return {slotIndex, slot.baseAddress, slot.sizeInBytes, slot.valid};
}

std::span<const std::byte> Cache::readLineBytes(std::size_t slotIndex) const {
    validateSlotIndex(slotIndex);

    const Slot& slot = m_slots[slotIndex];
    return {slot.bytes.data(), slot.sizeInBytes};
}

std::byte Cache::readByte(Address address) const {
    const std::optional<std::size_t> slotIndex = findSlotIndex(address);
    if (!slotIndex.has_value()) {
        throw std::out_of_range("Cache miss while reading byte");
    }

    const Slot& slot = m_slots[*slotIndex];
    const std::size_t offset = static_cast<std::size_t>(address - slot.baseAddress);
    return slot.bytes[offset];
}

CacheSlotInfo Cache::loadLine(const RAM& ram, Address address) {
    const RamLineInfo ramLine = ram.getLineForAddress(address);
    const std::span<const std::byte> ramBytes = ram.readLineBytes(ramLine.index);
    const std::size_t slotIndex = getSlotIndexForAddress(address);
    Slot& slot = m_slots[slotIndex];

    slot.valid = true;
    slot.baseAddress = getLineBaseAddress(address);
    slot.sizeInBytes = ramLine.sizeInBytes;
    std::fill(slot.bytes.begin(), slot.bytes.end(), std::byte{0});
    std::copy(ramBytes.begin(), ramBytes.end(), slot.bytes.begin());

    return getSlotInfo(slotIndex);
}

void Cache::clear() {
    for (Slot& slot : m_slots) {
        slot.valid = false;
        slot.baseAddress = 0;
        slot.sizeInBytes = 0;
        std::fill(slot.bytes.begin(), slot.bytes.end(), std::byte{0});
    }
}

std::size_t Cache::getSlotIndexForAddress(Address address) const {
    if (m_slots.empty()) {
        throw std::out_of_range("Cache has no slots");
    }

    return static_cast<std::size_t>(address / kCacheLineSizeInBytes) % m_slots.size();
}

Address Cache::getLineBaseAddress(Address address) const {
    return (address / kCacheLineSizeInBytes) * kCacheLineSizeInBytes;
}

void Cache::validateSlotIndex(std::size_t slotIndex) const {
    if (slotIndex >= m_slots.size()) {
        throw std::out_of_range("Cache slot index is out of range");
    }
}
} // namespace sim
