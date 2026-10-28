#include "uds/service_layer.h"

#include <algorithm>
#include <chrono>

#include "diag/uds/constants.h"

namespace diag::uds {
namespace {

std::vector<uint8_t> Negative(uint8_t sid, uint8_t nrc) {
  return {0x7F, sid, nrc};
}

void PutU16(std::vector<uint8_t>& v, uint16_t u) {
  v.push_back(static_cast<uint8_t>((u >> 8) & 0xFF));
  v.push_back(static_cast<uint8_t>(u & 0xFF));
}

uint16_t ReadU16(const std::vector<uint8_t>& p, std::size_t off) {
  return static_cast<uint16_t>((p[off] << 8) | p[off + 1]);
}

constexpr uint16_t kDemoPendingDid = 0xF19E;
constexpr uint16_t kSeedKeyMask = 0x5AA5;
constexpr int kMaxAttempts = 3;

}  // namespace

void DataStore::SetDidValue(uint16_t did, std::vector<uint8_t> value) {
  std::lock_guard<std::mutex> lk(mu_);
  dids_[did] = std::move(value);
}

void DataStore::SetDtc(uint32_t dtc, uint8_t status) {
  std::lock_guard<std::mutex> lk(mu_);
  for (auto& r : dtcs_) {
    if (r.dtc == dtc) {
      r.status = status;
      return;
    }
  }
  dtcs_.push_back({dtc, status});
}

void DataStore::ClearDtcs() {
  std::lock_guard<std::mutex> lk(mu_);
  dtcs_.clear();
}

std::vector<uint8_t> DataStore::GetDidValue(uint16_t did) const {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = dids_.find(did);
  if (it == dids_.end()) return {};
  return it->second;
}

std::vector<DtcRecord> DataStore::GetDtcs() const {
  std::lock_guard<std::mutex> lk(mu_);
  return dtcs_;
}

HandleResult Session::Handle(const std::vector<uint8_t>& pdu) {
  if (pdu.empty()) return {Negative(0x00, 0x13), false, {}};
  uint8_t sid = pdu[0];

  switch (sid) {
    case sid::kDiagnosticSessionControl: {
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};
      uint8_t sub = pdu[1];
      if (sub != 0x01 && sub != 0x02 && sub != 0x03) {
        return {Negative(sid, 0x12), false, {}};
      }
      state_.session = sub;
      state_.security_level = 0x00;
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40), sub};
      PutU16(r, 2000);  // P2
      PutU16(r, 5000);  // P2*
      return {r, false, {}};
    }

    case sid::kTesterPresent: {
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};
      if (pdu[1] == 0x00) {
        return {{static_cast<uint8_t>(sid | 0x40), 0x00}, false, {}};
      }
      return {Negative(sid, 0x12), false, {}};
    }

    case sid::kReadDataByIdentifier: {
      if (pdu.size() < 3) return {Negative(sid, 0x13), false, {}};
      uint16_t did = ReadU16(pdu, 1);

      if (did == kDemoPendingDid) {
        std::vector<uint8_t> r{0x62, 0xF1, 0x9E, 'D', 'O', 'N', 'E'};
        return {r, true, std::chrono::milliseconds(300)};
      }

      std::vector<uint8_t> value = store_->GetDidValue(did);
      if (value.empty()) return {Negative(sid, 0x31), false, {}};

      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40),
                             static_cast<uint8_t>((did >> 8) & 0xFF),
                             static_cast<uint8_t>(did & 0xFF)};
      r.insert(r.end(), value.begin(), value.end());
      return {r, false, {}};
    }

    case sid::kReadDtcInformation: {
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};
      uint8_t sub = pdu[1];
      std::vector<DtcRecord> dtcs = store_->GetDtcs();

      if (sub == 0x02) {
        if (pdu.size() < 3) return {Negative(sid, 0x13), false, {}};
        std::vector<uint8_t> r{0x59, 0x02, 0xFF, 0x01};
        for (const auto& d : dtcs) {
          r.push_back(static_cast<uint8_t>((d.dtc >> 16) & 0xFF));
          r.push_back(static_cast<uint8_t>((d.dtc >> 8) & 0xFF));
          r.push_back(static_cast<uint8_t>(d.dtc & 0xFF));
          r.push_back(d.status);
        }
        return {r, false, {}};
      }

      if (sub == 0x0A) {
        std::vector<uint8_t> r{0x59, 0x0A, 0xFF, 0x01};
        for (const auto& d : dtcs) {
          r.push_back(static_cast<uint8_t>((d.dtc >> 16) & 0xFF));
          r.push_back(static_cast<uint8_t>((d.dtc >> 8) & 0xFF));
          r.push_back(static_cast<uint8_t>(d.dtc & 0xFF));
        }
        return {r, false, {}};
      }

      if (sub == 0x01) {
        if (pdu.size() < 3) return {Negative(sid, 0x13), false, {}};
        std::vector<uint8_t> r{0x59, 0x01, 0xFF, 0x01};
        PutU16(r, static_cast<uint16_t>(dtcs.size()));
        return {r, false, {}};
      }

      return {Negative(sid, 0x12), false, {}};
    }

    case sid::kSecurityAccess: {
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};
      uint8_t sub = pdu[1];

      if (sub == 0x01) {  // requestSeed
        auto now = std::chrono::steady_clock::now();
        if (state_.lockout_until.time_since_epoch().count() != 0 &&
            now < state_.lockout_until) {
          return {Negative(sid, 0x37), false, {}};
        }
        state_.seed = 0x1234;
        state_.attempts = 0;
        std::vector<uint8_t> r{0x67, 0x01,
                               static_cast<uint8_t>((state_.seed >> 8) & 0xFF),
                               static_cast<uint8_t>(state_.seed & 0xFF)};
        return {r, false, {}};
      }

      if (sub == 0x02) {  // sendKey
        if (pdu.size() < 4) return {Negative(sid, 0x13), false, {}};
        if (state_.attempts >= kMaxAttempts) {
          return {Negative(sid, 0x36), false, {}};
        }
        uint16_t key = ReadU16(pdu, 2);
        uint16_t expected =
            static_cast<uint16_t>((state_.seed ^ kSeedKeyMask) & 0xFFFF);
        if (key == expected) {
          state_.security_level = 0x01;
          state_.attempts = 0;
          return {{0x67, 0x02}, false, {}};
        }
        ++state_.attempts;
        if (state_.attempts >= kMaxAttempts) {
          state_.lockout_until =
              std::chrono::steady_clock::now() + std::chrono::seconds(10);
          return {Negative(sid, 0x36), false, {}};
        }
        return {Negative(sid, 0x35), false, {}};
      }

      return {Negative(sid, 0x12), false, {}};
    }

    case sid::kEcuReset: {
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};
      if (pdu[1] == 0x01 || pdu[1] == 0x03) {
        return {{static_cast<uint8_t>(sid | 0x40), pdu[1]}, false, {}};
      }
      return {Negative(sid, 0x12), false, {}};
    }

    case sid::kClearDiagnosticInformation: {
      if (pdu.size() < 4) return {Negative(sid, 0x13), false, {}};
      store_->ClearDtcs();
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40)};
      r.insert(r.end(), pdu.begin() + 1, pdu.begin() + 4);
      return {r, false, {}};
    }

    case sid::kCommunicationControl: {
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};
      uint8_t sub = pdu[1];
      if (sub > 0x03) return {Negative(sid, 0x12), false, {}};
      state_.comm_enabled = (sub == 0x00);
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40), sub};
      return {r, false, {}};
    }

    case sid::kWriteDataByIdentifier: {
      if (pdu.size() < 4) return {Negative(sid, 0x13), false, {}};
      uint16_t did = ReadU16(pdu, 1);
      std::vector<uint8_t> value(pdu.begin() + 3, pdu.end());
      store_->SetDidValue(did, value);
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40),
                             static_cast<uint8_t>((did >> 8) & 0xFF),
                             static_cast<uint8_t>(did & 0xFF)};
      return {r, false, {}};
    }

    case sid::kInputOutputControlByIdentifier: {
      if (pdu.size() < 3) return {Negative(sid, 0x13), false, {}};
      uint16_t did = ReadU16(pdu, 1);
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40),
                             static_cast<uint8_t>((did >> 8) & 0xFF),
                             static_cast<uint8_t>(did & 0xFF)};
      r.insert(r.end(), pdu.begin() + 3, pdu.end());
      return {r, false, {}};
    }

    case sid::kRoutineControl: {
      if (pdu.size() < 4) return {Negative(sid, 0x13), false, {}};
      uint8_t sub = pdu[1];
      if (sub != 0x01 && sub != 0x02 && sub != 0x03) {
        return {Negative(sid, 0x12), false, {}};
      }
      uint16_t routine = ReadU16(pdu, 2);
      uint8_t status = (sub == 0x01) ? 0x01 : ((sub == 0x02) ? 0x02 : 0x03);
      state_.routines[routine] = status;
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40), sub,
                             static_cast<uint8_t>((routine >> 8) & 0xFF),
                             static_cast<uint8_t>(routine & 0xFF), status};
      if (sub == 0x03) {
        r.insert(r.end(), pdu.begin() + 4, pdu.end());
      }
      return {r, false, {}};
    }

    case sid::kRequestDownload: {
      if (pdu.size() < 4) return {Negative(sid, 0x13), false, {}};
      uint8_t alf = pdu[2];
      std::size_t addr_len = static_cast<std::size_t>((alf >> 4) & 0x0F);
      std::size_t size_len = static_cast<std::size_t>(alf & 0x0F);
      std::size_t expected = 3 + addr_len + size_len;
      if (pdu.size() < expected) return {Negative(sid, 0x13), false, {}};

      uint32_t total = 0;
      for (std::size_t i = 0; i < size_len; ++i) {
        total = (total << 8) | pdu[3 + addr_len + i];
      }
      state_.transfer = {true, total, 0, 0};

      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40), 0x00, 0xFF};
      return {r, false, {}};
    }

    case sid::kTransferData: {
      if (!state_.transfer.active) return {Negative(sid, 0x24), false, {}};
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};

      uint8_t counter = pdu[1];
      std::vector<uint8_t> data(pdu.begin() + 2, pdu.end());
      state_.transfer.received += static_cast<uint32_t>(data.size());
      state_.transfer.block = counter;

      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40), counter};
      r.insert(r.end(), data.begin(), data.end());
      if (state_.transfer.received >= state_.transfer.total) {
        state_.transfer.active = false;
      }
      return {r, false, {}};
    }

    case sid::kRequestTransferExit: {
      if (!state_.transfer.active) return {Negative(sid, 0x24), false, {}};
      state_.transfer.active = false;
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40)};
      r.insert(r.end(), pdu.begin() + 1, pdu.end());
      return {r, false, {}};
    }

    case sid::kControlDtcSetting: {
      if (pdu.size() < 2) return {Negative(sid, 0x13), false, {}};
      uint8_t sub = pdu[1];
      if (sub != 0x01 && sub != 0x02) return {Negative(sid, 0x12), false, {}};
      state_.dtc_setting = sub;
      std::vector<uint8_t> r{static_cast<uint8_t>(sid | 0x40), sub};
      return {r, false, {}};
    }

    default:
      return {Negative(sid, 0x11), false, {}};
  }
}

}  // namespace diag::uds
