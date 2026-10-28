#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <vector>

namespace diag::uds {

struct DtcRecord {
  uint32_t dtc = 0;
  uint8_t status = 0;
};

class DataStore {
 public:
  static constexpr std::size_t kMemorySize = 64 * 1024;

  void SetDidValue(uint16_t did, std::vector<uint8_t> value);
  void SetDtc(uint32_t dtc, uint8_t status);
  void ClearDtcs();

  std::vector<uint8_t> GetDidValue(uint16_t did) const;
  std::vector<DtcRecord> GetDtcs() const;

  bool ReadMemory(uint32_t address, uint32_t length,
                  std::vector<uint8_t>& out) const;
  bool WriteMemory(uint32_t address, const uint8_t* data, uint32_t length);

  void SetScalingData(uint16_t did, std::vector<uint8_t> value);
  std::vector<uint8_t> GetScalingData(uint16_t did) const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<uint16_t, std::vector<uint8_t>> dids_;
  std::vector<DtcRecord> dtcs_;
  std::vector<uint8_t> memory_ = std::vector<uint8_t>(kMemorySize, 0);
  std::unordered_map<uint16_t, std::vector<uint8_t>> scaling_;
};

struct HandleResult {
  std::vector<uint8_t> pdu;
  bool pending_first = false;
  std::chrono::milliseconds delay{0};
};

class Session {
 public:
  explicit Session(DataStore* store) : store_(store) {}

  // Takes a full UDS request PDU (starting with the SID) and returns the full
  // response PDU (positive SID+0x40 or negative 0x7F).
  HandleResult Handle(const std::vector<uint8_t>& pdu);

  uint8_t active_session() const { return state_.session; }
  uint8_t security_level() const { return state_.security_level; }

 private:
  struct State {
    uint8_t session = 0x01;
    uint8_t security_level = 0x00;
    uint16_t seed = 0;
    int attempts = 0;
    std::chrono::steady_clock::time_point lockout_until{};
    uint8_t dtc_setting = 0x01;
    bool comm_enabled = true;
    std::unordered_map<uint16_t, uint8_t> routines;
    std::unordered_map<uint16_t, std::pair<uint32_t, uint16_t>> dynamic_dids;
    struct {
      bool active = false;
      bool upload = false;
      uint32_t total = 0;
      uint32_t received = 0;
      uint8_t block = 0;
      uint32_t address = 0;
    } transfer;
  };

  DataStore* store_;
  State state_;
};

}  // namespace diag::uds
