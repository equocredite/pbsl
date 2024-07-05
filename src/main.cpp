#include <iostream>

#include "skip_list.hpp"
#include "common/timer.hpp"
#include "pbsl/config.hpp"
#include "common/util.hpp"
#include "common/testcase.hpp"

using namespace pbsl;
using util::types::Seq;
using K = config::Key;

void TestSmallMerge() {
    size_t const n = 100;
    Seq<size_t> keys;
    for (size_t i = 1; i < n; ++i) {
        keys.push_back(i * 2);
    }
    auto sl = SkipList::FromOrderedKeys(keys);
    auto nodes = sl.DebugGetNodes();
    size_t height = nodes.front()->Height();
    for (size_t level = 0; level < height; ++level) {
        std::cout << level << ": ";
        for (Node* node = nodes.front(); node != nullptr; node = node->Next(level)) {
            std::cout << node->key << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;

    Seq<size_t> batch;
    for (size_t i = 1; i < n; ++i) {
        batch.push_back(i * 2 + 1);
    }
    sl.InsertOrdered(batch);

    nodes = sl.DebugGetNodes();
    for (size_t level = 0; level < sl.Height(); ++level) {
        std::cout << level << ": ";
        for (Node* node = nodes.front(); node != nullptr; node = node->Next(level)) {
            std::cout << node->key << " ";
        }
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

void TestMaxSize() {
    env::SetNThreads(8);
    size_t const m = 2e7;
    TestDescription desc(8, m, m);
    auto test = GenerateTest(desc);
    auto sl = SkipList::FromOrderedKeys(test.initial);
    auto duration = MeasureTimeMillis([&]() {
        sl.InsertOrdered(test.batch);
    });
    std::cout << static_cast<long double>(duration) << std::endl;
}

void TestDurationByM() {
    size_t const n = 2e7;
    for (size_t m : {1e4, 1e5, 1e6, 2e6, 4e6, 6e6, 8e6, 1e7, 2e7}) {
        TestDescription desc(8, n, m);
        auto test = GenerateTest(desc);
        auto sl = SkipList::FromOrderedKeys(test.initial);
        auto duration = MeasureTimeMillis([&]() {
            sl.InsertOrdered(test.batch);
        });
        std::cout << m << ": " << static_cast<long double>(duration) / m  << "," << std::endl;
    }
}

void TestDurationByMWithEqualN() {
    for (size_t m : {1e4, 1e5, 1e6, 2e6, 4e6, 6e6, 8e6, 1e7, 2e7}) {
        TestDescription desc(8, m, m);
        auto test = GenerateTest(desc);
        auto sl = SkipList::FromOrderedKeys(test.initial);
        auto duration = MeasureTimeMillis([&]() {
            sl.InsertOrdered(test.batch);
        });
        std::cout << m << ": " << static_cast<long double>(duration) / m  << "," << std::endl;
    }
}

void TestSpeedup() {
    size_t const n = 2e7;
    TestDescription desc(8, n, n);
    auto test = GenerateTest(desc);
    std::vector<size_t> durs;
    for (size_t p = 1; p <= 8; ++p) {
        std::cout << "p = " << p << ": ";
        env::SetNThreads(p);
        auto sl = SkipList::FromOrderedKeys(test.initial);
        auto duration = MeasureTimeMillis([&]() {
            sl.InsertOrdered(test.batch);
        });
        std::cout << duration << std::endl;
        durs.push_back(duration);
    }
    //for (size_t d : durs) std::cout << d << std::endl;
    std::cout << std::endl;
    for (size_t i = 0; i < durs.size(); ++i) std::cout << static_cast<double>(durs[0]) / static_cast<double>(durs[i]) << std::endl;
}

void TestWithSetNWorkers() {
    size_t const n = 2e7;
    TestDescription desc(8, n, n);
    auto test = GenerateTest(desc);
    auto sl = SkipList::FromOrderedKeys(test.initial);
    auto duration = MeasureTimeMillis([&]() {
        sl.InsertOrdered(test.batch);
    });
    std::cout << "p = " << std::getenv(env::PARLAY_NUM_THREADS) << std::endl;
    std::cout << duration << std::endl;
}

int main() {
    TestWithSetNWorkers();
//    std::cout << std::getenv(env::PARLAY_NUM_THREADS) << std::endl;

}
