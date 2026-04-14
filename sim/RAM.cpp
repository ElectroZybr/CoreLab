#include "sim/RAM.h"

#include <algorithm>
#include <stdexcept>

namespace {
std::size_t ceilDiv(std::size_t value, std::size_t divisor) {
    if (divisor == 0) {
        return 0;
    }
    return (value + divisor - 1) / divisor;
}
} // namespace

namespace sim {
RAM::RAM(std::size_t sizeInBytes) : m_bytes(sizeInBytes, std::byte{0}) {
}

std::size_t RAM::getLineCount() const {
    return ceilDiv(m_bytes.size(), kCacheLineSizeInBytes);
}

bool RAM::contains(Address address, std::size_t sizeInBytes) const {
    if (sizeInBytes == 0) {
        return static_cast<std::size_t>(address) <= m_bytes.size();
    }
    if (address >= m_bytes.size()) {
        return false;
    }

    return sizeInBytes <= m_bytes.size() - static_cast<std::size_t>(address);
}

RamLineInfo RAM::getLineInfo(std::size_t lineIndex) const {
    if (lineIndex >= getLineCount()) {
        throw std::out_of_range("RAM line index is out of range");
    }

    return {lineIndex, static_cast<Address>(lineIndex * kCacheLineSizeInBytes), getLineSize(lineIndex)};
}

RamLineInfo RAM::getLineForAddress(Address address) const {
    validateRange(address, 1);
    return getLineInfo(static_cast<std::size_t>(address) / kCacheLineSizeInBytes);
}

std::span<const std::byte> RAM::readLineBytes(std::size_t lineIndex) const {
    const RamLineInfo lineInfo = getLineInfo(lineIndex);
    return {m_bytes.data() + static_cast<std::size_t>(lineInfo.baseAddress), lineInfo.sizeInBytes};
}

std::byte RAM::readByte(Address address) const {
    validateRange(address, 1);
    return m_bytes[static_cast<std::size_t>(address)];
}

void RAM::writeByte(Address address, std::byte value) {
    validateRange(address, 1);
    m_bytes[static_cast<std::size_t>(address)] = value;
}

void RAM::clear(std::byte value) {
    std::fill(m_bytes.begin(), m_bytes.end(), value);
}

std::size_t RAM::getLineSize(std::size_t lineIndex) const {
    const std::size_t lineStart = lineIndex * kCacheLineSizeInBytes;
    const std::size_t remainingBytes = m_bytes.size() - lineStart;
    return std::min(remainingBytes, kCacheLineSizeInBytes);
}

void RAM::validateRange(Address address, std::size_t sizeInBytes) const {
    if (!contains(address, sizeInBytes)) {
        throw std::out_of_range("RAM address is out of range");
    }
}
} // namespace sim
