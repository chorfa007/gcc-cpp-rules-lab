// ============================================================
// 20_json_reflection.cpp — C++26 P2996 Reflection vs JSON Libraries
// Flags: -std=c++26 -freflection -O2 -lboost_json
//
// Benchmarks 5 approaches for JSON → Dataset deserialization:
//   1. C++26 P2996 Reflection  (custom parser + template for)
//   2. Manual / Classical      (custom parser + hand-written fields)
//   3. nlohmann/json           (header-only, expressive API)
//   4. RapidJSON               (header-only, SAX/DOM, speed-focused)
//   5. Boost.JSON              (compiled, value-semantic DOM)
//
// Build:
//   g++ -std=c++26 -freflection -O2 \
//       -o bin/20_json_reflection 20_json_reflection.cpp -lboost_json
// ============================================================

#include <meta>

// Standard library
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <charconv>
#include <chrono>
#include <print>
#include <format>
#include <type_traits>
#include <concepts>
#include <cassert>
#include <cctype>
#include <cstring>
#include <algorithm>

// Third-party JSON libraries
#include <nlohmann/json.hpp>
#include <rapidjson/document.h>
#include <boost/json.hpp>

// ============================================================
// §1  JsonVal — custom tagged-union intermediate representation
// ============================================================

struct JsonVal {
    enum class Kind { Null, Bool, Int, Double, String, Array, Object }
        kind = Kind::Null;

    bool        b   = false;
    long long   i   = 0;
    double      d   = 0.0;
    std::string s;
    std::vector<JsonVal>                         arr;
    std::vector<std::pair<std::string, JsonVal>> obj;

    static JsonVal null_()             { return JsonVal{}; }
    static JsonVal bool_(bool v)       { JsonVal jv; jv.kind=Kind::Bool;   jv.b=v; return jv; }
    static JsonVal int_(long long v)   { JsonVal jv; jv.kind=Kind::Int;    jv.i=v; return jv; }
    static JsonVal dbl_(double v)      { JsonVal jv; jv.kind=Kind::Double; jv.d=v; return jv; }
    static JsonVal str_(std::string v) { JsonVal jv; jv.kind=Kind::String; jv.s=std::move(v); return jv; }

    bool               as_bool()   const { return b; }
    long long          as_int()    const { return i; }
    double             as_double() const { return kind==Kind::Int ? double(i) : d; }
    const std::string& as_string() const { return s; }

    const JsonVal* find(std::string_view key) const {
        for (auto& [k, v] : obj)
            if (k == key) return &v;
        return nullptr;
    }
};

// ============================================================
// §2  Custom JSON parser (recursive descent)
// ============================================================

class JsonParser {
    const char* p_;
    const char* end_;

    void skip_ws() noexcept {
        while (p_ < end_ && std::isspace((unsigned char)*p_)) ++p_;
    }

    JsonVal parse_string() {
        ++p_;
        std::string out; out.reserve(32);
        while (p_ < end_ && *p_ != '"') {
            if (*p_ == '\\' && p_+1 < end_) {
                ++p_;
                switch (*p_) {
                    case '"':  out+='"';  break; case '\\': out+='\\'; break;
                    case '/':  out+='/';  break; case 'n':  out+='\n'; break;
                    case 'r':  out+='\r'; break; case 't':  out+='\t'; break;
                    default:   out+=*p_;  break;
                }
            } else { out+=*p_; }
            ++p_;
        }
        if (p_<end_) ++p_;
        return JsonVal::str_(std::move(out));
    }

    JsonVal parse_number() {
        const char* start = p_; bool f = false;
        if (p_<end_ && *p_=='-') ++p_;
        while (p_<end_ && std::isdigit((unsigned char)*p_)) ++p_;
        if (p_<end_ && *p_=='.') { f=true; ++p_; while (p_<end_ && std::isdigit((unsigned char)*p_)) ++p_; }
        if (p_<end_ && (*p_=='e'||*p_=='E')) {
            f=true; ++p_;
            if (p_<end_ && (*p_=='+'||*p_=='-')) ++p_;
            while (p_<end_ && std::isdigit((unsigned char)*p_)) ++p_;
        }
        if (f) { double v=0; std::from_chars(start,p_,v); return JsonVal::dbl_(v); }
        else   { long long v=0; std::from_chars(start,p_,v); return JsonVal::int_(v); }
    }

    void expect(std::string_view lit) {
        for (char c : lit) {
            if (p_>=end_||*p_!=c)
                throw std::runtime_error("Parse error");
            ++p_;
        }
    }

    JsonVal parse_array() {
        ++p_; JsonVal jv; jv.kind=JsonVal::Kind::Array;
        skip_ws(); if (p_<end_&&*p_==']') {++p_; return jv;}
        while (p_<end_) {
            jv.arr.push_back(parse_value()); skip_ws();
            if (p_<end_&&*p_==']') {++p_; break;}
            if (p_<end_&&*p_==',') ++p_;
        }
        return jv;
    }

    JsonVal parse_object() {
        ++p_; JsonVal jv; jv.kind=JsonVal::Kind::Object;
        skip_ws(); if (p_<end_&&*p_=='}') {++p_; return jv;}
        while (p_<end_) {
            skip_ws();
            if (p_>=end_||*p_!='"') throw std::runtime_error("Expected key");
            auto key = parse_string().s;
            skip_ws(); if (p_<end_&&*p_==':') ++p_;
            jv.obj.push_back({std::move(key), parse_value()});
            skip_ws();
            if (p_<end_&&*p_=='}') {++p_; break;}
            if (p_<end_&&*p_==',') ++p_;
        }
        return jv;
    }

public:
    JsonVal parse_value() {
        skip_ws();
        if (p_>=end_) throw std::runtime_error("Unexpected EOF");
        switch (*p_) {
            case '"': return parse_string();
            case '{': return parse_object();
            case '[': return parse_array();
            case 't': expect("true");  return JsonVal::bool_(true);
            case 'f': expect("false"); return JsonVal::bool_(false);
            case 'n': expect("null");  return JsonVal::null_();
            default:  return parse_number();
        }
    }
    JsonVal parse(std::string_view src) {
        p_=src.data(); end_=p_+src.size(); return parse_value();
    }
};

inline JsonVal parse_json(std::string_view s) { return JsonParser{}.parse(s); }

// ============================================================
// §3  File I/O
// ============================================================

inline std::string read_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("Cannot open: " + path.string());
    auto sz = f.tellg();
    std::string buf(sz, '\0');
    f.seekg(0); f.read(buf.data(), sz);
    return buf;
}

// ============================================================
// §4  String escaping (for custom serializer)
// ============================================================

inline std::string json_escape(const std::string& s) {
    std::string out; out.reserve(s.size()+2); out+='"';
    for (char c : s) {
        switch (c) {
            case '"':  out+="\\\""; break; case '\\': out+="\\\\"; break;
            case '\n': out+="\\n";  break; case '\r': out+="\\r";  break;
            case '\t': out+="\\t";  break; default:   out+=c;      break;
        }
    }
    out+='"'; return out;
}

// ============================================================
// §5  Domain structs — zero annotation, plain aggregates
// ============================================================

struct Address {
    std::string street;
    std::string city;
    std::string country;
};

struct Person {
    std::string name;
    int         age;
    double      score;
    bool        active;
    Address     address;
};

struct Dataset {
    std::string          source;
    int                  version;
    std::vector<Person>  persons;
};

// ============================================================
// §6  Reflection-based JSON (de)serialization  — C++26 P2996
// ============================================================

template<typename T> std::string to_json(const T& val);

template<typename T>
std::string to_json_val(const T& v) {
    if constexpr (std::is_same_v<T, bool>) {
        return v ? "true" : "false";
    } else if constexpr (std::is_same_v<T, std::string>) {
        return json_escape(v);
    } else if constexpr (std::is_integral_v<T>) {
        return std::to_string(v);
    } else if constexpr (std::is_floating_point_v<T>) {
        auto s = std::format("{}", v);
        if (s.find('.')==std::string::npos && s.find('e')==std::string::npos) s+=".0";
        return s;
    } else if constexpr (!std::is_same_v<T,std::string> &&
                         requires { v.begin(); v.end(); typename T::value_type; }) {
        std::string out = "["; bool first = true;
        for (const auto& item : v) {
            if (!first) out += ','; first = false;
            out += to_json_val(item);
        }
        out += ']'; return out;
    } else if constexpr (std::is_class_v<T>) {
        return to_json(v);
    } else {
        return "null";
    }
}

template<typename T>
    requires (std::is_class_v<T> && !std::is_same_v<T, std::string>)
std::string to_json(const T& obj) {
    constexpr auto ctx = std::meta::access_context::unchecked();
    std::string result; result.reserve(64); result += '{';
    bool first = true;
    template for (constexpr auto mem :
                  define_static_array(nonstatic_data_members_of(^^T, ctx))) {
        if (!first) result += ','; first = false;
        constexpr std::string_view name = identifier_of(mem);
        result += '"'; result.append(name.data(), name.size()); result += "\":";
        result += to_json_val(obj.[:mem:]);
    }
    result += '}'; return result;
}

template<typename T> T from_json(const JsonVal& jv);

template<typename T>
T from_json_val(const JsonVal& jv) {
    if constexpr (std::is_same_v<T, bool>) {
        return jv.as_bool();
    } else if constexpr (std::is_same_v<T, std::string>) {
        return jv.as_string();
    } else if constexpr (std::is_integral_v<T>) {
        return static_cast<T>(jv.as_int());
    } else if constexpr (std::is_floating_point_v<T>) {
        return static_cast<T>(jv.as_double());
    } else if constexpr (!std::is_same_v<T,std::string> &&
                         requires { typename T::value_type; }) {
        T result; result.reserve(jv.arr.size());
        for (const auto& item : jv.arr)
            result.push_back(from_json_val<typename T::value_type>(item));
        return result;
    } else if constexpr (std::is_class_v<T>) {
        return from_json<T>(jv);
    } else {
        return T{};
    }
}

template<typename T>
    requires (std::is_class_v<T> && !std::is_same_v<T, std::string>)
T from_json(const JsonVal& jv) {
    constexpr auto ctx = std::meta::access_context::unchecked();
    T result{};
    template for (constexpr auto mem :
                  define_static_array(nonstatic_data_members_of(^^T, ctx))) {
        constexpr std::string_view name = identifier_of(mem);
        if (const JsonVal* field = jv.find(name)) {
            using MemberType = typename [: type_of(mem) :];
            result.[:mem:] = from_json_val<MemberType>(*field);
        }
    }
    return result;
}

// Full pipeline: raw string → Dataset  (parse + reflection deser)
inline Dataset reflection_from_string(const std::string& raw) {
    return from_json<Dataset>(parse_json(raw));
}

// ============================================================
// §7  Manual / Classical deserialization
// ============================================================

[[nodiscard]] inline Person manual_person_from_json(const JsonVal& jv) {
    Person p;
    if (const auto* f = jv.find("name"))   p.name   = f->as_string();
    if (const auto* f = jv.find("age"))    p.age    = int(f->as_int());
    if (const auto* f = jv.find("score"))  p.score  = f->as_double();
    if (const auto* f = jv.find("active")) p.active = f->as_bool();
    if (const auto* a = jv.find("address")) {
        if (const auto* f = a->find("street"))  p.address.street  = f->as_string();
        if (const auto* f = a->find("city"))    p.address.city    = f->as_string();
        if (const auto* f = a->find("country")) p.address.country = f->as_string();
    }
    return p;
}

[[nodiscard]] inline Dataset manual_dataset_from_json(const JsonVal& jv) {
    Dataset d;
    if (const auto* f = jv.find("source"))  d.source  = f->as_string();
    if (const auto* f = jv.find("version")) d.version = int(f->as_int());
    if (const auto* pa = jv.find("persons")) {
        d.persons.reserve(pa->arr.size());
        for (const auto& pjv : pa->arr)
            d.persons.push_back(manual_person_from_json(pjv));
    }
    return d;
}

// Full pipeline: raw string → Dataset  (parse + manual deser)
inline Dataset manual_from_string(const std::string& raw) {
    return manual_dataset_from_json(parse_json(raw));
}

// ============================================================
// §8  nlohmann/json deserialization
// ============================================================

[[nodiscard]] inline Dataset nlohmann_from_string(const std::string& raw) {
    auto j = nlohmann::json::parse(raw);
    Dataset d;
    d.source  = j["source"].get<std::string>();
    d.version = j["version"].get<int>();
    const auto& persons = j["persons"];
    d.persons.reserve(persons.size());
    for (const auto& pj : persons) {
        Person p;
        p.name   = pj["name"].get<std::string>();
        p.age    = pj["age"].get<int>();
        p.score  = pj["score"].get<double>();
        p.active = pj["active"].get<bool>();
        p.address.street  = pj["address"]["street"].get<std::string>();
        p.address.city    = pj["address"]["city"].get<std::string>();
        p.address.country = pj["address"]["country"].get<std::string>();
        d.persons.push_back(std::move(p));
    }
    return d;
}

// ============================================================
// §9  RapidJSON deserialization
// ============================================================

[[nodiscard]] inline Dataset rapidjson_from_string(const std::string& raw) {
    rapidjson::Document doc;
    doc.Parse(raw.c_str(), raw.size());
    if (doc.HasParseError())
        throw std::runtime_error("RapidJSON parse error");

    Dataset d;
    d.source  = doc["source"].GetString();
    d.version = doc["version"].GetInt();
    const auto& persons = doc["persons"].GetArray();
    d.persons.reserve(persons.Size());
    for (const auto& pj : persons) {
        Person p;
        p.name   = pj["name"].GetString();
        p.age    = pj["age"].GetInt();
        p.score  = pj["score"].GetDouble();
        p.active = pj["active"].GetBool();
        p.address.street  = pj["address"]["street"].GetString();
        p.address.city    = pj["address"]["city"].GetString();
        p.address.country = pj["address"]["country"].GetString();
        d.persons.push_back(std::move(p));
    }
    return d;
}

// ============================================================
// §10  Boost.JSON deserialization
// ============================================================

[[nodiscard]] inline Dataset boostjson_from_string(const std::string& raw) {
    auto jv  = boost::json::parse(raw);
    const auto& obj = jv.as_object();

    Dataset d;
    d.source  = std::string(obj.at("source").as_string());
    d.version = static_cast<int>(obj.at("version").as_int64());

    const auto& persons = obj.at("persons").as_array();
    d.persons.reserve(persons.size());
    for (const auto& pjv : persons) {
        const auto& pobj = pjv.as_object();
        Person p;
        p.name   = std::string(pobj.at("name").as_string());
        p.age    = static_cast<int>(pobj.at("age").as_int64());
        p.score  = pobj.at("score").as_double();
        p.active = pobj.at("active").as_bool();
        const auto& addr = pobj.at("address").as_object();
        p.address.street  = std::string(addr.at("street").as_string());
        p.address.city    = std::string(addr.at("city").as_string());
        p.address.country = std::string(addr.at("country").as_string());
        d.persons.push_back(std::move(p));
    }
    return d;
}

// ============================================================
// §11  Dataset generator
// ============================================================

static const char* first_names[] = {
    "Alice","Bob","Carol","David","Eve","Frank","Grace","Henry",
    "Iris","Jack","Kate","Leo","Mia","Noah","Olivia","Paul",
    "Quinn","Rose","Sam","Tina","Uma","Victor","Wendy","Xander","Yara","Zack"
};
static const char* last_names[] = {
    "Martin","Chen","Dupont","Smith","Patel","Garcia","Kim",
    "Nakamura","Okonkwo","Ferreira","Ivanov","Müller","Tanaka",
    "Lopez","Brown","Wilson"
};
static const char* cities[]    = { "Paris","Tokyo","New York","Berlin","Cairo","São Paulo","Mumbai","Sydney","Toronto","Seoul","Lagos","London" };
static const char* countries[] = { "France","Japan","USA","Germany","Egypt","Brazil","India","Australia","Canada","South Korea","Nigeria","UK" };

void generate_dataset(const std::filesystem::path& path, int n) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream f(path);
    if (!f) throw std::runtime_error("Cannot write " + path.string());

    f << "{\n  \"source\": \"gcc-cpp-rules-lab/20_json_reflection\",\n  \"version\": 1,\n  \"persons\": [\n";
    for (int k = 0; k < n; ++k) {
        int fi  = k % std::size(first_names);
        int li  = (k / (int)std::size(first_names)) % std::size(last_names);
        int ci  = k % std::size(cities);
        int cni = k % std::size(countries);
        std::string name    = std::string(first_names[fi]) + " " + last_names[li];
        std::string city    = cities[ci];
        std::string country = countries[cni];
        int    age          = 20 + (k % 45);
        double score        = 5.0 + (k % 50) * 0.1;
        bool   active       = (k % 3 != 0);
        int    street_num   = 1 + (k % 999);

        f << std::format(
            "    {{\"name\":{},\"age\":{},\"score\":{:.1f},"
            "\"active\":{},\"address\":{{\"street\":\"{} Main St\","
            "\"city\":{},\"country\":{}}}}}",
            json_escape(name), age, score,
            active ? "true" : "false", street_num,
            json_escape(city), json_escape(country));
        if (k + 1 < n) f << ",\n";
    }
    f << "\n  ]\n}\n";
}

// ============================================================
// §12  Benchmark harness
// ============================================================

using Clock = std::chrono::high_resolution_clock;
using Ms    = std::chrono::duration<double, std::milli>;

struct BenchResult { std::string label; double ms; double ops_per_sec; };

template<typename Fn>
BenchResult bench(std::string_view label, int n, Fn&& fn) {
    for (int i = 0; i < std::min(n/5, 3); ++i) fn();   // warmup
    auto t0 = Clock::now();
    for (int i = 0; i < n; ++i) fn();
    double ms  = Ms(Clock::now() - t0).count();
    double ops = double(n) / (ms * 0.001);
    std::println("  {:<52} {:>8.2f} ms   {:>10.0f} ops/s",
                 label, ms, ops);
    return { std::string(label), ms, ops };
}

// ============================================================
// §13  Main
// ============================================================

int main() {
    std::println("╔══════════════════════════════════════════════════════════════╗");
    std::println("║  C++26 P2996 Reflection vs JSON Libraries — Benchmark        ║");
    std::println("║  Contenders: Reflection · Manual · nlohmann · RapidJSON · Boost.JSON ║");
    std::println("╚══════════════════════════════════════════════════════════════╝\n");

    // ── Generate dataset ───────────────────────────────────────────
    constexpr int DATASET_SIZE = 10'000;
    const std::filesystem::path data_file = "data/persons.json";

    std::println("[ Step 1 ] Generating {} …", data_file.string());
    generate_dataset(data_file, DATASET_SIZE);
    auto file_size = std::filesystem::file_size(data_file);
    std::println("           {:>6} records  |  {:>7.1f} KB\n", DATASET_SIZE, file_size/1024.0);

    // ── Read file ──────────────────────────────────────────────────
    std::println("[ Step 2 ] Reading file …");
    std::string raw;
    {
        auto t0 = Clock::now();
        raw = read_file(data_file);
        std::println("           {:.1f} KB in {:.2f} ms\n",
                     raw.size()/1024.0, Ms(Clock::now()-t0).count());
    }

    // ── Correctness check ──────────────────────────────────────────
    std::println("[ Step 3 ] Correctness check (all methods agree) …");
    {
        auto ds_ref    = reflection_from_string(raw);
        auto ds_manual = manual_from_string(raw);
        auto ds_nloh   = nlohmann_from_string(raw);
        auto ds_rapid  = rapidjson_from_string(raw);
        auto ds_boost  = boostjson_from_string(raw);

        for (size_t k = 0; k < ds_ref.persons.size(); ++k) {
            assert(ds_ref.persons[k].name   == ds_manual.persons[k].name);
            assert(ds_ref.persons[k].name   == ds_nloh.persons[k].name);
            assert(ds_ref.persons[k].name   == ds_rapid.persons[k].name);
            assert(ds_ref.persons[k].name   == ds_boost.persons[k].name);
            assert(ds_ref.persons[k].age    == ds_manual.persons[k].age);
            assert(ds_ref.persons[k].age    == ds_nloh.persons[k].age);
            assert(ds_ref.persons[k].age    == ds_rapid.persons[k].age);
            assert(ds_ref.persons[k].age    == ds_boost.persons[k].age);
        }
        std::println("           ✓  {} records match across all 5 implementations\n",
                     ds_ref.persons.size());
    }

    // ── Benchmark ──────────────────────────────────────────────────
    constexpr int BENCH_ITERS = 15;
    volatile size_t sink = 0;

    std::println("┌──────────────────────────────────────────────────────────────────────┐");
    std::println("│  Full pipeline:  raw string → Dataset  ({} passes × {} records)    │",
                 BENCH_ITERS, DATASET_SIZE);
    std::println("│  Each pass includes: text parse  +  struct deserialization           │");
    std::println("└──────────────────────────────────────────────────────────────────────┘\n");

    auto r_reflect = bench("1. Reflection   (C++26 P2996 template for)", BENCH_ITERS,
        [&]{ auto ds = reflection_from_string(raw);  sink += ds.persons.size(); });

    auto r_manual  = bench("2. Manual       (hand-written field access)", BENCH_ITERS,
        [&]{ auto ds = manual_from_string(raw);       sink += ds.persons.size(); });

    auto r_nloh    = bench("3. nlohmann/json", BENCH_ITERS,
        [&]{ auto ds = nlohmann_from_string(raw);     sink += ds.persons.size(); });

    auto r_rapid   = bench("4. RapidJSON", BENCH_ITERS,
        [&]{ auto ds = rapidjson_from_string(raw);    sink += ds.persons.size(); });

    auto r_boost   = bench("5. Boost.JSON", BENCH_ITERS,
        [&]{ auto ds = boostjson_from_string(raw);    sink += ds.persons.size(); });

    // ── Summary table ───────────────────────────────────────────────
    std::println("\n┌──────────────────────────────────────────────────────────────────────┐");
    std::println("│  Summary  (ratio vs Reflection — lower is faster)                    │");
    std::println("├──────────────────────────────┬───────────┬───────────┬───────────────┤");
    std::println("│  Method                      │  Time(ms) │  ops/s    │  vs Reflect   │");
    std::println("├──────────────────────────────┼───────────┼───────────┼───────────────┤");

    auto row = [&](const BenchResult& r) {
        double ratio = r.ms / r_reflect.ms;
        std::println("│  {:<28}  │  {:>7.2f}  │  {:>7.0f}  │  {:<13}  │",
                     r.label.substr(0, 28), r.ms, r.ops_per_sec,
                     std::format("{:.2f}x", ratio));
    };
    row(r_reflect); row(r_manual); row(r_nloh); row(r_rapid); row(r_boost);
    std::println("└──────────────────────────────┴───────────┴───────────┴───────────────┘");

    // ── CSV output for plotter ──────────────────────────────────────
    std::println("\n--- BENCHMARK_CSV ---");
    std::println("method,ms,ops_per_sec");
    for (const auto& r : {r_reflect, r_manual, r_nloh, r_rapid, r_boost})
        std::println("{},{:.4f},{:.2f}", r.label, r.ms, r.ops_per_sec);
    std::println("--- END_CSV ---");

    std::println("\n  (sink={} — prevents dead-code elimination)", (size_t)sink);
    return 0;
}
