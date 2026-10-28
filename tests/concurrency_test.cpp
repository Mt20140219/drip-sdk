#include "test.h"

#include <cstdint>
#include <thread>
#include <vector>

#include "diag/uds/constants.h"
#include "uds/service_layer.h"

namespace concurrency_tests {

void Run() {
  diag::uds::DataStore store;
  constexpr int kThreads = 4;
  constexpr int kIters = 2000;

  std::vector<std::thread> threads;
  threads.reserve(kThreads);
  for (int t = 0; t < kThreads; ++t) {
    threads.emplace_back([&store, t]() {
      diag::uds::Session session(&store);
      for (int i = 0; i < kIters; ++i) {
        uint8_t value = static_cast<uint8_t>((t * 7 + i) & 0xFF);
        session.Handle({0x2E, 0xF1, 0x90, value});
        session.Handle({0x22, 0xF1, 0x90});
        if ((i % 100) == 0) {
          store.SetDtc(0x100000 + static_cast<uint32_t>(t),
                       static_cast<uint8_t>(0x20 | (i & 0x0F)));
        }
      }
    });
  }
  for (auto& th : threads) th.join();

  std::vector<uint8_t> value = store.GetDidValue(0xF190);
  EXPECT_EQ(value.size(), 1u);
  EXPECT_EQ(store.GetDtcs().size(), static_cast<std::size_t>(kThreads));
}

}  // namespace concurrency_tests
