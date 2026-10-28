#include "test.h"

#include <cstdint>
#include <vector>

#include "diag/uds/constants.h"
#include "uds/service_layer.h"

namespace uds_tests {

void Run() {
  diag::uds::DataStore store;
  std::vector<uint8_t> vin = {'L', 'E', 'A', 'R', 'N', 'T', 'E', 'S',
                              'T', 'V', 'I', 'N', '0', '0', '0', '0', '1'};
  store.SetDidValue(0xF190, vin);
  store.SetDtc(0x123456, 0x2F);
  store.SetDtc(0x654321, 0x08);

  diag::uds::Session session(&store);

  auto result = session.Handle({0x10, 0x03});
  EXPECT_EQ(result.pdu[0], 0x50);
  EXPECT_EQ(result.pdu[1], 0x03);
  EXPECT_EQ(session.active_session(), 0x03);

  result = session.Handle({0x10, 0x04});
  EXPECT_EQ(result.pdu[0], 0x7F);
  EXPECT_EQ(result.pdu[2], 0x12);

  result = session.Handle({0x22, 0xF1, 0x90});
  EXPECT_EQ(result.pdu[0], 0x62);
  EXPECT_EQ(result.pdu.size(), 3u + 17u);

  result = session.Handle({0x22, 0xAA, 0xBB});
  EXPECT_EQ(result.pdu[2], 0x31);

  result = session.Handle({0x22, 0xF1});
  EXPECT_EQ(result.pdu[2], 0x13);

  result = session.Handle({0x19, 0x02, 0xFF});
  EXPECT_EQ(result.pdu[0], 0x59);
  EXPECT_EQ((result.pdu.size() - 4) / 4, 2u);

  result = session.Handle({0x19, 0x0A});
  EXPECT_EQ(result.pdu[0], 0x59);
  EXPECT_EQ((result.pdu.size() - 4) / 3, 2u);

  result = session.Handle({0x27, 0x01});
  EXPECT_EQ(result.pdu[0], 0x67);
  uint16_t seed =
      static_cast<uint16_t>((result.pdu[2] << 8) | result.pdu[3]);
  EXPECT_EQ(seed, 0x1234);

  result = session.Handle({0x27, 0x02, 0xFF, 0xFF});
  EXPECT_EQ(result.pdu[2], 0x35);

  uint16_t key = static_cast<uint16_t>((seed ^ 0x5AA5) & 0xFFFF);
  result = session.Handle(
      {0x27, 0x02, static_cast<uint8_t>((key >> 8) & 0xFF),
       static_cast<uint8_t>(key & 0xFF)});
  EXPECT_EQ(result.pdu[0], 0x67);
  EXPECT_EQ(result.pdu[1], 0x02);
  EXPECT_EQ(session.security_level(), 0x01);

  session.Handle({0x27, 0x01});
  result = session.Handle({0x27, 0x02, 0x00, 0x00});
  EXPECT_EQ(result.pdu[2], 0x35);
  result = session.Handle({0x27, 0x02, 0x00, 0x00});
  EXPECT_EQ(result.pdu[2], 0x35);
  result = session.Handle({0x27, 0x02, 0x00, 0x00});
  EXPECT_EQ(result.pdu[2], 0x36);
  result = session.Handle({0x27, 0x01});
  EXPECT_EQ(result.pdu[2], 0x37);

  result = session.Handle({0x3E, 0x00});
  EXPECT_EQ(result.pdu[0], 0x7E);

  result = session.Handle({0x11, 0x01});
  EXPECT_EQ(result.pdu[0], 0x51);

  result = session.Handle({0x99});
  EXPECT_EQ(result.pdu[2], 0x11);

  result = session.Handle({0x22, 0xF1, 0x9E});
  EXPECT(result.pending_first);
  EXPECT_EQ(result.pdu[0], 0x62);
}

}  // namespace uds_tests
