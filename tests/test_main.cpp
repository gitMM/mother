#include <string_view>

#include "test.h"

// Runs every test whose name starts with argv[1], or all tests with no argument.
int main(int argc, char** argv) {
    const std::string_view prefix = argc > 1 ? argv[1] : "";
    int run = 0;
    for (const mtest::Case& c : mtest::registry()) {
        if (!std::string_view(c.name).starts_with(prefix)) continue;
        ++run;
        const int before = mtest::failures();
        c.fn();
        std::printf("%s %s\n", mtest::failures() == before ? "ok  " : "FAIL", c.name);
    }
    std::printf("%d tests, %d failed checks\n", run, mtest::failures());
    if (run == 0) return 1;  // a filter that matches nothing is a mistake
    return mtest::failures() == 0 ? 0 : 1;
}
