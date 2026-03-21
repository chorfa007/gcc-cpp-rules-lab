// P2996 Example 16 — Reflective tuple_cat implementation
// godbolt.org/z/v8cfGeo3q
// Flags: -std=c++26 -freflection

#include <meta>
#include <functional>
#include <utility>
#include <vector>
#include <initializer_list>
#include <tuple>

template<std::pair<std::size_t, std::size_t>... indices>
struct Indexer {
   template<typename Tuples>
   auto operator()(Tuples&& tuples) const {
     using ResultType = std::tuple<
       std::tuple_element_t<
         indices.second,
         std::remove_cvref_t<std::tuple_element_t<indices.first, std::remove_cvref_t<Tuples>>>
       >...
     >;
     return ResultType(std::get<indices.second>(std::get<indices.first>(std::forward<Tuples>(tuples)))...);
   }
};

template <class T>
consteval auto subst_by_value(std::meta::info tmpl, std::vector<T> args)
    -> std::meta::info
{
    std::vector<std::meta::info> a2;
    for (T x : args) {
        a2.push_back(std::meta::reflect_constant(x));
    }

    return substitute(tmpl, a2);
}

consteval auto make_indexer(std::vector<std::size_t> sizes)
    -> std::meta::info
{
    std::vector<std::pair<int, int>> args;

    for (std::size_t tidx = 0; tidx < sizes.size(); ++tidx) {
        for (std::size_t eidx = 0; eidx < sizes[tidx]; ++eidx) {
            args.push_back({(int)tidx, (int)eidx});
        }
    }

    return subst_by_value(^^Indexer, args);
}

template<typename... Tuples>
auto my_tuple_cat(Tuples&&... tuples) {
    constexpr typename [: make_indexer({std::tuple_size_v<std::remove_cvref_t<Tuples>>...}) :] indexer;
    return indexer(std::forward_as_tuple(std::forward<Tuples>(tuples)...));
}

int r;
auto x = my_tuple_cat(std::make_tuple(10, std::ref(r)), std::make_tuple(21.0, 22, 23, 24));
static_assert(std::same_as<decltype(x), std::tuple<int, int&, double, int, int, int>>);

int main() {
  return std::get<5>(x);
}
