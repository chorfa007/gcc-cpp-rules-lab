// P2996 Example 4 — Find member by name at compile time
// godbolt.org/z/reEs9bja9
// Flags: -std=c++26 -freflection

#include <meta>
#include <iostream>
#include <utility>

struct S { unsigned : 1; unsigned i:2, j:6; };

consteval auto member_named(std::string_view name) {
  auto ctx = std::meta::access_context::current();
  for (std::meta::info field : nonstatic_data_members_of(^^S, ctx)) {
    if (has_identifier(field) && identifier_of(field) == name)
      return field;
  }
  std::unreachable();
}

int main() {
  S s{0, 0};
  s.[:member_named("j"):] = 42;  // Same as: s.j = 42;
  // s.[:member_named("x"):] = 0;   // Error (member_named("x") is not a constant).
  std::cout << "s=S{i=" << s.i << ", j=" << s.j << "}\n";
}
