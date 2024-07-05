#ifndef PBSL_UTIL_HPP
#define PBSL_UTIL_HPP

#include <random>
#include <parlay/sequence.h>

#include "config.hpp"


namespace pbsl::util {

namespace types {

template<typename T>
using Seq = parlay::sequence<T>;

using KeySeq = Seq<config::Key>;

} // namespace types

namespace random {

auto NextGeometric(double p = 0.5) -> size_t {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    return std::geometric_distribution<size_t>(p)(gen);
}

}

struct Constants {
    static constexpr config::Key MIN_KEY = std::numeric_limits<config::Key>::min();
    static constexpr config::Key MAX_KEY = std::numeric_limits<config::Key>::max();
};

#define Debug if constexpr (config::DebugEnabled())

constexpr auto Assert(bool condition) -> void { Debug { assert(condition); } }

template<typename T>
constexpr T* EnsureNotNull(T* ptr) {
    Assert(ptr != nullptr);
    return ptr;
}

} // namespace pbsl::util

#endif //PBSL_UTIL_HPP
