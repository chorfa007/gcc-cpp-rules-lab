// P2996 Example 6 — Generate integer_sequence via template substitution
// godbolt.org/z/8odThYaxc
// Flags: -std=c++26 -freflection

#include <meta>
#include <utility>
#include <vector>

template<typename T>
consteval std::meta::info make_integer_seq_refl(T N) {
  std::vector args{^^T};
  for (T k = 0; k < N; ++k) {
    args.push_back(std::meta::reflect_constant(k));
  }
  return substitute(^^std::integer_sequence, args);
}

template<typename T, T N>
  using make_integer_sequence = [:make_integer_seq_refl<T>(N):];

static_assert(std::same_as<
    make_integer_sequence<int, 10>,
    std::make_integer_sequence<int, 10>
    >);

int main() {}
