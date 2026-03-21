// P2996 Example 17 — Named tuple construction via pair<type, name>
// godbolt.org/z/8e6feWj6K
// Flags: -std=c++26 -freflection

#include <meta>
#include <algorithm>

template <size_t N>
struct fixed_string {
    char data[N];

    constexpr fixed_string(char const (&s)[N]) {
        std::copy(s, s+N, data);
    }

    constexpr auto view() const -> std::string_view { return data; }
};

template <class T, fixed_string Name>
struct pair {
    static constexpr auto name() -> std::string_view { return Name.view(); }
    using type = T;
};

template <class... Tags>
consteval auto make_named_tuple(std::meta::info type, Tags... tags) {
    std::vector<std::meta::info> nsdms;
    auto f = [&]<class Tag>(Tag tag){
        nsdms.push_back(std::meta::data_member_spec(
            dealias(^^typename Tag::type),
            {.name=Tag::name()}));
    };
    (f(tags), ...);
    return define_aggregate(type, nsdms);
}

struct R;
consteval {
  make_named_tuple(^^R, pair<int, "x">{}, pair<double, "y">{});
}

constexpr auto ctx = std::meta::access_context::current();
static_assert(type_of(nonstatic_data_members_of(^^R, ctx)[0]) == ^^int);
static_assert(type_of(nonstatic_data_members_of(^^R, ctx)[1]) == ^^double);

int main() {
    [[maybe_unused]] auto r = R{.x=1, .y=2.0};
}
