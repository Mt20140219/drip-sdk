#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "diag/doip/protocol.h"
#include "diag/error.h"
#include "diag/event.h"
#include "diag/net/socket.h"
#include "diag/types.h"
#include "diag/uds/request.h"
#include "diag/uds/response.h"
#include "doip/codec.h"

namespace diag::doip {

struct TesterConfig {
  std::string bind_address{"0.0.0.0"};
  uint16_t tcp_port = kDefaultPort;
  uint16_t udp_port = kDefaultPort;
  LogicalAddress logical_address = kDefaultTesterAddress;
  ProtocolVersion version = ProtocolVersion::Iso13400_2012;
  Timing timing;
};

class TesterClient {
 public:
  explicit TesterClient(TesterConfig cfg);
  ~TesterClient();

  TesterClient(const TesterClient&) = delete;
  TesterClient& operator=(const TesterClient&) = delete;

  Error Start();
  void Stop();

  void SetSink(std::shared_ptr<diag::EventSink> sink);

  Error Discover(std::vector<VehicleInfo>& out,
                 std::chrono::milliseconds timeout);
  Error Connect(const std::string& address, LogicalAddress target);
  Error RoutingActivate(ActivationType type);
  Error Disconnect();
  Error Send(const diag::uds::Request& req, diag::uds::Response& resp);

 private:
  void ReaderLoop();
  void AliveLoop();

  bool SendMessage(PayloadType type, const std::vector<uint8_t>& payload);
  Error WaitForAny(const std::vector<PayloadType>& types, Message& out,
                   std::chrono::milliseconds timeout);
  std::shared_ptr<diag::EventSink> Sink() const;

  TesterConfig cfg_;
  std::atomic<bool> running_{false};
  std::atomic<bool> routing_active_{false};

  diag::net::TcpClient tcp_;
  int fd_ = -1;
  LogicalAddress target_ = 0;

  std::thread reader_thread_;
  std::thread alive_thread_;

  std::mutex mu_;
  std::condition_variable cv_;
  std::vector<Message> rx_queue_;
  std::chrono::steady_clock::time_point last_alive_{};

  mutable std::mutex sink_mu_;
  std::shared_ptr<diag::EventSink> sink_;
};

}  // namespace diag::doip
