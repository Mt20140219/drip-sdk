#include "test.h"

#include <cstdint>
#include <vector>

#include "diag/doip/protocol.h"
#include "doip/codec.h"

namespace codec_tests {

void Run() {
  using namespace diag::doip;

  Message msg{ProtocolVersion::Iso13400_2012,
              PayloadType::VehicleIdentificationRequest, {}};
  std::vector<uint8_t> bytes = Encode(msg);
  EXPECT_EQ(bytes.size(), 8u);
  EXPECT_EQ(bytes[0], 0x02);
  EXPECT_EQ(bytes[1], 0xFD);
  EXPECT_EQ(bytes[3], 0x01);

  Message out;
  EXPECT(Decode(bytes.data(), bytes.size(), out));
  EXPECT_EQ(static_cast<int>(out.type),
            static_cast<int>(PayloadType::VehicleIdentificationRequest));

  bytes[1] = 0x00;  // Broken inverse-version byte.
  EXPECT(!Decode(bytes.data(), bytes.size(), out));
  bytes[1] = 0xFD;

  EXPECT(!Decode(bytes.data(), 7, out));  // Truncated header.

  diag::VehicleInfo info;
  info.vin = "ABCDEFGHIJKLMNOPQ";
  info.logical_address = 0x0E80;
  info.eid = {1, 2, 3, 4, 5, 6};
  info.gid = {7, 8, 9, 10, 11, 12};

  std::vector<uint8_t> payload = BuildVehicleIdResponse(info);
  EXPECT_EQ(payload.size(), 32u);
  diag::VehicleInfo parsed;
  EXPECT(ParseVehicleIdResponse(payload, parsed));
  EXPECT_EQ(parsed.vin, info.vin);
  EXPECT_EQ(parsed.logical_address, 0x0E80);

  std::vector<uint8_t> req = BuildRoutingActivationRequest(
      0x0E00, ActivationType::DefaultCentralSecurity);
  uint16_t source = 0;
  ActivationType type = ActivationType::DefaultCentralSecurity;
  EXPECT(ParseRoutingActivationRequest(req, source, type));
  EXPECT_EQ(source, 0x0E00);

  std::vector<uint8_t> resp =
      BuildRoutingActivationResponse(0x0E00, 0x0E80, 0x10);
  uint16_t tester = 0;
  uint16_t entity = 0;
  uint8_t code = 0;
  EXPECT(ParseRoutingActivationResponse(resp, tester, entity, code));
  EXPECT_EQ(code, 0x10);
  EXPECT_EQ(entity, 0x0E80);

  uint8_t uds[] = {0x3E, 0x00};
  std::vector<uint8_t> dm =
      BuildDiagnosticMessage(0x0E00, 0x0E80, uds, sizeof(uds));
  uint16_t s = 0;
  uint16_t t = 0;
  std::vector<uint8_t> u;
  EXPECT(ParseDiagnosticMessage(dm, s, t, u));
  EXPECT_EQ(u.size(), 2u);
  EXPECT_EQ(u[0], 0x3E);
  EXPECT_EQ(u[1], 0x00);
}

}  // namespace codec_tests
