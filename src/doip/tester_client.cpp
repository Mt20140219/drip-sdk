#include "doip/tester_client.h"

#include <algorithm>
#include <set>
#include <utility>

#include "doip/session_util.h"

namespace diag::doip {

TesterClient::TesterClient(TesterConfig cfg) : cfg_(std::move(cfg)) {}

TesterClient::~TesterClient() { Stop(); }

Error TesterClient::Start() {
  if (running_.exchange(true)) return Error::kAlreadyRunning;
  return Error::kOk;
}

void TesterClient::Stop() {
  if (!running_.exchange(false)) return;
  routing_active_ = false;
  cv_.notify_all();
  tcp_.Close();
  fd_ = -1;
  if (alive_thread_.joinable()) alive_thread_.join();
  if (reader_thread_.joinable()) reader_thread_.join();
}

void TesterClient::SetSink(std::shared_ptr<diag::EventSink> sink) {
  std::lock_guard<std::mutex> lk(sink_mu_);
  sink_ = std::move(sink);
}

std::shared_ptr<diag::EventSink> TesterClient::Sink() const {
  std::lock_guard<std::mutex> lk(sink_mu_);
  return sink_;
}

bool TesterClient::SendMessage(PayloadType type,
                               const std::vector<uint8_t>& payload) {
  if (fd_ < 0) return false;
  Message msg{cfg_.version, type, payload};
  EmitMessage(Sink(), msg, true);
  std::vector<uint8_t> bytes = Encode(msg);
  return diag::net::SendAll(fd_, bytes.data(), bytes.size()) ==
         static_cast<std::ptrdiff_t>(bytes.size());
}

Error TesterClient::WaitForAny(const std::vector<PayloadType>& types,
                               Message& out,
                               std::chrono::milliseconds timeout) {
  std::unique_lock<std::mutex> lk(mu_);
  auto deadline = std::chrono::steady_clock::now() + timeout;
  while (true) {
    for (auto it = rx_queue_.begin(); it != rx_queue_.end(); ++it) {
      if (std::find(types.begin(), types.end(), it->type) != types.end()) {
        out = *it;
        rx_queue_.erase(it);
        return Error::kOk;
      }
    }
    if (cv_.wait_until(lk, deadline) == std::cv_status::timeout) {
      return Error::kTimeout;
    }
  }
}

Error TesterClient::Discover(std::vector<VehicleInfo>& out,
                             std::chrono::milliseconds timeout) {
  diag::net::UdpSocket udp;
  if (!udp.Bind("0.0.0.0", 0)) return Error::kInternal;
  udp.SetBroadcast(true);
  udp.SetRecvTimeout(std::chrono::milliseconds(100));

  Message req{cfg_.version, PayloadType::VehicleIdentificationRequest, {}};
  std::vector<uint8_t> bytes = Encode(req);
  udp.SendTo("255.255.255.255", cfg_.udp_port, bytes.data(), bytes.size());
  udp.SendTo("127.0.0.1", cfg_.udp_port, bytes.data(), bytes.size());

  out.clear();
  std::set<std::string> seen;
  auto deadline = std::chrono::steady_clock::now() + timeout;
  std::vector<uint8_t> buf(2048);
  while (std::chrono::steady_clock::now() < deadline) {
    std::string from;
    uint16_t port = 0;
    std::ptrdiff_t n = udp.RecvFrom(buf.data(), buf.size(), from, port);
    if (n < 0) return Error::kInternal;
    if (n == 0) continue;

    Message msg;
    if (!Decode(buf.data(), static_cast<std::size_t>(n), msg)) continue;
    EmitMessage(Sink(), msg, false);
    if (msg.type != PayloadType::VehicleAnnouncementOrIdentificationResponse) {
      continue;
    }

    VehicleInfo info;
    if (!ParseVehicleIdResponse(msg.payload, info)) continue;
    std::string key = info.vin + ":" + std::to_string(info.logical_address);
    if (seen.insert(key).second) out.push_back(info);
  }
  return out.empty() ? Error::kTimeout : Error::kOk;
}

Error TesterClient::Connect(const std::string& address,
                            LogicalAddress target) {
  if (fd_ >= 0) Disconnect();
  EmitState(Sink(), diag::ConnectionState::Connecting);

  if (!tcp_.Connect(address, cfg_.tcp_port)) {
    EmitState(Sink(), diag::ConnectionState::Disconnected);
    return Error::kConnectionClosed;
  }

  fd_ = tcp_.fd();
  target_ = target;
  diag::net::SetRecvTimeout(fd_, std::chrono::milliseconds(500));

  reader_thread_ = std::thread(&TesterClient::ReaderLoop, this);

  if (!SendMessage(PayloadType::VehicleIdentificationRequest, {})) {
    Disconnect();
    return Error::kConnectionClosed;
  }
  Message resp;
  Error err = WaitForAny(
      {PayloadType::VehicleAnnouncementOrIdentificationResponse}, resp,
      cfg_.timing.p2);
  if (err != Error::kOk) {
    Disconnect();
    return err;
  }
  return Error::kOk;
}

Error TesterClient::RoutingActivate(ActivationType type) {
  if (fd_ < 0) return Error::kNotRunning;

  if (!SendMessage(PayloadType::RoutingActivationRequest,
                   BuildRoutingActivationRequest(cfg_.logical_address, type))) {
    return Error::kConnectionClosed;
  }

  Message resp;
  Error err = WaitForAny({PayloadType::RoutingActivationResponse}, resp,
                         cfg_.timing.p2);
  if (err != Error::kOk) return err;

  uint16_t tester = 0;
  uint16_t entity = 0;
  uint8_t code = 0;
  if (!ParseRoutingActivationResponse(resp.payload, tester, entity, code)) {
    return Error::kInternal;
  }
  if (code != 0x10) return Error::kDoipNack;

  routing_active_ = true;
  {
    std::lock_guard<std::mutex> lk(mu_);
    last_alive_ = std::chrono::steady_clock::now();
  }
  EmitState(Sink(), diag::ConnectionState::RoutingActive);
  alive_thread_ = std::thread(&TesterClient::AliveLoop, this);
  return Error::kOk;
}

Error TesterClient::Disconnect() {
  routing_active_ = false;
  cv_.notify_all();
  tcp_.Close();
  fd_ = -1;
  if (alive_thread_.joinable()) alive_thread_.join();
  if (reader_thread_.joinable()) reader_thread_.join();
  EmitState(Sink(), diag::ConnectionState::Disconnected);
  return Error::kOk;
}

Error TesterClient::Send(const diag::uds::Request& req,
                         diag::uds::Response& resp) {
  return SendDiagnostic(req, resp, target_, false);
}

Error TesterClient::SendFunctional(const diag::uds::Request& req,
                                   diag::uds::Response& resp) {
  return SendDiagnostic(req, resp, cfg_.functional_address, true);
}

Error TesterClient::SendDiagnostic(const diag::uds::Request& req,
                                   diag::uds::Response& resp,
                                   LogicalAddress target, bool functional) {
  if (fd_ < 0) return Error::kNotRunning;

  const std::vector<uint8_t>& pdu = req.pdu();
  if (!SendMessage(PayloadType::DiagnosticMessage,
                   BuildDiagnosticMessage(cfg_.logical_address, target,
                                          pdu.data(), pdu.size()))) {
    return Error::kConnectionClosed;
  }

  if (functional) {
    auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    while (std::chrono::steady_clock::now() < deadline) {
      auto now = std::chrono::steady_clock::now();
      auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
          deadline - now);
      Message msg;
      Error err = WaitForAny(
          {PayloadType::DiagnosticMessage,
           PayloadType::DiagnosticMessageNegativeAck}, msg, remaining);
      if (err == Error::kTimeout) {
        resp = diag::uds::Response();
        return Error::kOk;
      }
      if (err != Error::kOk) return err;
      if (msg.type == PayloadType::DiagnosticMessageNegativeAck) {
        return Error::kDoipNack;
      }

      uint16_t source = 0;
      uint16_t target_addr = 0;
      std::vector<uint8_t> uds;
      if (!ParseDiagnosticMessage(msg.payload, source, target_addr, uds) ||
          uds.empty()) {
        return Error::kInternal;
      }
      resp = diag::uds::Response(std::move(uds));
      return Error::kOk;
    }
    resp = diag::uds::Response();
    return Error::kOk;
  }

  auto deadline = std::chrono::steady_clock::now() + cfg_.timing.p2_star;
  while (true) {
    auto now = std::chrono::steady_clock::now();
    if (now >= deadline) return Error::kTimeout;
    auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
        deadline - now);

    Message msg;
    Error err = WaitForAny(
        {PayloadType::DiagnosticMessage,
         PayloadType::DiagnosticMessageNegativeAck},
        msg, remaining);
    if (err != Error::kOk) return err;

    if (msg.type == PayloadType::DiagnosticMessageNegativeAck) {
      return Error::kDoipNack;
    }

    uint16_t source = 0;
    uint16_t target = 0;
    std::vector<uint8_t> uds;
    if (!ParseDiagnosticMessage(msg.payload, source, target, uds) ||
        uds.empty()) {
      return Error::kInternal;
    }

    diag::uds::Response candidate(std::move(uds));
    if (candidate.is_pending()) {
      if (auto sink = Sink()) {
        sink->onLog(diag::LogLevel::Info,
                    "UDS response pending (0x78), waiting for final response");
      }
      continue;
    }
    resp = std::move(candidate);
    return Error::kOk;
  }
}

void TesterClient::ReaderLoop() {
  while (running_) {
    uint8_t hdr[kHeaderSize];
    std::ptrdiff_t got = diag::net::RecvAll(fd_, hdr, sizeof(hdr));
    if (got != static_cast<std::ptrdiff_t>(sizeof(hdr))) break;

    uint32_t len = (static_cast<uint32_t>(hdr[4]) << 24) |
                   (static_cast<uint32_t>(hdr[5]) << 16) |
                   (static_cast<uint32_t>(hdr[6]) << 8) |
                   static_cast<uint32_t>(hdr[7]);
    if (len > (1u << 20)) break;

    std::vector<uint8_t> full(kHeaderSize + len);
    std::copy(hdr, hdr + kHeaderSize, full.begin());
    if (len > 0) {
      got = diag::net::RecvAll(fd_, full.data() + kHeaderSize, len);
      if (got != static_cast<std::ptrdiff_t>(len)) break;
    }

    Message msg;
    if (!Decode(full.data(), full.size(), msg)) continue;
    EmitMessage(Sink(), msg, false);

    if (msg.type == PayloadType::AliveCheckResponse) {
      std::lock_guard<std::mutex> lk(mu_);
      last_alive_ = std::chrono::steady_clock::now();
      continue;
    }

    if (msg.type == PayloadType::DiagnosticMessagePositiveAck) {
      continue;
    }

    {
      std::lock_guard<std::mutex> lk(mu_);
      rx_queue_.push_back(msg);
    }
    cv_.notify_all();
  }
}

void TesterClient::AliveLoop() {
  while (running_ && routing_active_) {
    for (int i = 0; i < 10 && running_ && routing_active_; ++i) {
      std::this_thread::sleep_for(cfg_.timing.alive_check / 10);
    }
    if (!running_ || !routing_active_) break;

    if (!SendMessage(PayloadType::AliveCheckRequest,
                     BuildAliveCheck(cfg_.logical_address))) {
      break;
    }

    std::lock_guard<std::mutex> lk(mu_);
    auto now = std::chrono::steady_clock::now();
    if (now - last_alive_ > cfg_.timing.inactivity) {
      routing_active_ = false;
    }
  }
  routing_active_ = false;
}

}  // namespace diag::doip
