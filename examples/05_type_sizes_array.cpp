// P2996 Example 5 — Compile-time type array + size_of via reflection
// godbolt.org/z/vcxqWxr19
// Flags: -std=c++26 -freflection

#include <meta>
#include <array>
#include <algorithm>

constexpr std::array types = {^^int, ^^float, ^^double};

constexpr std::array sizes = []() {
  std::array<std::size_t, types.size()> r;
  std::transform(types.begin(), types.end(), r.begin(), std::meta::size_of);
  return r;
}();

static_assert(sizes[0] == sizeof(int));
static_assert(sizes[1] == sizeof(float));
static_assert(sizes[2] == sizeof(double));

int main() {}
