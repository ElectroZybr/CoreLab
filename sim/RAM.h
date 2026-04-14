#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sim {
using Address = std::uint64_t;

struct RamLineInfo {
    std::size_t index = 0;
    Address baseAddress = 0;
    std::size_t sizeInBytes = 0;
};

class RAM {
  public:
    static constexpr std::size_t kCacheLineSizeInBytes = 64;
    static constexpr std::size_t kFloatSizeInBytes = 4;

    explicit RAM(std::size_t sizeInBytes);

    [[nodiscard]] std::size_t getSizeInBytes() const {
        return m_bytes.size();
    }
    [[nodiscard]] std::size_t getLineCount() const;
    [[nodiscard]] bool contains(Address address, std::size_t sizeInBytes = 1) const;

    [[nodiscard]] RamLineInfo getLineInfo(std::size_t lineIndex) const;
    [[nodiscard]] RamLineInfo getLineForAddress(Address address) const;

    [[nodiscard]] std::span<const std::byte> readLineBytes(std::size_t lineIndex) const;
    [[nodiscard]] std::byte readByte(Address address) const;

    void writeByte(Address address, std::byte value);
    void clear(std::byte value = std::byte{0});

  private:
    [[nodiscard]] std::size_t getLineSize(std::size_t lineIndex) const;
    void validateRange(Address address, std::size_t sizeInBytes) const;

    std::vector<std::byte> m_bytes;
};
} // namespace sim
