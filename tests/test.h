#pragma once

// Minimal test harness: TEST(name) registers a case; CHECK and CHECK_EQ count failures.
// The brief allows no dependencies beyond SDL3 and Luau, so there is no test framework.

#include <cstdio>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

namespace mtest {

struct Case {
    const char* name;
    void (*fn)();
};

inline std::vector<Case>& registry() {
    static std::vector<Case> cases;
    return cases;
}

inline int& failures() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(const char* name, void (*fn)()) { registry().push_back({name, fn}); }
};

template <typename T>
std::string show(const T& v) {
    std::ostringstream os;
    if constexpr (std::is_arithmetic_v<T>) {
        os << +v;  // print uint8_t and int8_t as numbers
    } else {
        os << v;
    }
    return os.str();
}

template <typename A, typename B>
void check_eq(const A& a, const B& b, const char* ea, const char* eb, const char* file, int line) {
    if (a == b) return;
    ++failures();
    std::printf("%s:%d: CHECK_EQ(%s, %s) failed: %s != %s\n", file, line, ea, eb, show(a).c_str(),
                show(b).c_str());
}

}  // namespace mtest

#define TEST(name)                                                    \
    static void name();                                               \
    static const mtest::Registrar name##_registrar(#name, name);      \
    static void name()

#define CHECK(cond)                                                                   \
    do {                                                                              \
        if (!(cond)) {                                                                \
            ++mtest::failures();                                                      \
            std::printf("%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);      \
        }                                                                             \
    } while (0)

#define CHECK_EQ(a, b) mtest::check_eq((a), (b), #a, #b, __FILE__, __LINE__)
