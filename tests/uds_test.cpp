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

  diag::uds::DataStore store2;
  diag::uds::Session session2(&store2);
  store2.SetDtc(0x123456, 0x2F);

  result = session2.Handle({0x14, 0xFF, 0xFF, 0xFF});
  EXPECT_EQ(result.pdu[0], 0x54);
  EXPECT_EQ(store2.GetDtcs().size(), 0u);

  result = session2.Handle({0x28, 0x03});
  EXPECT_EQ(result.pdu[0], 0x68);

  result = session2.Handle({0x2E, 0xF1, 0x90, 'V', 'I', 'N'});
  EXPECT_EQ(result.pdu[0], 0x6E);
  result = session2.Handle({0x22, 0xF1, 0x90});
  EXPECT_EQ(result.pdu.size(), 6u);
  EXPECT_EQ(result.pdu[3], 'V');

  result = session2.Handle({0x2F, 0xF1, 0x90, 0x01});
  EXPECT_EQ(result.pdu[0], 0x6F);

  result = session2.Handle({0x31, 0x01, 0x12, 0x34});
  EXPECT_EQ(result.pdu[0], 0x71);
  EXPECT_EQ(result.pdu[4], 0x01);
  result = session2.Handle({0x31, 0x03, 0x12, 0x34});
  EXPECT_EQ(result.pdu[4], 0x03);

  result = session2.Handle({0x34, 0x00, 0x11, 0x00, 0x08});
  EXPECT_EQ(result.pdu[0], 0x74);
  result = session2.Handle({0x36, 0x01, 0xAA, 0xBB, 0xCC, 0xDD});
  EXPECT_EQ(result.pdu[0], 0x76);
  result = session2.Handle({0x37});
  EXPECT_EQ(result.pdu[0], 0x77);

  result = session2.Handle({0x85, 0x02});
  EXPECT_EQ(result.pdu[0], 0xC5);

  result = session2.Handle({0x3D, 0x11, 0x00, 0xAA, 0xBB, 0xCC});
  EXPECT_EQ(result.pdu[0], 0x7D);
  result = session2.Handle({0x23, 0x12, 0x00, 0x00, 0x03});
  EXPECT_EQ(result.pdu[0], 0x63);
  EXPECT_EQ(result.pdu.size(), 4u);
  EXPECT_EQ(result.pdu[1], 0xAA);
  EXPECT_EQ(result.pdu[3], 0xCC);

  store2.SetScalingData(0x7001, {0x01, 0x02});
  result = session2.Handle({0x24, 0x70, 0x01});
  EXPECT_EQ(result.pdu[0], 0x64);
  EXPECT_EQ(result.pdu[3], 0x01);

  result = session2.Handle({0x2C, 0x02, 0xF2, 0x00, 0x11, 0x00, 0x02});
  EXPECT_EQ(result.pdu[0], 0x6C);
  result = session2.Handle({0x22, 0xF2, 0x00});
  EXPECT_EQ(result.pdu[0], 0x62);
  EXPECT_EQ(result.pdu.size(), 5u);
  EXPECT_EQ(result.pdu[3], 0xAA);
  EXPECT_EQ(result.pdu[4], 0xBB);

  result = session2.Handle({0x35, 0x00, 0x11, 0x00, 0x04});
  EXPECT_EQ(result.pdu[0], 0x75);
  result = session2.Handle({0x36, 0x01});
  EXPECT_EQ(result.pdu[0], 0x76);
  EXPECT_EQ(result.pdu.size(), 6u);

  result = session2.Handle({0x29, 0x00});
  EXPECT_EQ(result.pdu[0], 0x69);

  result = session2.Handle({0x2A, 0x01, 0x02});
  EXPECT_EQ(result.pdu[0], 0x6A);

  result = session2.Handle({0x86, 0x00});
  EXPECT_EQ(result.pdu[0], 0xC6);

  result = session2.Handle({0x87, 0x01});
  EXPECT_EQ(result.pdu[0], 0xC7);
}

}  // namespace uds_tests
