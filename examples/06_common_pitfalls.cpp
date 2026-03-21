/**
 * Example 6 – Common Pitfalls
 *
 * Shows what goes wrong when you forget the rules, using
 * sanitizer-friendly code (no actual UB is triggered — we just
 * demonstrate the logical issues with explanations).
 *
 * Pitfalls covered:
 *   A. Forgetting copy ctor → double-free
 *   B. Forgetting move → silent copy inside std::vector resize
 *   C. Exception safety in assignment without copy-and-swap
 *   D. Accidentally disabling moves by declaring only a destructor
 */

#include <cassert>
#include <cstring>
#include <iostream>
#include <vector>
#include <type_traits>

// ══════════════════════════════════════════════════════════════════════════════
// A. Pitfall: destructor declared, no copy → compiler copies pointer shallowly
// ══════════════════════════════════════════════════════════════════════════════
namespace pitfall_A {

struct Shallow {
    std::size_t size;
    char* data;

    explicit Shallow(const char* s)
        : size(std::strlen(s) + 1), data(new char[size]) {
        std::memcpy(data, s, size);
    }
    ~Shallow() { delete[] data; }
    // No copy ctor / copy assign → compiler generates shallow copy → DOUBLE FREE!

    const char* get() const { return data; }
};

void demo() {
    std::cout << "--- A. Missing copy ctor ---\n";
    std::cout << "  Shallow a(\"hello\");\n";
    std::cout << "  Shallow b = a;  // b.data == a.data (same pointer!)\n";
    std::cout << "  // ~b() → frees data; ~a() → frees same pointer again → CRASH\n";
    std::cout << "  Fix: implement copy ctor (Rule of Three) or use std::string.\n\n";
}
} // namespace pitfall_A

// ══════════════════════════════════════════════════════════════════════════════
// B. Pitfall: no noexcept on move → std::vector falls back to copy on resize
// ══════════════════════════════════════════════════════════════════════════════
namespace pitfall_B {

struct WithoutNoexcept {
    std::size_t size;
    char*       data;

    explicit WithoutNoexcept(const char* s)
        : size(std::strlen(s)+1), data(new char[size]) { std::memcpy(data,s,size); }
    ~WithoutNoexcept()                                  { delete[] data; }
    WithoutNoexcept(const WithoutNoexcept& o)           : size(o.size), data(new char[o.size]) { std::memcpy(data,o.data,size); }
    WithoutNoexcept& operator=(const WithoutNoexcept&) = delete;

    // Move ctor WITHOUT noexcept
    WithoutNoexcept(WithoutNoexcept&& o) /* no noexcept! */
        : size(o.size), data(o.data) { o.data=nullptr; o.size=0; }
    WithoutNoexcept& operator=(WithoutNoexcept&&) = delete;
};

struct WithNoexcept {
    std::size_t size;
    char*       data;

    explicit WithNoexcept(const char* s)
        : size(std::strlen(s)+1), data(new char[size]) { std::memcpy(data,s,size); }
    ~WithNoexcept()                                   { delete[] data; }
    WithNoexcept(const WithNoexcept& o)               : size(o.size), data(new char[o.size]) { std::memcpy(data,o.data,size); }
    WithNoexcept& operator=(const WithNoexcept&)    = delete;

    // Move ctor WITH noexcept → vector uses move, not copy, on realloc
    WithNoexcept(WithNoexcept&& o) noexcept
        : size(o.size), data(o.data) { o.data=nullptr; o.size=0; }
    WithNoexcept& operator=(WithNoexcept&&)         = delete;
};

void demo() {
    std::cout << "--- B. Missing noexcept on move ctor ---\n";

    // is_nothrow_move_constructible tells us what vector will choose
    bool wn = std::is_nothrow_move_constructible_v<WithoutNoexcept>;
    bool wt = std::is_nothrow_move_constructible_v<WithNoexcept>;

    std::cout << "  WithoutNoexcept: is_nothrow_move_constructible = " << wn
              << " → vector will COPY on realloc (slow!)\n";
    std::cout << "  WithNoexcept:    is_nothrow_move_constructible = " << wt
              << " → vector will MOVE on realloc (fast)\n";
    std::cout << "  Rule: always mark move ctor/assign as noexcept.\n\n";
}
} // namespace pitfall_B

// ══════════════════════════════════════════════════════════════════════════════
// C. Pitfall: exception-unsafe copy assignment
// ══════════════════════════════════════════════════════════════════════════════
namespace pitfall_C {

void demo() {
    std::cout << "--- C. Exception-unsafe assignment (no copy-and-swap) ---\n";
    std::cout << "  Bad pattern:\n";
    std::cout << "    delete[] data;          // resource released\n";
    std::cout << "    data = new char[other.size]; // throws! → data is dangling!\n";
    std::cout << "  Fix (copy-and-swap): allocate FIRST, then swap, then release.\n";
    std::cout << "    Buffer& operator=(Buffer other) {\n";
    std::cout << "        std::swap(data, other.data);\n";
    std::cout << "        std::swap(size, other.size);\n";
    std::cout << "        return *this; // old data freed by other's dtor\n";
    std::cout << "    }\n\n";
}
} // namespace pitfall_C

// ══════════════════════════════════════════════════════════════════════════════
// D. Pitfall: declaring destructor suppresses implicit move generation
// ══════════════════════════════════════════════════════════════════════════════
namespace pitfall_D {

struct NoMoveGenerated {
    std::size_t size;
    char*       data;

    explicit NoMoveGenerated(const char* s)
        : size(std::strlen(s)+1), data(new char[size]) { std::memcpy(data,s,size); }

    // Declaring ONLY a destructor suppresses compiler-generated move operations.
    // std::vector<NoMoveGenerated> will copy (not move) elements on realloc.
    ~NoMoveGenerated() { delete[] data; }
};

void demo() {
    std::cout << "--- D. Declaring destructor suppresses move generation ---\n";
    bool can_move = std::is_move_constructible_v<NoMoveGenerated>;
    bool nothrow  = std::is_nothrow_move_constructible_v<NoMoveGenerated>;
    std::cout << "  is_move_constructible       = " << can_move
              << "  (falls back to copy)\n";
    std::cout << "  is_nothrow_move_constructible = " << nothrow << "\n";
    std::cout << "  Fix: implement all five, or use = default / smart pointers.\n\n";
}
} // namespace pitfall_D

int main() {
    std::cout << "=== Common Pitfalls ===\n\n";
    pitfall_A::demo();
    pitfall_B::demo();
    pitfall_C::demo();
    pitfall_D::demo();
    std::cout << ">>> Summary:\n";
    std::cout << "  1. Define all three (Rule of Three) or all five (Rule of Five).\n";
    std::cout << "  2. Mark move ctor/assign noexcept.\n";
    std::cout << "  3. Use copy-and-swap for exception-safe assignment.\n";
    std::cout << "  4. Prefer Rule of Zero (smart pointers) to avoid writing any.\n";
    return 0;
}
