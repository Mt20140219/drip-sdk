#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "diag/doip/protocol.h"
#include "diag/event.h"
#include "diag/net/socket.h"
#include "diag/types.h"
#include "diag/uds/service.h"
#include "doip/codec.h"
#include "uds/service_layer.h"

namespace diag::doip {

struct EntityConfig {
  std::string bind_address{"0.0.0.0"};
  uint16_t tcp_port = kDefaultPort;
  uint16_t udp_port = kDefaultPort;
  LogicalAddress logical_address = kDefaultEntityAddress;
  std::size_t max_connections = 8;
  VehicleInfo vehicle;
  ProtocolVersion version = ProtocolVersion::Iso13400_2012;
  std::chrono::milliseconds inactivity{5000};
};

class EntityServer {
 public:
  explicit EntityServer(EntityConfig cfg);
  ~EntityServer();

  EntityServer(const EntityServer&) = delete;
  EntityServer& operator=(const EntityServer&) = delete;

  bool Start();
  void Stop();

  void SetSink(std::shared_ptr<diag::EventSink> sink);
  void RegisterHandler(uint8_t sid,
                       std::shared_ptr<diag::uds::ServiceHandler> handler);
  void UnregisterHandler(uint8_t sid);

  void SetDidValue(uint16_t did, std::vector<uint8_t> value);
  void SetDtc(uint32_t dtc, uint8_t status);
  void ClearDtcs();

 private:
  void AcceptLoop();
  void UdpLoop();
  void ClientLoop(int fd);
  void HandleDiagnostic(int fd, const Message& msg, bool routing_active,
                        diag::uds::Session& session);

  bool Send(int fd, PayloadType type, const std::vector<uint8_t>& payload);
  void EraseClientFd(int fd);
  std::shared_ptr<diag::EventSink> Sink() const;

  EntityConfig cfg_;
  std::atomic<bool> running_{false};

  diag::net::TcpServer tcp_;
  diag::net::UdpSocket udp_;

  std::thread accept_thread_;
  std::thread udp_thread_;

  std::mutex clients_mu_;
  std::vector<int> client_fds_;

  std::mutex threads_mu_;
  std::vector<std::thread> client_threads_;

  std::mutex handlers_mu_;
  std::unordered_map<uint8_t, std::shared_ptr<diag::uds::ServiceHandler>>
      handlers_;

  diag::uds::DataStore uds_store_;

  mutable std::mutex sink_mu_;
  std::shared_ptr<diag::EventSink> sink_;
};

}  // namespace diag::doip
