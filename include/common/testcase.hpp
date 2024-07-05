#ifndef PBSL_TESTCASE_HPP
#define PBSL_TESTCASE_HPP

#include <parlay/sequence.h>
#include <parlay/primitives.h>

#include <random>

#include "config.hpp"
#include "util.hpp"
#include "timer.hpp"

struct TestDescription {
    size_t n_proc;
    size_t initial_size;
    size_t batch_size;
};

struct TestGroupDescription {
    size_t n_proc;
    std::vector<TestDescription> test_descriptions;
};

struct Test {
    parlay::sequence<size_t> initial;
    parlay::sequence<size_t> batch;
};

struct TestGroup {
    std::vector<Test> tests;

    auto Size() const -> size_t { return tests.size(); }
};

auto GenerateTest(TestDescription desc) -> Test {
    //std::cout << "before keys\n";
    auto keys = parlay::tabulate(desc.initial_size + desc.batch_size, [](size_t i) { return i + 1; });
    //std::cout << "before shuffle\n";
    keys = parlay::random_shuffle(keys);
    //std::cout << "after shuffle\n";
    //std::shuffle(keys.begin(), keys.end(), std::mt19937(std::random_device()()));
    auto initial = keys.substr(0, desc.initial_size);
    auto batch = keys.substr(desc.initial_size, desc.batch_size);
    //std::cout << "before sort\n";
    parlay::sort_inplace(initial);
    parlay::sort_inplace(batch);
    return Test(std::move(initial), std::move(batch));
}

auto RunTest(Test test) -> size_t {
    return 0;
}

template<typename C>
auto RunTestGroup(TestGroup group) -> std::vector<size_t> {
    std::vector<size_t> times(group.Size());
    for (auto& test : group.tests) {
        auto duration = RunTest(test);
        times.push_back(duration);
    }
    return times;
}

#endif //PBSL_TESTCASE_HPP
