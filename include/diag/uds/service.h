#pragma once

#include <chrono>
#include <cstdint>
#include <utility>
#include <vector>

#include "diag/types.h"

namespace diag::uds {

struct RequestContext {
  LogicalAddress source = 0;
  LogicalAddress target = 0;
  bool functional = false;
  uint8_t active_session = 0x01;
  uint8_t security_level = 0x00;
  std::chrono::steady_clock::time_point rx_time{};
};

class ServiceResponse {
 public:
  void setData(std::vector<uint8_t> data) {
    data_ = std::move(data);
    is_negative_ = false;
  }

  void setNegative(uint8_t nrc) {
    nrc_ = nrc;
    is_negative_ = true;
  }

  void setResponsePending() { setNegative(0x78); }

  bool is_negative() const noexcept { return is_negative_; }
  uint8_t nrc() const noexcept { return nrc_; }
  const std::vector<uint8_t>& data() const noexcept { return data_; }

 private:
  std::vector<uint8_t> data_;
  uint8_t nrc_ = 0;
  bool is_negative_ = false;
};

class ServiceHandler {
 public:
  virtual ~ServiceHandler() = default;
  virtual void onRequest(const RequestContext& ctx,
                         const std::vector<uint8_t>& params,
                         ServiceResponse& resp) = 0;
};

}  // namespace diag::uds
