#pragma once

#include <cstdint>

namespace diag::doip {

enum class ProtocolVersion : uint8_t {
  Iso13400_2012 = 0x02,
  Iso13400_2019 = 0x03,
};

constexpr uint8_t InverseVersion(ProtocolVersion v) {
  return static_cast<uint8_t>(~static_cast<uint8_t>(v));
}

enum class ActivationType : uint8_t {
  DefaultCentralSecurity = 0x00,
  Oem = 0xE0,
};

enum class PayloadType : uint16_t {
  GenericNegativeAck = 0x0000,
  VehicleIdentificationRequest = 0x0001,
  VehicleIdentificationRequestWithEid = 0x0002,
  VehicleIdentificationRequestWithVin = 0x0003,
  VehicleAnnouncementOrIdentificationResponse = 0x0004,
  RoutingActivationRequest = 0x0005,
  RoutingActivationResponse = 0x0006,
  AliveCheckRequest = 0x0007,
  AliveCheckResponse = 0x0008,
  DiagnosticMessage = 0x8001,
  DiagnosticMessagePositiveAck = 0x8002,
  DiagnosticMessageNegativeAck = 0x8003,
};

constexpr uint16_t kDefaultTesterAddress = 0x0E00;
constexpr uint16_t kDefaultEntityAddress = 0x0E80;
constexpr uint16_t kDefaultPort = 13400;

}  // namespace diag::doip
