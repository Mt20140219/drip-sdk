#pragma once

#include <chrono>
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
  void SetDidValue(uint16_t did, std::vector<uint8_t> value);
  void SetDtc(uint32_t dtc, uint8_t status);
  void ClearDtcs();

  std::vector<uint8_t> GetDidValue(uint16_t did) const;
  std::vector<DtcRecord> GetDtcs() const;

 private:
  mutable std::mutex mu_;
  std::unordered_map<uint16_t, std::vector<uint8_t>> dids_;
  std::vector<DtcRecord> dtcs_;
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
    struct {
      bool active = false;
      uint32_t total = 0;
      uint32_t received = 0;
      uint8_t block = 0;
    } transfer;
  };

  DataStore* store_;
  State state_;
};

}  // namespace diag::uds
