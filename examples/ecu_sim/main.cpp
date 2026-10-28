#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <memory>
#include <string_view>
#include <thread>
#include <vector>

#include "diag/node_ecu.h"

namespace {

std::atomic<bool> g_stop{false};

void HandleSignal(int) { g_stop = true; }

struct ConsoleSink : diag::EventSink {
  void onLog(diag::LogLevel, std::string_view message) override {
    std::printf("%.*s\n", static_cast<int>(message.size()), message.data());
    std::fflush(stdout);
  }

  void onConnectionState(diag::ConnectionState state) override {
    static const char* names[] = {"Disconnected", "Connecting", "RoutingActive",
                                  "Closing"};
    std::printf("[ecu_sim] connection: %s\n", names[static_cast<int>(state)]);
    std::fflush(stdout);
  }
};

}  // namespace

int main() {
  std::signal(SIGINT, HandleSignal);
  std::signal(SIGTERM, HandleSignal);

  diag::NodeConfig cfg;
  cfg.role = diag::Role::Entity;
  cfg.bind_address = "0.0.0.0";
  cfg.tcp_port = 13400;
  cfg.udp_port = 13400;
  cfg.logical_address = 0x0E80;
  cfg.vehicle.vin = "LEARNTESTVIN00001";
  cfg.vehicle.logical_address = cfg.logical_address;
  cfg.vehicle.eid = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
  cfg.vehicle.gid = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};

  diag::EcuNode node(cfg);
  node.setSink(std::make_shared<ConsoleSink>());

  const char* vin = "LEARNTESTVIN00001";
  node.setDidValue(0xF190, std::vector<uint8_t>(vin, vin + 17));
  const char* serial = "SN12345678";
  node.setDidValue(0xF18A, std::vector<uint8_t>(serial, serial + 10));
  node.setDtc(0x123456, 0x2F);
  node.setDtc(0x654321, 0x08);

  if (node.start() != diag::Error::kOk) {
    std::fprintf(stderr, "ecu_sim: failed to start\n");
    return 1;
  }

  std::printf("ecu_sim: running (Ctrl-C to stop)\n");
  std::fflush(stdout);
  while (!g_stop) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  node.stop();
  std::printf("ecu_sim: stopped\n");
  return 0;
}
