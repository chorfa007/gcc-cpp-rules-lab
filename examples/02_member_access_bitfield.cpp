// P2996 Example 2 — Reflective member access on bit-fields
// godbolt.org/z/66eY94ac9
// Flags: -std=c++26 -freflection

#include <meta>
#include <iostream>
#include <cassert>
#include <utility>

struct S { unsigned i:2, j:6; };

consteval auto member_number(int n) {
  if (n == 0) return ^^S::i;
  else if (n == 1) return ^^S::j;

  std::unreachable();
}

int main() {
  S s{0, 0};
  s.[:member_number(1):] = 42;  // Same as: s.j = 42;

#if 0
  s.[:member_number(5):] = 0;   // Error (member_number(5) is not a constant).
#endif

  std::cout << "s.i=" << s.i << ", s.j=" << s.j << '\n';
}
