// P2996 Example 12 — Struct-of-Arrays transformation via reflection
// godbolt.org/z/dMdzrc64c
// Flags: -std=c++26 -freflection

#include <meta>
#include <array>
#include <iostream>

template <typename T, size_t N>
struct struct_of_arrays_impl {
  struct impl;

  consteval {
    auto ctx = std::meta::access_context::current();

    std::vector<std::meta::info> old_members = nonstatic_data_members_of(^^T, ctx);
    std::vector<std::meta::info> new_members = {};
    for (std::meta::info member : old_members) {
        auto array_type = substitute(^^std::array, {
            type_of(member),
            std::meta::reflect_constant(N),
        });
        auto mem_descr = data_member_spec(array_type, {.name = identifier_of(member)});
        new_members.push_back(mem_descr);
    }

    define_aggregate(^^impl, new_members);
  }
};

template <typename T, size_t N>
using struct_of_arrays = struct_of_arrays_impl<T, N>::impl;

struct point {
  float x;
  float y;
  float z;
};

int main() {
    using points = struct_of_arrays<point, 2>;

    points p = {
        .x={1.1f, 2.2f},
        .y={3.3f, 4.4f},
        .z={5.5f, 6.6f}
    };
    static_assert(p.x.size() == 2);
    static_assert(p.y.size() == 2);
    static_assert(p.z.size() == 2);

    for (size_t i = 0; i != 2; ++i) {
        std::cout << "p[" << i << "] = (" << p.x[i] << ", " << p.y[i] << ", " << p.z[i] << ")\n";
    }
}
