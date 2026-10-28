#include "test.h"

#include <cstdint>
#include <stdexcept>

#include "diag/util/byte_buffer.h"

namespace buffer_tests {

void Run() {
  diag::util::ByteBuffer b;
  b.put_u8(0x12);
  b.put_u16(0x3456);
  b.put_u32(0x789ABCDE);

  EXPECT_EQ(b.size(), 7u);
  EXPECT_EQ(b.get_u8(), 0x12);
  EXPECT_EQ(b.get_u16(), 0x3456);
  EXPECT_EQ(b.get_u32(), 0x789ABCDEu);
  EXPECT_EQ(b.remaining(), 0u);

  bool threw = false;
  try {
    b.get_u8();
  } catch (const std::out_of_range&) {
    threw = true;
  }
  EXPECT(threw);
}

}  // namespace buffer_tests
