#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace diag::util {

std::string ToHex(const uint8_t* data, std::size_t size);
std::string FormatHexDump(const uint8_t* data, std::size_t size,
                          std::size_t bytes_per_line = 16);

}  // namespace diag::util
