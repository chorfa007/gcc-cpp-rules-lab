// P2996 Example 1 — Basic type splicing with [: :]
// godbolt.org/z/eKWM1vrn8
// Flags: -std=c++26 -freflection

#include <meta>
#include <cassert>

int main() {
    constexpr auto r = ^^int;
    typename[:r:] x = 42;       // Same as: int x = 42;
    typename[:^^char:] c = '*';  // Same as: char c = '*';

    static_assert(std::same_as<decltype(x), int>);
    static_assert(std::same_as<decltype(c), char>);
    assert(x == 42);
    assert(c == '*');
}
