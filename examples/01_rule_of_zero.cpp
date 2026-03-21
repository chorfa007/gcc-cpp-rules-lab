/**
 * Example 1 – Rule of Zero
 *
 * "Classes that don't directly manage resources should declare no special
 *  member functions.  Let compiler-generated defaults do the right thing."
 *
 * Key idea: compose with RAII types (std::string, std::vector,
 * std::unique_ptr, std::shared_ptr) so the compiler can generate
 * correct copy/move/destroy for free.
 */

#include <string>
#include <vector>
#include <memory>
#include <iostream>

// ── 1. Plain value-type class – zero special members needed ───────────────────
class User {
public:
    std::string name;
    std::vector<int> scores;

    User(std::string n, std::vector<int> s)
        : name(std::move(n)), scores(std::move(s)) {}

    void print() const {
        std::cout << "User{" << name << ", scores=" << scores.size() << "}\n";
    }

    // No destructor, no copy, no move — compiler handles all correctly.
};

// ── 2. Exclusive ownership via unique_ptr – move-only, no copy ─────────────────
class ExclusiveBuffer {
    std::unique_ptr<int[]> data;
    std::size_t            len;

public:
    explicit ExclusiveBuffer(std::size_t n)
        : data(std::make_unique<int[]>(n)), len(n) {
        for (std::size_t i = 0; i < len; ++i) data[i] = static_cast<int>(i);
    }

    std::size_t size() const { return len; }
    int         operator[](std::size_t i) const { return data[i]; }

    // unique_ptr's move constructor propagates automatically –
    // ExclusiveBuffer is move-only without writing a single special member.
};

// ── 3. Shared ownership via shared_ptr – copyable ─────────────────────────────
class SharedConfig {
    std::shared_ptr<std::vector<std::string>> entries;

public:
    SharedConfig() : entries(std::make_shared<std::vector<std::string>>()) {}

    void add(std::string s) { entries->push_back(std::move(s)); }
    std::size_t size() const { return entries->size(); }

    // Copies share the same underlying vector (shallow / reference-counted).
    // Still zero declared special members.
};

// ── main ──────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== Rule of Zero ===\n\n";

    // 1 – User: copy and move both work
    User alice{"Alice", {10, 20, 30}};
    User bob = alice;                   // deep copy via std::string / std::vector
    User carol = std::move(alice);      // move; alice is now valid-but-unspecified

    bob.print();
    carol.print();

    // 2 – ExclusiveBuffer: move-only
    ExclusiveBuffer buf1(5);
    ExclusiveBuffer buf2 = std::move(buf1);  // ownership transferred
    std::cout << "buf2[3] = " << buf2[3] << "\n";
    // ExclusiveBuffer buf3 = buf2;  // ← would not compile (good!)

    // 3 – SharedConfig: copies share data
    SharedConfig cfg1;
    cfg1.add("host=localhost");
    SharedConfig cfg2 = cfg1;           // both point to same vector
    cfg2.add("port=8080");
    std::cout << "cfg1.size() = " << cfg1.size()   // 2 — shared!
              << "  cfg2.size() = " << cfg2.size() << "\n";

    std::cout << "\n>>> Rule of Zero: zero written, everything correct.\n";
    return 0;
}
