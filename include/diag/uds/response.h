#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace diag::uds {

class Response {
 public:
  Response() = default;
  explicit Response(std::vector<uint8_t> pdu) : pdu_(std::move(pdu)) {}

  bool empty() const noexcept { return pdu_.empty(); }

  bool is_positive() const noexcept {
    return !pdu_.empty() && (pdu_[0] & 0x40U) != 0 && pdu_[0] != 0x7F;
  }

  bool is_negative() const noexcept {
    return !pdu_.empty() && pdu_[0] == 0x7F;
  }

  bool is_pending() const noexcept {
    return is_negative() && nrc() == 0x78;
  }

  uint8_t sid() const noexcept { return pdu_.empty() ? 0 : pdu_[0]; }

  uint8_t nrc() const noexcept {
    if (pdu_.size() < 3 || pdu_[0] != 0x7F) return 0;
    return pdu_[2];
  }

  const std::vector<uint8_t>& pdu() const noexcept { return pdu_; }

  uint8_t u8(std::size_t offset) const {
    Check(offset, 1);
    return pdu_[offset];
  }

  uint16_t u16(std::size_t offset) const {
    Check(offset, 2);
    return static_cast<uint16_t>((pdu_[offset] << 8) | pdu_[offset + 1]);
  }

 private:
  void Check(std::size_t offset, std::size_t n) const {
    if (offset + n > pdu_.size()) {
      throw std::out_of_range("uds::Response read out of range");
    }
  }

  std::vector<uint8_t> pdu_;
};

}  // namespace diag::uds
