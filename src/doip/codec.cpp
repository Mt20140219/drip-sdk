#include "doip/codec.h"

#include <algorithm>

#include "diag/util/byte_buffer.h"

namespace diag::doip {

namespace {

bool CheckSize(const std::vector<uint8_t>& p, std::size_t n) {
  return p.size() >= n;
}

}  // namespace

std::vector<uint8_t> Encode(const Message& msg) {
  std::vector<uint8_t> out;
  out.reserve(kHeaderSize + msg.payload.size());
  out.push_back(static_cast<uint8_t>(msg.version));
  out.push_back(InverseVersion(msg.version));
  out.push_back(
      static_cast<uint8_t>((static_cast<uint16_t>(msg.type) >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(static_cast<uint16_t>(msg.type) & 0xFF));

  uint32_t len = static_cast<uint32_t>(msg.payload.size());
  out.push_back(static_cast<uint8_t>((len >> 24) & 0xFF));
  out.push_back(static_cast<uint8_t>((len >> 16) & 0xFF));
  out.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
  out.push_back(static_cast<uint8_t>(len & 0xFF));

  out.insert(out.end(), msg.payload.begin(), msg.payload.end());
  return out;
}

bool Decode(const uint8_t* data, std::size_t size, Message& out) {
  if (!data || size < kHeaderSize) return false;

  uint8_t ver = data[0];
  uint8_t inv = data[1];
  if (static_cast<uint8_t>(~ver) != inv) return false;
  if (ver != 0x02 && ver != 0x03) return false;

  uint32_t len = (static_cast<uint32_t>(data[4]) << 24) |
                 (static_cast<uint32_t>(data[5]) << 16) |
                 (static_cast<uint32_t>(data[6]) << 8) |
                 static_cast<uint32_t>(data[7]);
  if (len != size - kHeaderSize) return false;

  out.version = static_cast<ProtocolVersion>(ver);
  out.type = static_cast<PayloadType>(
      (static_cast<uint16_t>(data[2]) << 8) | data[3]);
  out.payload.assign(data + kHeaderSize, data + size);
  return true;
}

const char* PayloadTypeName(PayloadType type) {
  switch (type) {
    case PayloadType::GenericNegativeAck: return "GenericNACK";
    case PayloadType::VehicleIdentificationRequest: return "VehicleIdReq";
    case PayloadType::VehicleIdentificationRequestWithEid: return "VehicleIdReqEid";
    case PayloadType::VehicleIdentificationRequestWithVin: return "VehicleIdReqVin";
    case PayloadType::VehicleAnnouncementOrIdentificationResponse: return "VehicleIdResp";
    case PayloadType::RoutingActivationRequest: return "RoutingActivationReq";
    case PayloadType::RoutingActivationResponse: return "RoutingActivationResp";
    case PayloadType::AliveCheckRequest: return "AliveCheckReq";
    case PayloadType::AliveCheckResponse: return "AliveCheckResp";
    case PayloadType::DiagnosticMessage: return "DiagnosticMessage";
    case PayloadType::DiagnosticMessagePositiveAck: return "DiagnosticAck";
    case PayloadType::DiagnosticMessageNegativeAck: return "DiagnosticNack";
  }
  return "Unknown";
}

std::vector<uint8_t> BuildVehicleIdResponse(const VehicleInfo& info) {
  std::string vin = info.vin;
  if (vin.size() > 17) vin.resize(17);
  while (vin.size() < 17) vin.push_back(' ');

  diag::util::ByteBuffer b;
  b.put_bytes(reinterpret_cast<const uint8_t*>(vin.data()), 17);
  b.put_u16(info.logical_address);
  for (uint8_t v : info.eid) b.put_u8(v);
  for (uint8_t v : info.gid) b.put_u8(v);
  b.put_u8(0x00);  // Further action required.
  return b.data();
}

bool ParseVehicleIdResponse(const std::vector<uint8_t>& p, VehicleInfo& out) {
  if (p.size() < 32) return false;

  out.vin.assign(reinterpret_cast<const char*>(p.data()), 17);
  while (!out.vin.empty() && out.vin.back() == ' ') out.vin.pop_back();

  out.logical_address =
      static_cast<uint16_t>((p[17] << 8) | p[18]);
  std::copy(p.begin() + 19, p.begin() + 25, out.eid.begin());
  std::copy(p.begin() + 25, p.begin() + 31, out.gid.begin());
  return true;
}

std::vector<uint8_t> BuildRoutingActivationRequest(uint16_t source,
                                                   ActivationType type) {
  diag::util::ByteBuffer b;
  b.put_u16(source);
  b.put_u8(static_cast<uint8_t>(type));
  b.put_u32(0);  // Reserved in ISO 13400-2:2012.
  return b.data();
}

bool ParseRoutingActivationRequest(const std::vector<uint8_t>& p,
                                   uint16_t& source, ActivationType& type) {
  if (!CheckSize(p, 7)) return false;
  source = static_cast<uint16_t>((p[0] << 8) | p[1]);
  type = static_cast<ActivationType>(p[2]);
  return true;
}

std::vector<uint8_t> BuildRoutingActivationResponse(uint16_t tester,
                                                    uint16_t entity,
                                                    uint8_t code) {
  diag::util::ByteBuffer b;
  b.put_u16(tester);
  b.put_u16(entity);
  b.put_u8(code);
  b.put_u32(0);  // Reserved in ISO 13400-2:2012.
  return b.data();
}

bool ParseRoutingActivationResponse(const std::vector<uint8_t>& p,
                                    uint16_t& tester, uint16_t& entity,
                                    uint8_t& code) {
  if (!CheckSize(p, 9)) return false;
  tester = static_cast<uint16_t>((p[0] << 8) | p[1]);
  entity = static_cast<uint16_t>((p[2] << 8) | p[3]);
  code = p[4];
  return true;
}

std::vector<uint8_t> BuildAliveCheck(uint16_t source) {
  diag::util::ByteBuffer b;
  b.put_u16(source);
  return b.data();
}

bool ParseAliveCheck(const std::vector<uint8_t>& p, uint16_t& source) {
  if (!CheckSize(p, 2)) return false;
  source = static_cast<uint16_t>((p[0] << 8) | p[1]);
  return true;
}

std::vector<uint8_t> BuildDiagnosticMessage(uint16_t source, uint16_t target,
                                            const uint8_t* data,
                                            std::size_t n) {
  diag::util::ByteBuffer b;
  b.put_u16(source);
  b.put_u16(target);
  b.put_bytes(data, n);
  return b.data();
}

bool ParseDiagnosticMessage(const std::vector<uint8_t>& p, uint16_t& source,
                            uint16_t& target, std::vector<uint8_t>& uds) {
  if (!CheckSize(p, 4)) return false;
  source = static_cast<uint16_t>((p[0] << 8) | p[1]);
  target = static_cast<uint16_t>((p[2] << 8) | p[3]);
  uds.assign(p.begin() + 4, p.end());
  return true;
}

std::vector<uint8_t> BuildDiagnosticAck(uint16_t source, uint16_t target,
                                        uint8_t code) {
  diag::util::ByteBuffer b;
  b.put_u16(source);
  b.put_u16(target);
  b.put_u8(code);
  return b.data();
}

bool ParseDiagnosticAck(const std::vector<uint8_t>& p, uint16_t& source,
                        uint16_t& target, uint8_t& code) {
  if (!CheckSize(p, 5)) return false;
  source = static_cast<uint16_t>((p[0] << 8) | p[1]);
  target = static_cast<uint16_t>((p[2] << 8) | p[3]);
  code = p[4];
  return true;
}

std::vector<uint8_t> BuildDiagnosticNack(uint16_t source, uint16_t target,
                                         uint8_t code) {
  return BuildDiagnosticAck(source, target, code);
}

bool ParseDiagnosticNack(const std::vector<uint8_t>& p, uint16_t& source,
                         uint16_t& target, uint8_t& code) {
  return ParseDiagnosticAck(p, source, target, code);
}

}  // namespace diag::doip
