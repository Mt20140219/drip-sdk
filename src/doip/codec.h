#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "diag/doip/protocol.h"
#include "diag/types.h"

namespace diag::doip {

constexpr std::size_t kHeaderSize = 8;

struct Message {
  ProtocolVersion version = ProtocolVersion::Iso13400_2012;
  PayloadType type = PayloadType::GenericNegativeAck;
  std::vector<uint8_t> payload;
};

std::vector<uint8_t> Encode(const Message& msg);
bool Decode(const uint8_t* data, std::size_t size, Message& out);

const char* PayloadTypeName(PayloadType type);

// 0x0004 vehicle identification response / vehicle announcement.
std::vector<uint8_t> BuildVehicleIdResponse(const VehicleInfo& info);
bool ParseVehicleIdResponse(const std::vector<uint8_t>& p, VehicleInfo& out);

// 0x0005 routing activation request.
std::vector<uint8_t> BuildRoutingActivationRequest(uint16_t source,
                                                   ActivationType type);
bool ParseRoutingActivationRequest(const std::vector<uint8_t>& p,
                                   uint16_t& source, ActivationType& type);

// 0x0006 routing activation response.
std::vector<uint8_t> BuildRoutingActivationResponse(uint16_t tester,
                                                    uint16_t entity,
                                                    uint8_t code);
bool ParseRoutingActivationResponse(const std::vector<uint8_t>& p,
                                    uint16_t& tester, uint16_t& entity,
                                    uint8_t& code);

// 0x0007 / 0x0008 alive check.
std::vector<uint8_t> BuildAliveCheck(uint16_t source);
bool ParseAliveCheck(const std::vector<uint8_t>& p, uint16_t& source);

// 0x8001 diagnostic message.
std::vector<uint8_t> BuildDiagnosticMessage(uint16_t source, uint16_t target,
                                            const uint8_t* data,
                                            std::size_t n);
bool ParseDiagnosticMessage(const std::vector<uint8_t>& p, uint16_t& source,
                            uint16_t& target, std::vector<uint8_t>& uds);

// 0x8002 positive acknowledgement.
std::vector<uint8_t> BuildDiagnosticAck(uint16_t source, uint16_t target,
                                        uint8_t code);
bool ParseDiagnosticAck(const std::vector<uint8_t>& p, uint16_t& source,
                        uint16_t& target, uint8_t& code);

// 0x8003 negative acknowledgement.
std::vector<uint8_t> BuildDiagnosticNack(uint16_t source, uint16_t target,
                                         uint8_t code);
bool ParseDiagnosticNack(const std::vector<uint8_t>& p, uint16_t& source,
                         uint16_t& target, uint8_t& code);

}  // namespace diag::doip
