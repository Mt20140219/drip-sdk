#pragma once

#include <memory>
#include <string>

#include "diag/doip/message.h"
#include "diag/event.h"
#include "diag/util/log.h"
#include "doip/codec.h"

namespace diag::doip {

inline void EmitMessage(const std::shared_ptr<diag::EventSink>& sink,
                        const Message& msg, bool tx) {
  if (!sink) return;

  diag::doip::MessageView view{
      static_cast<uint8_t>(msg.version),
      InverseVersion(msg.version),
      msg.type,
      msg.payload.data(),
      msg.payload.size(),
  };
  sink->onDoipMessage(view, tx);

  std::vector<uint8_t> bytes = Encode(msg);
  std::string text = (tx ? "TX " : "RX ") + std::string(PayloadTypeName(msg.type)) +
                     " (" + std::to_string(bytes.size()) + " bytes)\n" +
                     diag::util::FormatHexDump(bytes.data(), bytes.size());
  sink->onLog(diag::LogLevel::Info, text);
}

inline void EmitState(const std::shared_ptr<diag::EventSink>& sink,
                      diag::ConnectionState state) {
  if (sink) sink->onConnectionState(state);
}

}  // namespace diag::doip
