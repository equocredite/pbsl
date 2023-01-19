#ifndef PBSL_UTIL_HPP
#define PBSL_UTIL_HPP

#include <random>

#include "config.hpp"

namespace pbsl::util {

bool TossCoin(double p) {
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    std::bernoulli_distribution dis(p);
    return dis(gen);
}

bool TossFairCoin() {
    return TossCoin(0.5);
}

void Assert(bool condition) {
    if constexpr (config::DEBUG) {
        assert(condition);
    }
}

}

#endif //PBSL_UTIL_HPP
