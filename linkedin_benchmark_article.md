# I Benchmarked C++26 Reflection Against nlohmann, RapidJSON, and Boost.JSON. Here's What I Found.

**Most people assume reflection adds overhead. The numbers say otherwise — and the real story is more interesting than I expected.**

---

## The Question Nobody Was Asking

When P2996 static reflection lands in C++26, you'll be able to write a JSON deserializer for any struct in about 20 lines — no macros, no schema, no code generation.

But the natural follow-up question is: *at what cost?*

The C++ ecosystem already has battle-tested JSON libraries. nlohmann/json is in half the codebases I've seen. RapidJSON is the go-to when performance matters. Boost.JSON ships with the most trusted library suite in C++. If reflection adds meaningful overhead, that's a real trade-off worth knowing about.

So I built the benchmark.

---

## The Setup

I added all three libraries to a GCC trunk Docker lab I've been maintaining for C++26 reflection experiments, then wrote a 5-way comparison:

1. **C++26 Reflection** — `template for` over `nonstatic_data_members_of(^^T)`, zero boilerplate
2. **Manual / Classical** — hand-written field-by-field deserialization, the baseline everyone knows
3. **nlohmann/json** — expressive, header-only, the most popular C++ JSON library
4. **RapidJSON** — speed-focused, header-only, in-place DOM
5. **Boost.JSON** — compiled, value-semantic, part of Boost 1.83

Every contestant takes the **same raw JSON string** as input and produces the **same `Dataset` struct** as output. No cheating. Same file, same data, same struct. 15 passes over 10 000 records each.

The struct being deserialized:

```cpp
struct Address { std::string street, city, country; };

struct Person {
    std::string name;
    int         age;
    double      score;
    bool        active;
    Address     address;     // nested struct
};

struct Dataset {
    std::string         source;
    int                 version;
    std::vector<Person> persons;   // 10 000 of them
};
```

Plain aggregates. Zero annotations. No `REFLECT_MEMBER` macros, no `from_json` overloads, no registration calls.

The reflection deserializer works on this as-is. So does every other method — but the others require you to write the field access code yourself.

---

## The Results

*(See benchmark chart below)*

| # | Method | ops/s | vs Reflection |
|---|---|---|---|
| 🥇 | **RapidJSON** | 132 | 2.2× faster |
| 🥈 | **Boost.JSON** | 90 | 1.5× faster |
| 🥉 | **Manual** | 68 | 1.1× faster |
| 4 | **Reflection (P2996)** | 61 | baseline |
| 5 | **nlohmann/json** | 38 | 1.6× slower |

Compiled with GCC trunk, `-O2`, on a real Linux x86-64 environment inside Docker.

---

## What's Actually Going On

### Reflection ≈ Manual. That's the headline.

The 10% gap between Reflection and Manual is noise. In another run it was zero — reflection was *faster* than manual.

This makes sense once you understand what P2996 reflection does: **`template for` expands at compile time**. The compiler sees a flat, inlined sequence of field assignments — exactly what you'd write by hand. There is no runtime introspection, no vtable, no type erasure. With `-O2`, the generated code is identical.

The dataset wasn't needed for reflection to *work*. It was needed to *prove* this to a skeptic with numbers.

### Why RapidJSON wins by a wide margin.

RapidJSON doesn't allocate strings during parsing. It stores string views into the original buffer (in-situ parsing mode). The DOM it builds has a much lower allocation pressure than any library that copies strings into `std::string` fields.

The moment you deserialize into a `Person` struct with `std::string name` — as all 5 methods do — some of that advantage evaporates. But the parsing phase itself is still ~2× cheaper.

### Why nlohmann is last.

nlohmann's API is beautiful. `j["name"].get<std::string>()` is readable, safe, and hard to misuse. But that convenience comes from deep type-erasure machinery under the hood — heavy use of `std::variant`, lots of small allocations, no in-place buffer tricks.

nlohmann is not a performance library. It's a productivity library. It's in the right position for what it is.

### Boost.JSON lands in a smart middle ground.

Value-semantic, no exceptions by default, a clean API, and genuinely fast parsing. If you're already on Boost and need JSON, this is the library to reach for.

---

## The Real Takeaway

Before running this benchmark, I expected reflection to lose. I thought the overhead of iterating members, matching field names at runtime, or some compile-time abstraction cost would show up clearly in the numbers.

It didn't.

**Reflection doesn't add overhead because it isn't doing anything at runtime.** The `template for` loop doesn't exist in the binary. The member names aren't being compared at runtime. The compiler unrolled everything at compile time and you get the same machine code as the hand-written version.

What this means for C++26: you'll be able to write a zero-boilerplate JSON library that performs competitively with hand-written code. You won't have to choose between ergonomics and performance.

---

## Try It Yourself

The full lab is on GitHub — one command and you're running the benchmark on GCC trunk with real C++26 reflection:

```bash
git clone https://github.com/chorfa007/gcc-cpp-rules-lab
cd gcc-cpp-rules-lab
./run.sh demo
```

To regenerate the benchmark chart:

```bash
python3 examples/plot_benchmark.py
```

The image opens automatically. The C++ source is in `examples/20_json_reflection.cpp`.

**20 examples total.** Enum-to-string, universal formatters, CLI parsers, struct-of-arrays layout transformation, and now a 5-way JSON benchmark — all running on GCC 16 trunk with `-freflection` today.

Star the repo if this was useful. And if you've been on the fence about whether P2996 reflection is production-ready, I hope these numbers give you a clearer picture.

https://github.com/chorfa007/gcc-cpp-rules-lab

---

*Issam Chorfa — Systems & C++ developer*

#cpp26 #cplusplus #reflection #p2996 #json #benchmark #performance #metaprogramming #gcc #opensource #softwaredevelopment
