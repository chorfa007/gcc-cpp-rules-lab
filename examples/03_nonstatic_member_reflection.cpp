// P2996 Example 3 — nonstatic_data_members_of + access_context
// godbolt.org/z/c1M5zjTTh
// Flags: -std=c++26 -freflection

#include <meta>
#include <iostream>

struct S { unsigned i:2, j:6; };

consteval auto member_number(int n) {
  auto ctx = std::meta::access_context::current();
  return std::meta::nonstatic_data_members_of(^^S, ctx)[n];
}

int main() {
  S s{0, 0};
  s.[:member_number(1):] = 42;  // Same as: s.j = 42;
  // s.[:member_number(5):] = 0;   // Error (member_number(5) is not a constant).
  std::cout << "s=S{i=" << s.i << ", j=" << s.j << "}\n";
}
