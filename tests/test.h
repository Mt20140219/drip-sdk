#pragma once

#include <cstdio>

namespace test {

inline int g_checks = 0;
inline int g_failures = 0;

inline void Report(bool ok, const char* expr, const char* file, int line) {
  ++g_checks;
  if (!ok) {
    ++g_failures;
    std::fprintf(stderr, "FAIL %s:%d: %s\n", file, line, expr);
  }
}

inline int Summary() {
  std::printf("%d checks, %d failures\n", g_checks, g_failures);
  return g_failures == 0 ? 0 : 1;
}

}  // namespace test

#define EXPECT(expr) ::test::Report((expr), #expr, __FILE__, __LINE__)
#define EXPECT_EQ(a, b) \
  ::test::Report(((a) == (b)), #a " == " #b, __FILE__, __LINE__)
