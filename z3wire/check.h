#ifndef Z3WIRE_CHECK_H_
#define Z3WIRE_CHECK_H_

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace z3w::internal {

class CheckFailMessage {
 public:
  CheckFailMessage(const char* file, int line, const char* condition) {
    stream_ << "Z3Wire check failed: " << condition << " at " << file << ":"
            << line;
  }

  ~CheckFailMessage() {
    std::cerr << stream_.str() << "\n";
    std::abort();
  }

  std::ostringstream& stream() { return stream_; }

 private:
  std::ostringstream stream_;
};

// Swallows the ostream from operator<< so the ternary produces void on both
// branches.
class CheckVoidify {
 public:
  void operator&(std::ostream& /*unused*/) const {}
};

}  // namespace z3w::internal

// NOLINTBEGIN(bugprone-macro-parentheses)
#define Z3W_CHECK(condition)                                                  \
  (condition)                                                                 \
      ? (void)0                                                               \
      : ::z3w::internal::CheckVoidify() &                                     \
            ::z3w::internal::CheckFailMessage(__FILE__, __LINE__, #condition) \
                .stream()
// NOLINTEND(bugprone-macro-parentheses)

#endif  // Z3WIRE_CHECK_H_
