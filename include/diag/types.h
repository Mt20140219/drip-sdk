#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace diag {

using LogicalAddress = uint16_t;

struct VehicleInfo {
  std::string vin;
  LogicalAddress logical_address = 0;
  std::array<uint8_t, 6> eid{};
  std::array<uint8_t, 6> gid{};
};

struct Timing {
  std::chrono::milliseconds p2{2000};
  std::chrono::milliseconds p2_star{5000};
  std::chrono::milliseconds alive_check{500};
  std::chrono::milliseconds inactivity{5000};
};

using Task = std::function<void()>;
using Executor = std::function<void(Task)>;

}  // namespace diag
