#pragma once

namespace diag {

enum class Error {
  kOk = 0,
  kInvalidArgument,
  kNotRunning,
  kAlreadyRunning,
  kTimeout,
  kConnectionClosed,
  kRoutingNotActive,
  kBusy,
  kNrc,
  kDoipNack,
  kInternal,
};

inline const char* ErrorToString(Error e) {
  switch (e) {
    case Error::kOk: return "ok";
    case Error::kInvalidArgument: return "invalid argument";
    case Error::kNotRunning: return "not running";
    case Error::kAlreadyRunning: return "already running";
    case Error::kTimeout: return "timeout";
    case Error::kConnectionClosed: return "connection closed";
    case Error::kRoutingNotActive: return "routing not active";
    case Error::kBusy: return "busy";
    case Error::kNrc: return "negative response";
    case Error::kDoipNack: return "DoIP negative acknowledgement";
    case Error::kInternal: return "internal error";
  }
  return "unknown";
}

}  // namespace diag
