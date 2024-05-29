#ifndef COMMON_UTIL_HPP
#define COMMON_UTIL_HPP

#include <cstdlib>
#include <string>
#include <parlay/parallel.h>

namespace env {

inline auto constexpr PARLAY_NUM_THREADS = "PARLAY_NUM_THREADS";

auto Set(char const* name, char const* value) -> bool {
    return setenv(name, value, 1);
}

auto GetNThreads() {
    return strtoll(getenv(PARLAY_NUM_THREADS), nullptr, 10);
}

auto SetNThreads(size_t n) -> void {
    Set(env::PARLAY_NUM_THREADS, std::to_string(n).c_str());
}

}

template<typename F>
auto DoParOnConditions(bool cond1, bool cond2, F&& f1, F&& f2) -> void {
    if (cond1 && cond2) {
        parlay::par_do(std::forward(f1), std::forward(f2));
    } else if (cond1) {
        f1();
    } else if (cond2) {
        f2();
    }
}

#endif //COMMON_UTIL_HPP
