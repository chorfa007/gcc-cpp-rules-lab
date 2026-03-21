// P2996 Example 18 — Named tuple construction via initializer_list
// godbolt.org/z/6j8qzejsc
// Flags: -std=c++26 -freflection

#include <meta>
#include <algorithm>

consteval auto make_named_tuple(std::meta::info type,
                                std::initializer_list<std::pair<std::meta::info, std::string_view>> members) {
    std::vector<std::meta::info> nsdms;
    for (auto [type, name] : members) {
        nsdms.push_back(data_member_spec(type, {.name=name}));
    }
    return define_aggregate(type, nsdms);
}

struct R;
consteval {
  make_named_tuple(^^R, {{^^int, "x"}, {^^double, "y"}});
}

constexpr auto ctx = std::meta::access_context::current();
static_assert(type_of(nonstatic_data_members_of(^^R, ctx)[0]) == ^^int);
static_assert(type_of(nonstatic_data_members_of(^^R, ctx)[1]) == ^^double);

int main() {
    [[maybe_unused]] auto r = R{.x=1, .y=2.0};
}
