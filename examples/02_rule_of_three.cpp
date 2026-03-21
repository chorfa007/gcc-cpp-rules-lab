/**
 * Example 2 – Rule of Three
 *
 * "If a class needs a custom destructor, copy constructor, or copy assignment
 *  operator, it almost certainly needs all three."
 *
 * This is the pre-C++11 pattern for classes that manage raw resources.
 * We show a BAD (single-special-member) version first, then the correct fix.
 */

#include <cstring>
#include <iostream>
#include <stdexcept>

// ══════════════════════════════════════════════════════════════════════════════
// BAD: only destructor is user-defined → compiler generates SHALLOW copy.
// Running this triggers a double-free (undefined behaviour).
// ══════════════════════════════════════════════════════════════════════════════
class BadBuffer {
    std::size_t size;
    char*       data;
public:
    explicit BadBuffer(const char* s)
        : size(std::strlen(s) + 1), data(new char[size]) {
        std::memcpy(data, s, size);
    }
    ~BadBuffer() { delete[] data; }

    // Compiler-generated copy: copies the POINTER, not the data.
    // Two BadBuffers end up owning the same memory → double-free on destruction!

    const char* get() const { return data; }
};

// ══════════════════════════════════════════════════════════════════════════════
// GOOD: Rule of Three – all three special members implemented.
// ══════════════════════════════════════════════════════════════════════════════
class Buffer {
    std::size_t size;
    char*       data;

    // Private helper – allocate and copy
    static char* dup(const char* src, std::size_t n) {
        char* p = new char[n];
        std::memcpy(p, src, n);
        return p;
    }

public:
    // ── constructor ───────────────────────────────────────────────────────────
    explicit Buffer(const char* s = "")
        : size(std::strlen(s) + 1), data(dup(s, size)) {
        std::cout << "  [ctor]  \"" << data << "\"\n";
    }

    // ── 1/3 destructor ────────────────────────────────────────────────────────
    ~Buffer() {
        std::cout << "  [dtor]  \"" << data << "\"\n";
        delete[] data;
    }

    // ── 2/3 copy constructor ─────────────────────────────────────────────────
    Buffer(const Buffer& other)
        : size(other.size), data(dup(other.data, other.size)) {
        std::cout << "  [copy-ctor] \"" << data << "\"\n";
    }

    // ── 3/3 copy assignment operator ─────────────────────────────────────────
    // Copy-and-swap idiom: exception-safe, handles self-assignment.
    Buffer& operator=(Buffer other) {   // pass by value → calls copy ctor
        std::swap(data, other.data);
        std::swap(size, other.size);
        std::cout << "  [copy-assign] \"" << data << "\"\n";
        return *this;                   // other (old data) destroyed on exit
    }

    const char* get()    const { return data; }
    std::size_t length() const { return size - 1; }
};

// ══════════════════════════════════════════════════════════════════════════════
// Demonstrate a subtle bug: forgetting the copy-ctor in a container resize.
// ══════════════════════════════════════════════════════════════════════════════

void showBug() {
    std::cout << "\n--- BAD (do NOT run for real – UB / double-free) ---\n";
    std::cout << "  BadBuffer b1(\"hello\");\n";
    std::cout << "  BadBuffer b2 = b1;  ← b1 and b2 share the same pointer!\n";
    std::cout << "  // ~b2() frees the data; ~b1() tries to free it again → crash\n";
}

int main() {
    std::cout << "=== Rule of Three ===\n\n";

    {
        Buffer b1("hello");
        Buffer b2(b1);          // copy constructor
        Buffer b3("world");
        b3 = b1;                // copy assignment (copy-and-swap)

        std::cout << "\nb1=" << b1.get()
                  << "  b2=" << b2.get()
                  << "  b3=" << b3.get() << "\n\n";

        // Self-assignment must be safe
        b1 = b1;
        std::cout << "After self-assign: b1=" << b1.get() << "\n\n";
    } // all three destructors called here

    showBug();
    std::cout << "\n>>> Rule of Three: destructor + copy-ctor + copy-assign.\n";
    return 0;
}
