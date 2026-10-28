#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "diag/config.h"
#include "diag/error.h"
#include "diag/event.h"
#include "diag/uds/service.h"

namespace diag {

class EcuNode {
 public:
  explicit EcuNode(NodeConfig config);
  ~EcuNode();

  EcuNode(const EcuNode&) = delete;
  EcuNode& operator=(const EcuNode&) = delete;

  Error start();
  void stop();

  Error registerHandler(uint8_t sid, std::shared_ptr<uds::ServiceHandler> handler);
  void unregisterHandler(uint8_t sid);

  Error setDidValue(uint16_t did, std::vector<uint8_t> value);
  Error setDtc(uint32_t dtc, uint8_t status);
  void clearDtcs();

  void setSink(std::shared_ptr<EventSink> sink);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace diag
