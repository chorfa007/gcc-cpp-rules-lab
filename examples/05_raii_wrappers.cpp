/**
 * Example 5 – RAII Wrappers (applied Rule of Five)
 *
 * Practical RAII wrappers around C-style resources that follow the Rule of Five.
 * These are the patterns you actually write in real code.
 *
 *   - FileHandle  : RAII around FILE*
 *   - MutexLock   : RAII around pthread_mutex_t
 *   - UniqueArray : lightweight unique_ptr<T[]>-like wrapper
 */

#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

// ── 1. FileHandle – RAII wrapper around FILE* ─────────────────────────────────
class FileHandle {
    FILE* fp = nullptr;

public:
    FileHandle() = default;

    explicit FileHandle(const char* path, const char* mode) {
        fp = std::fopen(path, mode);
        if (!fp) throw std::runtime_error(std::string("Cannot open: ") + path);
        std::cout << "  [FileHandle] opened " << path << "\n";
    }

    // 1/5
    ~FileHandle() { close(); }

    // 2/5 – copy semantics make no sense for an exclusive file handle → delete
    FileHandle(const FileHandle&)            = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    // 4/5 move constructor
    FileHandle(FileHandle&& other) noexcept : fp(other.fp) {
        other.fp = nullptr;
        std::cout << "  [FileHandle] moved\n";
    }

    // 5/5 move assignment
    FileHandle& operator=(FileHandle&& other) noexcept {
        if (this != &other) {
            close();
            fp       = other.fp;
            other.fp = nullptr;
        }
        return *this;
    }

    void close() {
        if (fp) {
            std::fclose(fp);
            std::cout << "  [FileHandle] closed\n";
            fp = nullptr;
        }
    }

    bool   is_open() const { return fp != nullptr; }
    FILE*  get()     const { return fp; }

    void write(const char* s) { if (fp) std::fputs(s, fp); }
};

// ── 2. UniqueArray – minimal unique_ptr<T[]> to illustrate Rule of Five ────────
template<typename T>
class UniqueArray {
    T*          ptr  = nullptr;
    std::size_t len  = 0;

public:
    UniqueArray() = default;

    explicit UniqueArray(std::size_t n)
        : ptr(new T[n]()), len(n) {
        std::cout << "  [UniqueArray] alloc " << n << " elements\n";
    }

    // 1/5
    ~UniqueArray() {
        if (ptr) std::cout << "  [UniqueArray] free " << len << " elements\n";
        delete[] ptr;
    }

    // 2/5 + 3/5 – exclusive ownership: no copy
    UniqueArray(const UniqueArray&)            = delete;
    UniqueArray& operator=(const UniqueArray&) = delete;

    // 4/5
    UniqueArray(UniqueArray&& o) noexcept : ptr(o.ptr), len(o.len) {
        o.ptr = nullptr; o.len = 0;
        std::cout << "  [UniqueArray] move-ctor\n";
    }

    // 5/5
    UniqueArray& operator=(UniqueArray&& o) noexcept {
        if (this != &o) {
            delete[] ptr;
            ptr = o.ptr; len = o.len;
            o.ptr = nullptr; o.len = 0;
        }
        return *this;
    }

    T&          operator[](std::size_t i)       { return ptr[i]; }
    const T&    operator[](std::size_t i) const { return ptr[i]; }
    std::size_t size()    const { return len; }
    bool        empty()   const { return ptr == nullptr; }
    T*          data()          { return ptr; }
};

// ── main ──────────────────────────────────────────────────────────────────────
int main() {
    std::cout << "=== RAII Wrappers (Rule of Five in practice) ===\n\n";

    // --- FileHandle ---
    {
        std::cout << "-- FileHandle --\n";
        FileHandle f("/tmp/raii_test.txt", "w");
        f.write("hello from RAII\n");

        FileHandle g = std::move(f);   // ownership transferred
        std::cout << "  f.is_open()=" << f.is_open()
                  << "  g.is_open()=" << g.is_open() << "\n";
        // g destroyed here → file closed
    }

    // --- UniqueArray ---
    std::cout << "\n-- UniqueArray<double> --\n";
    {
        UniqueArray<double> arr(8);
        for (std::size_t i = 0; i < arr.size(); ++i) arr[i] = static_cast<double>(i) * 1.5;

        UniqueArray<double> arr2 = std::move(arr);
        std::cout << "  arr.empty()=" << arr.empty()
                  << "  arr2[3]="     << arr2[3] << "\n";
        // arr2 freed here
    }

    std::cout << "\n>>> RAII + Rule of Five = safe, efficient resource management.\n";
    return 0;
}
