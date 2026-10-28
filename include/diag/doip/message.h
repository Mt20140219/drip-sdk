#pragma once

#include <cstddef>
#include <cstdint>

#include "diag/doip/protocol.h"

namespace diag::doip {

// Raw view of a DoIP message after the 8-byte generic header has been parsed.
struct MessageView {
  uint8_t protocol_version = 0;
  uint8_t inverse_version = 0;
  PayloadType type = PayloadType::GenericNegativeAck;
  const uint8_t* payload = nullptr;
  std::size_t payload_size = 0;
};

}  // namespace diag::doip
