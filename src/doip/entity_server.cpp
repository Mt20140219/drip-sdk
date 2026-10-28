#include "doip/entity_server.h"

#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <utility>

#include "doip/codec.h"
#include "doip/session_util.h"

namespace diag::doip {

EntityServer::EntityServer(EntityConfig cfg) : cfg_(std::move(cfg)) {
  if (cfg_.vehicle.logical_address == 0) {
    cfg_.vehicle.logical_address = cfg_.logical_address;
  }
}

EntityServer::~EntityServer() { Stop(); }

bool EntityServer::Start() {
  if (running_.exchange(true)) return false;

  if (!tcp_.Listen(cfg_.bind_address, cfg_.tcp_port)) {
    running_ = false;
    return false;
  }
  if (!udp_.Bind(cfg_.bind_address, cfg_.udp_port)) {
    tcp_.Close();
    running_ = false;
    return false;
  }
  udp_.SetRecvTimeout(std::chrono::milliseconds(500));

  accept_thread_ = std::thread(&EntityServer::AcceptLoop, this);
  udp_thread_ = std::thread(&EntityServer::UdpLoop, this);
  return true;
}

void EntityServer::Stop() {
  if (!running_.exchange(false)) return;

  tcp_.Close();
  udp_.Close();
  {
    std::lock_guard<std::mutex> lk(clients_mu_);
    for (int fd : client_fds_) ::close(fd);
    client_fds_.clear();
  }

  if (accept_thread_.joinable()) accept_thread_.join();
  if (udp_thread_.joinable()) udp_thread_.join();

  std::vector<std::thread> threads;
  {
    std::lock_guard<std::mutex> lk(threads_mu_);
    threads.swap(client_threads_);
  }
  for (auto& t : threads) {
    if (t.joinable()) t.join();
  }
}

void EntityServer::SetSink(std::shared_ptr<diag::EventSink> sink) {
  std::lock_guard<std::mutex> lk(sink_mu_);
  sink_ = std::move(sink);
}

std::shared_ptr<diag::EventSink> EntityServer::Sink() const {
  std::lock_guard<std::mutex> lk(sink_mu_);
  return sink_;
}

void EntityServer::RegisterHandler(
    uint8_t sid, std::shared_ptr<diag::uds::ServiceHandler> handler) {
  std::lock_guard<std::mutex> lk(handlers_mu_);
  handlers_[sid] = std::move(handler);
}

void EntityServer::UnregisterHandler(uint8_t sid) {
  std::lock_guard<std::mutex> lk(handlers_mu_);
  handlers_.erase(sid);
}

void EntityServer::SetDidValue(uint16_t did, std::vector<uint8_t> value) {
  uds_store_.SetDidValue(did, std::move(value));
}

void EntityServer::SetDtc(uint32_t dtc, uint8_t status) {
  uds_store_.SetDtc(dtc, status);
}

void EntityServer::ClearDtcs() { uds_store_.ClearDtcs(); }

void EntityServer::AcceptLoop() {
  while (running_) {
    int fd = tcp_.Accept();
    if (fd < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) continue;
      break;
    }

    {
      std::lock_guard<std::mutex> lk(clients_mu_);
      client_fds_.push_back(fd);
    }
    {
      std::lock_guard<std::mutex> lk(threads_mu_);
      client_threads_.emplace_back(&EntityServer::ClientLoop, this, fd);
    }
  }
}

void EntityServer::UdpLoop() {
  std::vector<uint8_t> buf(2048);
  while (running_) {
    std::string from;
    uint16_t port = 0;
    std::ptrdiff_t n = udp_.RecvFrom(buf.data(), buf.size(), from, port);
    if (n < 0) break;
    if (n == 0) continue;  // Timeout slice.

    Message msg;
    if (!Decode(buf.data(), static_cast<std::size_t>(n), msg)) continue;
    EmitMessage(Sink(), msg, false);

    if (msg.type == PayloadType::VehicleIdentificationRequest ||
        msg.type == PayloadType::VehicleIdentificationRequestWithEid ||
        msg.type == PayloadType::VehicleIdentificationRequestWithVin) {
      Message resp{
          cfg_.version,
          PayloadType::VehicleAnnouncementOrIdentificationResponse,
          BuildVehicleIdResponse(cfg_.vehicle),
      };
      EmitMessage(Sink(), resp, true);
      std::vector<uint8_t> bytes = Encode(resp);
      udp_.SendTo(from, port, bytes.data(), bytes.size());
    }
  }
}

bool EntityServer::Send(int fd, PayloadType type,
                        const std::vector<uint8_t>& payload) {
  Message msg{cfg_.version, type, payload};
  EmitMessage(Sink(), msg, true);
  std::vector<uint8_t> bytes = Encode(msg);
  return diag::net::SendAll(fd, bytes.data(), bytes.size()) ==
         static_cast<std::ptrdiff_t>(bytes.size());
}

void EntityServer::ClientLoop(int fd) {
  diag::net::SetRecvTimeout(fd, cfg_.inactivity);
  bool routing_active = false;
  diag::uds::Session uds_session(&uds_store_);
  EmitState(Sink(), diag::ConnectionState::Connecting);

  while (running_) {
    uint8_t hdr[kHeaderSize];
    std::ptrdiff_t got = diag::net::RecvAll(fd, hdr, sizeof(hdr));
    if (got != static_cast<std::ptrdiff_t>(sizeof(hdr))) break;

    uint32_t len = (static_cast<uint32_t>(hdr[4]) << 24) |
                   (static_cast<uint32_t>(hdr[5]) << 16) |
                   (static_cast<uint32_t>(hdr[6]) << 8) |
                   static_cast<uint32_t>(hdr[7]);
    if (len > (1u << 20)) break;

    std::vector<uint8_t> full(kHeaderSize + len);
    std::copy(hdr, hdr + kHeaderSize, full.begin());
    if (len > 0) {
      got = diag::net::RecvAll(fd, full.data() + kHeaderSize, len);
      if (got != static_cast<std::ptrdiff_t>(len)) break;
    }

    Message msg;
    if (!Decode(full.data(), full.size(), msg)) {
      Send(fd, PayloadType::GenericNegativeAck, {});
      continue;
    }
    EmitMessage(Sink(), msg, false);

    switch (msg.type) {
      case PayloadType::VehicleIdentificationRequest:
      case PayloadType::VehicleIdentificationRequestWithEid:
      case PayloadType::VehicleIdentificationRequestWithVin:
        Send(fd, PayloadType::VehicleAnnouncementOrIdentificationResponse,
             BuildVehicleIdResponse(cfg_.vehicle));
        break;

      case PayloadType::RoutingActivationRequest: {
        uint16_t source = 0;
        ActivationType type = ActivationType::DefaultCentralSecurity;
        if (!ParseRoutingActivationRequest(msg.payload, source, type)) {
          Send(fd, PayloadType::RoutingActivationResponse,
               BuildRoutingActivationResponse(source, cfg_.logical_address,
                                              0x03));
          break;
        }
        routing_active = true;
        EmitState(Sink(), diag::ConnectionState::RoutingActive);
        Send(fd, PayloadType::RoutingActivationResponse,
             BuildRoutingActivationResponse(source, cfg_.logical_address,
                                            0x10));
        break;
      }

      case PayloadType::AliveCheckRequest:
        Send(fd, PayloadType::AliveCheckResponse,
             BuildAliveCheck(cfg_.logical_address));
        break;

      case PayloadType::DiagnosticMessage:
        HandleDiagnostic(fd, msg, routing_active, uds_session);
        break;

      default:
        Send(fd, PayloadType::GenericNegativeAck, {});
        break;
    }
  }

  EmitState(Sink(), diag::ConnectionState::Disconnected);
  ::close(fd);
  EraseClientFd(fd);
}

void EntityServer::HandleDiagnostic(int fd, const Message& msg,
                                    bool routing_active,
                                    diag::uds::Session& session) {
  uint16_t source = 0;
  uint16_t target = 0;
  std::vector<uint8_t> uds;
  if (!ParseDiagnosticMessage(msg.payload, source, target, uds) ||
      uds.empty()) {
    Send(fd, PayloadType::DiagnosticMessageNegativeAck,
         BuildDiagnosticNack(cfg_.logical_address, source, 0x00));
    return;
  }

  if (!routing_active) {
    Send(fd, PayloadType::DiagnosticMessageNegativeAck,
         BuildDiagnosticNack(cfg_.logical_address, source, 0x04));
    return;
  }

  if (target != cfg_.logical_address) {
    Send(fd, PayloadType::DiagnosticMessageNegativeAck,
         BuildDiagnosticNack(cfg_.logical_address, source, 0x01));
    return;
  }

  Send(fd, PayloadType::DiagnosticMessagePositiveAck,
       BuildDiagnosticAck(cfg_.logical_address, source, 0x00));

  uint8_t sid = uds[0];
  std::vector<uint8_t> resp;
  bool pending_first = false;
  std::chrono::milliseconds delay{0};

  std::shared_ptr<diag::uds::ServiceHandler> handler;
  {
    std::lock_guard<std::mutex> lk(handlers_mu_);
    auto it = handlers_.find(sid);
    if (it != handlers_.end()) handler = it->second;
  }

  if (handler) {
    diag::uds::RequestContext ctx{source, target, false,
                                  session.active_session(),
                                  session.security_level(),
                                  std::chrono::steady_clock::now()};
    std::vector<uint8_t> params(uds.begin() + 1, uds.end());
    diag::uds::ServiceResponse sr;
    handler->onRequest(ctx, params, sr);
    if (sr.is_negative()) {
      resp = {0x7F, sid, sr.nrc()};
    } else {
      resp.push_back(static_cast<uint8_t>(sid | 0x40));
      resp.insert(resp.end(), sr.data().begin(), sr.data().end());
    }
  } else {
    diag::uds::HandleResult result = session.Handle(uds);
    resp = std::move(result.pdu);
    pending_first = result.pending_first;
    delay = result.delay;
  }

  if (pending_first) {
    std::vector<uint8_t> pending = {0x7F, sid, 0x78};
    Send(fd, PayloadType::DiagnosticMessage,
         BuildDiagnosticMessage(cfg_.logical_address, source, pending.data(),
                                pending.size()));
    std::this_thread::sleep_for(delay);
  }

  Send(fd, PayloadType::DiagnosticMessage,
       BuildDiagnosticMessage(cfg_.logical_address, source, resp.data(),
                              resp.size()));
}

void EntityServer::EraseClientFd(int fd) {
  std::lock_guard<std::mutex> lk(clients_mu_);
  for (auto it = client_fds_.begin(); it != client_fds_.end(); ++it) {
    if (*it == fd) {
      client_fds_.erase(it);
      break;
    }
  }
}

}  // namespace diag::doip
