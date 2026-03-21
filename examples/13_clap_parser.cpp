// P2996 Example 13 — clap-style CLI argument parser via reflection
// godbolt.org/z/97PEaG3r3
// Flags: -std=c++26 -freflection
// Usage: ./13_clap_parser --name World --count 2

#include <algorithm>
#include <iostream>
#include <meta>
#include <ranges>
#include <sstream>
#include <string>

namespace clap {
  struct Flags {
      bool use_short;
      bool use_long;
  };

  template <typename T, Flags flags>
  struct Option {
      std::optional<T> initializer;

      Option() = default;
      Option(T t) : initializer(t) { }

      static constexpr bool use_short = flags.use_short;
      static constexpr bool use_long = flags.use_long;
  };

  consteval auto spec_to_opts(std::meta::info opts, std::meta::info spec) -> std::meta::info {
    std::vector<std::meta::info> new_members;
    for (auto member :
          nonstatic_data_members_of(spec, std::meta::access_context::current())) {
      auto new_type = template_arguments_of(type_of(member))[0];
      new_members.push_back(data_member_spec(new_type, {.name=identifier_of(member)}));
    }
    return define_aggregate(opts, new_members);
  }

  struct Clap {
    template <typename Spec>
    auto parse(this Spec const& spec, int argc, const char** argv) {
      std::vector<std::string_view> cmdline(argv + 1, argv + argc);

      struct Opts;
      consteval {
        spec_to_opts(^^Opts, ^^Spec);
      }
      Opts opts;

      constexpr auto ctx = std::meta::access_context::current();
      template for (constexpr auto Pair :
                    std::define_static_array(
                      std::views::zip(nonstatic_data_members_of(^^Spec, ctx),
                                      nonstatic_data_members_of(^^Opts, ctx)) |
                      std::views::transform([](auto z) { return std::pair(get<0>(z), get<1>(z)); }))) {
        constexpr auto sm = Pair.first;
        constexpr auto om = Pair.second;

        auto& cur = spec.[:sm:];
        constexpr auto type = type_of(om);

        auto it = std::find_if(cmdline.begin(), cmdline.end(),
            [&](std::string_view arg) {
              return (cur.use_short && arg.size() == 2 && arg[0] == '-' &&
                      arg[1] == identifier_of(sm)[0])
                  || (cur.use_long && arg.starts_with("--") &&
                      arg.substr(2) == identifier_of(sm));
            });

        if (it == cmdline.end()) {
          if constexpr (has_template_arguments(type) &&
                        template_of(type) == ^^std::optional) {
            continue;
          } else if (cur.initializer) {
            opts.[:om:] = *cur.initializer;
            continue;
          } else {
            std::cerr << "Missing required option "
                      << display_string_of(sm) << '\n';
            std::exit(EXIT_FAILURE);
          }
        } else if (it + 1 == cmdline.end()) {
          std::cout << "Option " << *it << " for " << display_string_of(sm)
                    << " is missing a value\n";
          std::exit(EXIT_FAILURE);
        }

        std::stringstream iss;
        iss << it[1];
        if (iss >> opts.[:om:]; !iss) {
          std::cerr << "Failed to parse " << it[1] << " into option "
                    << display_string_of(sm) << " of type "
                    << display_string_of(type_of(om)) << '\n';
          std::exit(EXIT_FAILURE);
        }
      }

      return opts;
    }
  };
}

using namespace clap;
struct Args : Clap {
  Option<std::string, Flags{.use_short=true, .use_long=true}> name;
  Option<int, Flags{.use_short=true, .use_long=true}> count = 1;
};

int main(int argc, const char** argv) {
  auto opts = Args{}.parse(argc, argv);

  for (int i = 0; i < opts.count; ++i) {
    std::cout << "Hello " << opts.name << "!\n";
  }
}
