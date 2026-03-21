/**
 * Example 3 – Rule of Five
 *
 * C++11 introduced move semantics.  The Rule of Three expands to the Rule of
 * Five: if you write any of the five special members, you should write all five.
 *
 *   1. Destructor
 *   2. Copy constructor
 *   3. Copy assignment operator
 *   4. Move constructor         ← new in C++11
 *   5. Move assignment operator ← new in C++11
 *
 * Move operations allow cheap "steal the guts" transfers.
 * Without them the compiler will fall back to copies (or delete them entirely
 * if a destructor is declared), costing performance and correctness.
 */

#include <cstring>
#include <iostream>
#include <utility>

class Buffer {
    std::size_t size;
    char*       data;

    static char* dup(const char* src, std::size_t n) {
        char* p = new char[n];
        std::memcpy(p, src, n);
        return p;
    }

public:
    // ── constructor ───────────────────────────────────────────────────────────
    explicit Buffer(const char* s = "")
        : size(std::strlen(s) + 1), data(dup(s, size)) {
        std::cout << "  [ctor]       \"" << data << "\"  @" << static_cast<void*>(data) << "\n";
    }

    // ── 1/5 destructor ────────────────────────────────────────────────────────
    ~Buffer() {
        if (data)
            std::cout << "  [dtor]       \"" << data << "\"  @" << static_cast<void*>(data) << "\n";
        else
            std::cout << "  [dtor]       (moved-from / null)\n";
        delete[] data;
    }

    // ── 2/5 copy constructor ─────────────────────────────────────────────────
    Buffer(const Buffer& other)
        : size(other.size), data(dup(other.data, other.size)) {
        std::cout << "  [copy-ctor]  \"" << data << "\"  (new alloc)\n";
    }

    // ── 3/5 copy assignment ───────────────────────────────────────────────────
    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;
        char* newData = dup(other.data, other.size);
        delete[] data;
        data = newData;
        size = other.size;
        std::cout << "  [copy-assign] \"" << data << "\"  (new alloc)\n";
        return *this;
    }

    // ── 4/5 move constructor ─────────────────────────────────────────────────
    // "Steal" the other object's resources; leave it in a valid, empty state.
    Buffer(Buffer&& other) noexcept
        : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
        std::cout << "  [move-ctor]  \"" << data << "\"  (no alloc!)\n";
    }

    // ── 5/5 move assignment ───────────────────────────────────────────────────
    Buffer& operator=(Buffer&& other) noexcept {
        if (this == &other) return *this;
        delete[] data;          // release our current resource
        data       = other.data;
        size       = other.size;
        other.data = nullptr;
        other.size = 0;
        std::cout << "  [move-assign] \"" << data << "\"  (no alloc!)\n";
        return *this;
    }

    const char* get()    const { return data ? data : ""; }
    std::size_t length() const { return size > 0 ? size - 1 : 0; }
    bool        empty()  const { return data == nullptr; }
};

// ── helper: creates a temporary Buffer (triggers move, not copy) ──────────────
Buffer makeBuffer(const char* s) {
    return Buffer(s);           // NRVO / move on return
}

int main() {
    std::cout << "=== Rule of Five ===\n\n";

    // --- copy operations ---
    std::cout << "-- Copy operations --\n";
    Buffer b1("hello");
    Buffer b2(b1);              // copy constructor
    Buffer b3("world");
    b3 = b1;                    // copy assignment
    std::cout << "b1=" << b1.get() << "  b2=" << b2.get() << "  b3=" << b3.get() << "\n\n";

    // --- move operations ---
    std::cout << "-- Move operations --\n";
    Buffer b4(std::move(b1));   // move constructor: b1 becomes empty
    std::cout << "b4=" << b4.get() << "  b1.empty()=" << b1.empty() << "\n";

    Buffer b5("temp");
    b5 = std::move(b4);         // move assignment: b4 becomes empty
    std::cout << "b5=" << b5.get() << "  b4.empty()=" << b4.empty() << "\n\n";

    // --- factory returning temporary ---
    std::cout << "-- Factory (return value, move from temporary) --\n";
    Buffer b6 = makeBuffer("factory");
    std::cout << "b6=" << b6.get() << "\n\n";

    std::cout << "-- Destructor calls follow --\n";
    return 0;
}
