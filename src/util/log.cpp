#include "diag/util/log.h"

#include <cctype>
#include <cstdio>

namespace diag::util {

std::string ToHex(const uint8_t* data, std::size_t size) {
  std::string out;
  out.reserve(size * 3);
  char buf[4];
  for (std::size_t i = 0; i < size; ++i) {
    std::snprintf(buf, sizeof(buf), "%02X", data[i]);
    out += buf;
    if (i + 1 < size) out += ' ';
  }
  return out;
}

std::string FormatHexDump(const uint8_t* data, std::size_t size,
                          std::size_t bytes_per_line) {
  std::string out;
  char buf[16];
  for (std::size_t off = 0; off < size; off += bytes_per_line) {
    std::snprintf(buf, sizeof(buf), "%04X  ", static_cast<unsigned>(off));
    out += buf;

    std::size_t line =
        (size - off < bytes_per_line) ? (size - off) : bytes_per_line;
    for (std::size_t i = 0; i < line; ++i) {
      std::snprintf(buf, sizeof(buf), "%02X ", data[off + i]);
      out += buf;
    }
    for (std::size_t i = line; i < bytes_per_line; ++i) {
      out += "   ";
    }

    out += " |";
    for (std::size_t i = 0; i < line; ++i) {
      unsigned char c = data[off + i];
      out += (std::isprint(c) ? static_cast<char>(c) : '.');
    }
    out += "|\n";
  }
  return out;
}

}  // namespace diag::util
