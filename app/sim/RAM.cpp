#include "sim/RAM.h"

#include "sim/Math.h"

#include <algorithm>
#include <bit>
#include <stdexcept>

namespace sim {
RAM::RAM(std::size_t sizeInBytes) : m_bytes(sizeInBytes, std::byte{0}) {
}

std::size_t RAM::getLineCount() const {
    return math::ceilDiv(m_bytes.size(), kCacheLineSizeInBytes);
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

RAM::LineCellLabels RAM::getLineCellLabels(std::size_t lineIndex) const {
    const RamLineInfo lineInfo = getLineInfo(lineIndex);
    LineCellLabels labels{};
    labels.fill("");

    const Address lineStart = lineInfo.baseAddress;
    const Address lineEnd = lineStart + lineInfo.sizeInBytes;

    for (const FloatLabel& label : m_floatLabels) {
        if (label.address < lineStart || label.address >= lineEnd) {
            continue;
        }

        const std::size_t cellIndex = static_cast<std::size_t>((label.address - lineStart) / kFloatSizeInBytes);
        if (cellIndex >= labels.size()) {
            continue;
        }

        if (labels[cellIndex].empty()) {
            labels[cellIndex] = label.text;
        } else {
            labels[cellIndex] += " | " + label.text;
        }
    }

    return labels;
}

void RAM::writeByte(Address address, std::byte value) {
    validateRange(address, 1);
    m_bytes[static_cast<std::size_t>(address)] = value;
}

void RAM::writeFloat(Address address, float value) {
    validateRange(address, kFloatSizeInBytes);
    const std::uint32_t rawValue = std::bit_cast<std::uint32_t>(value);
    m_bytes[static_cast<std::size_t>(address)] = static_cast<std::byte>(rawValue & 0xFFu);
    m_bytes[static_cast<std::size_t>(address) + 1] = static_cast<std::byte>((rawValue >> 8) & 0xFFu);
    m_bytes[static_cast<std::size_t>(address) + 2] = static_cast<std::byte>((rawValue >> 16) & 0xFFu);
    m_bytes[static_cast<std::size_t>(address) + 3] = static_cast<std::byte>((rawValue >> 24) & 0xFFu);
}

void RAM::writeFloatArray(Address startAddress, const std::vector<float>& values) {
    for (std::size_t index = 0; index < values.size(); ++index) {
        writeFloat(startAddress + static_cast<Address>(index * kFloatSizeInBytes), values[index]);
    }
}

void RAM::setFloatLabels(Address startAddress, const std::vector<std::string>& labels) {
    for (std::size_t index = 0; index < labels.size(); ++index) {
        const Address address = startAddress + static_cast<Address>(index * kFloatSizeInBytes);
        validateRange(address, kFloatSizeInBytes);

        auto it = std::find_if(m_floatLabels.begin(), m_floatLabels.end(), [address](const FloatLabel& label) {
            return label.address == address;
        });

        if (it != m_floatLabels.end()) {
            it->text = labels[index];
        } else {
            m_floatLabels.push_back({address, labels[index]});
        }
    }
}

void RAM::clear(std::byte value) {
    std::fill(m_bytes.begin(), m_bytes.end(), value);
    m_floatLabels.clear();
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
