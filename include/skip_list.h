#ifndef PBSL_SKIP_LIST_H
#define PBSL_SKIP_LIST_H

#include <cinttypes>
#include <climits>
#include <concepts>

#include <parlay/parallel.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "config.h"
#include "util.h"

namespace pbsl {

template<std::totally_ordered K = int32_t>
class SkipList {
    template<typename T> using seq = parlay::sequence<T>;

  public:
    void InsertBatch(const seq<K>& keys) {
        Merge(FromOrderedKeys(keys));
    }

    static SkipList FromKeys(const seq<K>& keys) {
        if constexpr (std::is_integral_v<K> && std::is_unsigned_v<K>) {
            return FromOrderedKeys(parlay::integer_sort(keys));
        } else {
            return FromOrderedKeys(parlay::sort(keys));
        }
    }

    static SkipList FromKeys(seq<K>&& keys) {
        if constexpr (std::is_integral_v<K> && std::is_unsigned_v<K>) {
            parlay::integer_sort_inplace(keys);
        } else {
            parlay::sort_inplace(keys);
        }
        return FromOrderedKeys(keys);
    }

    static SkipList<K> FromOrderedKeys(const seq<K>& keys) {
        SkipList<K> sl;
        parlay::sequence<Node*> layer{new Node{MINUS_INF}};
        layer.append(parlay::map(keys, [](K key) { return new Node{key}; }));
        layer.push_back(new Node{PLUS_INF});
        parlay::parallel_for(0, layer.size() - 1, [&](size_t i) { layer[i]->right = layer[i + 1]; });
        sl.layers.push_back(layer);
        while (layer.size() > MAX_NODES_ON_TOP_LEVEL) {
            layer = MakeNextLayer(layer);
            sl.layers.push_back(layer);
        }
        return sl;
    }


  //private:
    void Merge(SkipList<K>&& other) {

    }

    static constexpr K PLUS_INF  = std::numeric_limits<K>::max();
    static constexpr K MINUS_INF = std::numeric_limits<K>::min();

    static constexpr size_t MAX_NODES_ON_TOP_LEVEL = 10;
    static constexpr bool PROBABILISTIC = true;

    struct Node {
        K key;
        Node* right = nullptr;
        Node* down = nullptr;

        bool IsSentinel() {
            return key == PLUS_INF || key == MINUS_INF;
        }
    };

    seq<seq<Node*>> layers;

    static seq<Node*> SiftLayer(const seq<Node*>& layer) {
        return parlay::filter(layer, [](Node* node) {
            if constexpr (PROBABILISTIC) {
                return node->IsSentinel() || pbsl::util::TossFairCoin();
            } else {
                return true;
            }
        });
    }

    static seq<Node*> MakeNextLayer(const seq<Node*>& layer) {
#ifdef DEBUG
        assert(layer.size() >= 2 && layer.front()->IsSentinel() && layer.back()->IsSentinel());
#endif
        if (layer.size() <= 2) {
            return layer;
        }
        auto next_layer = SiftLayer(layer);
        parlay::parallel_for(0, next_layer.size(), [&](size_t i) {
            next_layer[i] = new Node{next_layer[i]->key, nullptr, next_layer[i]};
        });
        parlay::parallel_for(0, next_layer.size() - 1, [&](size_t i) {
            next_layer[i]->right = next_layer[i + 1];
        });
        return next_layer;
    }

};

}

#endif //PBSL_SKIP_LIST_H
