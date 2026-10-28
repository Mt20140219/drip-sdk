#pragma once

#include <chrono>
#include <memory>
#include <string_view>
#include <vector>

#include "diag/config.h"
#include "diag/error.h"
#include "diag/event.h"
#include "diag/uds/request.h"
#include "diag/uds/response.h"

namespace diag {

class TesterNode {
 public:
  explicit TesterNode(NodeConfig config);
  ~TesterNode();

  TesterNode(const TesterNode&) = delete;
  TesterNode& operator=(const TesterNode&) = delete;

  Error start();
  void stop();

  Error discover(std::vector<VehicleInfo>& out, std::chrono::milliseconds timeout);
  Error connect(std::string_view address, LogicalAddress target);
  Error routingActivate(doip::ActivationType type);
  Error disconnect();
  Error send(const uds::Request& req, uds::Response& resp);

  void setSink(std::shared_ptr<EventSink> sink);

 private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace diag
