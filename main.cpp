#include <iostream>
#include <vector>

#include <parlay/parallel.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "skip_list.h"

int main() {
    auto s = parlay::tabulate(50, [](size_t i) { return static_cast<int>(i); });
    auto sl = pbsl::SkipList<int32_t>::FromOrderedKeys(s);
    for (auto& layer : sl.layers) {
        auto node = layer.front();
        while (node != nullptr) {
            std::cout << node->key << " ";
            node = node->right;
        }
        std::cout << std::endl;
    }
}
