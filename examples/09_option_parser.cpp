// P2996 Example 9 — Command-line option parser using member reflection
// godbolt.org/z/KzfvPsenG
// Flags: -std=c++26 -freflection

#include <string>
#include <meta>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <span>

template<typename Opts>
auto parse_options(std::span<std::string_view const> args) -> Opts {
  Opts opts;

  constexpr auto ctx = std::meta::access_context::current();
  template for (constexpr auto dm : define_static_array(nonstatic_data_members_of(^^Opts, ctx))) {
    constexpr auto dmcopy = dm;  // hack around a known bug
    auto it = std::find_if(args.begin(), args.end(),
      [](std::string_view arg){
        return arg.starts_with("--") && arg.substr(2) == identifier_of(dmcopy);
      });

    if (it == args.end()) {
      // no option provided, use default
      continue;
    } else if (it + 1 == args.end()) {
      std::cerr << "Option " << *it << " is missing a value\n";
      std::exit(EXIT_FAILURE);
    }

    using T = typename[:type_of(dm):];
    auto iss = std::stringstream(it[1]);
    if (iss >> opts.[:dm:]; !iss) {
      std::cerr << "Failed to parse option " << *it << " into a "
                << display_string_of(^^T) << '\n';
      std::exit(EXIT_FAILURE);
    }
  }
  return opts;
}

struct MyOpts {
  std::string file_name = "input.txt";  // Option "--file_name <string>"
  int    count = 1;                     // Option "--count <int>"
};

int main(int argc, char *argv[]) {
  MyOpts opts = parse_options<MyOpts>(std::vector<std::string_view>(argv+1, argv+argc));

  std::cout << "opts.file=" << opts.file_name << '\n';
  std::cout << "opts.count=" << opts.count << '\n';
}
