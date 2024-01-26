#ifndef PBSL_TIMER_HPP
#define PBSL_TIMER_HPP

#include <chrono>

template<typename F>
auto MeasureTimeMillis(F&& f) -> long {
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::milliseconds;

    auto start = high_resolution_clock::now();
    f();
    auto finish = high_resolution_clock::now();
    return duration_cast<milliseconds>(finish - start).count();
}

#endif //PBSL_TIMER_HPP
