#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace diag::uds {

class Request {
 public:
  explicit Request(uint8_t sid) : pdu_(1, sid) {}

  Request& push_u8(uint8_t v) {
    pdu_.push_back(v);
    return *this;
  }

  Request& push_u16(uint16_t v) {
    pdu_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    pdu_.push_back(static_cast<uint8_t>(v & 0xFF));
    return *this;
  }

  Request& push_u24(uint32_t v) {
    pdu_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    pdu_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    pdu_.push_back(static_cast<uint8_t>(v & 0xFF));
    return *this;
  }

  Request& push_u32(uint32_t v) {
    pdu_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    pdu_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    pdu_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    pdu_.push_back(static_cast<uint8_t>(v & 0xFF));
    return *this;
  }

  Request& push_bytes(const uint8_t* p, std::size_t n) {
    pdu_.insert(pdu_.end(), p, p + n);
    return *this;
  }

  uint8_t sid() const noexcept { return pdu_[0]; }
  const std::vector<uint8_t>& pdu() const noexcept { return pdu_; }

 private:
  std::vector<uint8_t> pdu_;
};

}  // namespace diag::uds
