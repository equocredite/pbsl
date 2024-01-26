#include <iostream>

#include "skip_list.hpp"
#include "common/timer.hpp"
#include "pbsl/config.hpp"
#include "common/util.hpp"
#include "common/testcase.hpp"

using namespace pbsl;
using util::types::Seq;
using K = config::Key;

int main() {
//    auto old_keys = std::vector<size_t>{0, 1, 3, 4, 6, 7, 10, 12};
//    auto new_keys = std::vector<size_t>{2, 5, 8, 9, 11, 100};
//    auto old_nodes = parlay::tabulate(old_keys.size(), [&](size_t i) { return new Node(old_keys[i], 1); });
//    auto new_nodes = parlay::tabulate(new_keys.size(), [&](size_t i) { return new Node(new_keys[i], 1); });
//    auto res = SkipList::FindStartingNodesInCritLayer(old_nodes, new_nodes);
//    for (auto& node : res) {
//        std::cout << node->key << " ";
//    }
//    std::cout << std::endl;
    env::SetNThreads(8);
    Seq<K> keys;
    size_t const n = 20;
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

    Seq<K> batch;
    for (size_t i = 1; i < 100; ++i) {
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

//    auto duration = MeasureTimeMillis([&]() {
//        auto layer = sl.GetLayer(2);
//        //for (size_t i = 0; i < 100; ++i) std::cout << layer[i]->key << " ";
//    });
//    std::cout << std::endl << duration << std::endl;


//    Seq<K> keys{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
//    auto sl = SkipList::FromOrderedKeys(keys);
//    auto nodes = sl.DebugGetNodes();
//    size_t height = nodes.front()->Height();
//    for (size_t level = 0; level < height; ++level) {
//        std::cout << level << ": ";
//        for (Node* node = nodes.front(); node != nullptr; node = node->Next(level)) {
//            std::cout << node->key << " ";
//        }
//        std::cout << std::endl;
//    }
//    std::cout << std::endl;
//    size_t level = 2;
//    auto layer = sl.GetLayer(level);
//    for (auto const& node : layer) {
//        std::cout << node->key << " ";
//    }
//    std::cout << std::endl;
}
