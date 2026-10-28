#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

#include "diag/node_tester.h"
#include "diag/uds/constants.h"
#include "diag/uds/request.h"
#include "diag/uds/response.h"

namespace {

struct ConsoleSink : diag::EventSink {
  void onLog(diag::LogLevel, std::string_view message) override {
    std::printf("%.*s\n", static_cast<int>(message.size()), message.data());
    std::fflush(stdout);
  }

  void onConnectionState(diag::ConnectionState state) override {
    static const char* names[] = {"Disconnected", "Connecting", "RoutingActive",
                                  "Closing"};
    std::printf("[tester] connection: %s\n", names[static_cast<int>(state)]);
    std::fflush(stdout);
  }
};

void PrintResponse(const diag::uds::Response& resp) {
  if (resp.empty()) {
    std::printf("  (no response)\n");
    return;
  }
  std::printf("  SID=0x%02X %s", resp.sid(),
              resp.is_negative() ? "negative" : "positive");
  if (resp.is_negative()) std::printf(" NRC=0x%02X", resp.nrc());
  std::printf("\n");
}

}  // namespace

int main() {
  diag::NodeConfig cfg;
  cfg.role = diag::Role::Tester;
  cfg.bind_address = "0.0.0.0";
  cfg.tcp_port = 13400;
  cfg.udp_port = 13400;
  cfg.logical_address = 0x0E00;

  diag::TesterNode node(cfg);
  node.setSink(std::make_shared<ConsoleSink>());
  if (node.start() != diag::Error::kOk) {
    std::fprintf(stderr, "tester: failed to start\n");
    return 1;
  }

  std::vector<diag::VehicleInfo> vehicles;
  diag::Error err = node.discover(vehicles, std::chrono::seconds(2));
  if (err != diag::Error::kOk) {
    std::fprintf(stderr, "tester: discover failed: %s\n",
                 diag::ErrorToString(err));
    node.stop();
    return 1;
  }
  std::printf("tester: discovered %zu ECU(s)\n", vehicles.size());
  for (const auto& v : vehicles) {
    std::printf("tester:   VIN=%s LA=0x%04X\n", v.vin.c_str(),
                v.logical_address);
  }

  err = node.connect("127.0.0.1", 0x0E80);
  if (err != diag::Error::kOk) {
    std::fprintf(stderr, "tester: connect failed: %s\n",
                 diag::ErrorToString(err));
    node.stop();
    return 1;
  }

  err = node.routingActivate(diag::doip::ActivationType::DefaultCentralSecurity);
  if (err != diag::Error::kOk) {
    std::fprintf(stderr, "tester: routing activation failed: %s\n",
                 diag::ErrorToString(err));
    node.stop();
    return 1;
  }
  std::printf("tester: routing active\n");
  std::fflush(stdout);

  auto send = [&](diag::uds::Request& req) {
    diag::uds::Response resp;
    diag::Error e = node.send(req, resp);
    std::printf("  send SID=0x%02X -> %s\n", req.sid(),
                diag::ErrorToString(e));
    if (e == diag::Error::kOk) PrintResponse(resp);
    return resp;
  };

  std::printf("tester: switch to extended session (10 03)\n");
  {
    diag::uds::Request req(diag::uds::sid::kDiagnosticSessionControl);
    req.push_u8(0x03);
    send(req);
  }

  std::printf("tester: read VIN (22 F190)\n");
  {
    diag::uds::Request req(diag::uds::sid::kReadDataByIdentifier);
    req.push_u16(0xF190);
    diag::uds::Response resp = send(req);
    if (!resp.is_negative() && resp.pdu().size() > 3) {
      const auto& p = resp.pdu();
      std::string vin(p.begin() + 3, p.end());
      std::printf("  VIN=%s\n", vin.c_str());
    }
  }

  std::printf("tester: read DTC by status mask (19 02)\n");
  {
    diag::uds::Request req(diag::uds::sid::kReadDtcInformation);
    req.push_u8(0x02);
    req.push_u8(0xFF);
    diag::uds::Response resp = send(req);
    if (!resp.is_negative() && resp.pdu().size() >= 4) {
      std::size_t count = (resp.pdu().size() - 4) / 4;
      std::printf("  DTC count=%zu\n", count);
    }
  }

  std::printf("tester: security access (27)\n");
  {
    diag::uds::Request req(diag::uds::sid::kSecurityAccess);
    req.push_u8(0x01);
    diag::uds::Response resp = send(req);
    if (!resp.is_negative() && resp.pdu().size() >= 4) {
      uint16_t seed =
          static_cast<uint16_t>((resp.pdu()[2] << 8) | resp.pdu()[3]);
      std::printf("  seed=0x%04X\n", seed);

      diag::uds::Request wrong(diag::uds::sid::kSecurityAccess);
      wrong.push_u8(0x02);
      wrong.push_u16(0xFFFF);
      std::printf("  wrong key attempt\n");
      send(wrong);

      uint16_t key = static_cast<uint16_t>((seed ^ 0x5AA5) & 0xFFFF);
      diag::uds::Request right(diag::uds::sid::kSecurityAccess);
      right.push_u8(0x02);
      right.push_u16(key);
      std::printf("  correct key 0x%04X\n", key);
      send(right);
    }
  }

  std::printf("tester: read pending DID (22 F19E)\n");
  {
    diag::uds::Request req(diag::uds::sid::kReadDataByIdentifier);
    req.push_u16(0xF19E);
    diag::uds::Response resp = send(req);
    if (!resp.is_negative() && resp.pdu().size() > 3) {
      const auto& p = resp.pdu();
      std::string data(p.begin() + 3, p.end());
      std::printf("  data=%s\n", data.c_str());
    }
  }

  std::printf("tester: tester present (3E 00)\n");
  {
    diag::uds::Request req(diag::uds::sid::kTesterPresent);
    req.push_u8(0x00);
    send(req);
  }

  std::printf("tester: ECU reset (11 01)\n");
  {
    diag::uds::Request req(diag::uds::sid::kEcuReset);
    req.push_u8(0x01);
    send(req);
  }

  std::printf("tester: keeping routing active for 2s (alive checks)\n");
  std::fflush(stdout);
  std::this_thread::sleep_for(std::chrono::seconds(2));

  node.disconnect();
  node.stop();
  std::printf("tester: done\n");
  return 0;
}
