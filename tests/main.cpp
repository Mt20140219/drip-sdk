#include "test.h"

namespace buffer_tests {
void Run();
}

namespace codec_tests {
void Run();
}

namespace uds_tests {
void Run();
}

int main() {
  buffer_tests::Run();
  codec_tests::Run();
  uds_tests::Run();
  return test::Summary();
}
