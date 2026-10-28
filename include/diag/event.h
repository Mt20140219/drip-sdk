#pragma once

#include <string_view>

#include "diag/doip/message.h"

namespace diag {

enum class LogLevel { Debug, Info, Warning, Error };

enum class ConnectionState { Disconnected, Connecting, RoutingActive, Closing };

class EventSink {
 public:
  virtual ~EventSink() = default;
  virtual void onConnectionState(ConnectionState /*state*/) {}
  virtual void onLog(LogLevel /*level*/, std::string_view /*message*/) {}
  virtual void onDoipMessage(doip::MessageView /*msg*/, bool /*tx*/) {}
};

}  // namespace diag
