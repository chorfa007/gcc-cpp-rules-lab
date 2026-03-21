/**
 * Example 4 – Move vs Copy performance
 *
 * Demonstrates that the Rule of Five's move operations are not just for
 * correctness — they deliver measurable performance gains for large resources.
 *
 * We time:
 *   a) growing a std::vector<Buffer> with a copyable-only Buffer (Rule of Three)
 *   b) growing it with a fully movable Buffer (Rule of Five)
 *
 * Compile with: g++ -std=c++23 -O2 04_move_performance.cpp -o bin/04_move_performance
 */

#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

static constexpr std::size_t BUF_SIZE  = 1024 * 64;   // 64 KB per object
static constexpr int         N_OBJECTS = 2000;

// ── Rule of Three only: no move operations ────────────────────────────────────
class CopyOnlyBuffer {
    std::size_t size;
    char*       data;
public:
    explicit CopyOnlyBuffer(const char* tag)
        : size(BUF_SIZE), data(new char[BUF_SIZE]) {
        std::memset(data, static_cast<int>(tag[0]), BUF_SIZE);
    }
    ~CopyOnlyBuffer()                         { delete[] data; }
    CopyOnlyBuffer(const CopyOnlyBuffer& o)   : size(o.size), data(new char[o.size]) { std::memcpy(data, o.data, size); }
    CopyOnlyBuffer& operator=(const CopyOnlyBuffer& o) {
        if (this == &o) return *this;
        char* p = new char[o.size];
        std::memcpy(p, o.data, o.size);
        delete[] data;
        data = p; size = o.size;
        return *this;
    }
    // move operations are DELETED (no Rule of Five) → vector realloc copies
    CopyOnlyBuffer(CopyOnlyBuffer&&)                 = delete;
    CopyOnlyBuffer& operator=(CopyOnlyBuffer&&)      = delete;
};

// ── Rule of Five: copy + move ─────────────────────────────────────────────────
class MovableBuffer {
    std::size_t size;
    char*       data;
public:
    explicit MovableBuffer(const char* tag)
        : size(BUF_SIZE), data(new char[BUF_SIZE]) {
        std::memset(data, static_cast<int>(tag[0]), BUF_SIZE);
    }
    ~MovableBuffer()                          { delete[] data; }
    MovableBuffer(const MovableBuffer& o)     : size(o.size), data(new char[o.size]) { std::memcpy(data, o.data, size); }
    MovableBuffer& operator=(const MovableBuffer& o) {
        if (this == &o) return *this;
        char* p = new char[o.size];
        std::memcpy(p, o.data, o.size);
        delete[] data;
        data = p; size = o.size;
        return *this;
    }
    // ← the difference: cheap move that just swaps pointers
    MovableBuffer(MovableBuffer&& o) noexcept
        : size(o.size), data(o.data) { o.data = nullptr; o.size = 0; }
    MovableBuffer& operator=(MovableBuffer&& o) noexcept {
        if (this == &o) return *this;
        delete[] data;
        data = o.data; size = o.size;
        o.data = nullptr; o.size = 0;
        return *this;
    }
};

template<typename T, typename Factory>
long long benchmark(const char* label, Factory factory) {
    using clock = std::chrono::high_resolution_clock;
    std::vector<T> v;
    v.reserve(1);                    // force reallocations

    auto t0 = clock::now();
    for (int i = 0; i < N_OBJECTS; ++i)
        v.push_back(factory());
    auto t1 = clock::now();

    long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
    std::cout << "  " << label << ": " << ms << " ms\n";
    return ms;
}

int main() {
    std::cout << "=== Move vs Copy Performance ===\n";
    std::cout << "  " << N_OBJECTS << " objects × " << BUF_SIZE / 1024 << " KB each\n\n";

    long long copy_ms = benchmark<CopyOnlyBuffer>("CopyOnly (Rule of Three)",
        []{ return CopyOnlyBuffer("A"); });

    long long move_ms = benchmark<MovableBuffer> ("Movable  (Rule of Five) ",
        []{ return MovableBuffer("B"); });

    std::cout << "\n  Speedup: " << (copy_ms > 0 ? copy_ms / std::max(1LL, move_ms) : 0)
              << "x  (move avoids reallocating " << BUF_SIZE / 1024 << " KB on each vector growth)\n";
    return 0;
}
