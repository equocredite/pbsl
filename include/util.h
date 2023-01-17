//
// Created by equocredite on 06/01/23.
//

#ifndef PBSL_UTIL_H
#define PBSL_UTIL_H

#include <random>

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

}

#endif //PBSL_UTIL_H
