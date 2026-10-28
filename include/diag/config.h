#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "diag/doip/protocol.h"
#include "diag/types.h"

namespace diag {

enum class Role { Tester, Entity };

struct NodeConfig {
  Role role = Role::Tester;
  std::string bind_address{"0.0.0.0"};
  uint16_t tcp_port = doip::kDefaultPort;
  uint16_t udp_port = doip::kDefaultPort;
  LogicalAddress logical_address = 0;
  std::size_t max_connections = 8;
  VehicleInfo vehicle;
  doip::ProtocolVersion doip_version = doip::ProtocolVersion::Iso13400_2012;
  Timing timing;
  Executor callback_executor;
};

}  // namespace diag
