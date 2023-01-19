#ifndef PBSL_SKIP_LIST_HPP
#define PBSL_SKIP_LIST_HPP

#include <cinttypes>
#include <climits>
#include <concepts>
#include <utility>

#include <parlay/parallel.h>
#include <parlay/primitives.h>
#include <parlay/sequence.h>

#include "config.hpp"
#include "util.hpp"
#include "node.hpp"

namespace pbsl {

template<typename K>
concept key_type = std::totally_ordered<K>;

template<key_type K = int32_t>
class SkipList {
    template<typename T>
    using seq = parlay::sequence<T>;

    using node_t = Node<K>;

  public:
    static SkipList<K> FromOrderedKeys(seq<K> const& keys) {
        util::Assert(!keys.empty());

        SkipList<K> sl;
        node_t* left_sentinel = nullptr, right_sentinel = nullptr;
        auto layers = MakeLayers(keys);
        sl.heads_.reserve(layers.size() * 2);
        sl.lens_.reserve(layers.size() * 2);
        for (auto& layer : layers) {
            right_sentinel = node_t::NewRightSentinel(right_sentinel);
            left_sentinel = node_t::NewLeftSentinel(layer.empty() ? right_sentinel : layer.front(), left_sentinel);
            sl.heads_.push_back(left_sentinel);
            sl.tails_.push_back(right_sentinel);
            sl.lens_.push_back(layer.size() + 2);
            if (!layer.empty()) {
                layer.back()->right = right_sentinel;
            } else break;
        }
        return sl;
    }

    static SkipList FromKeys(seq<K> const& keys) {
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

    void InsertOrdered(seq<K> const& keys) {
        Merge(MakeLayers(keys));
    }


  private:
    // decision strategy for propagating a key to a higher layer;
    // deterministic won't actually be used, I guess
    enum class Mode { PROBABILISTIC, DETERMINISTIC };
    enum class Side { LEFT, RIGHT };

    static constexpr size_t MAX_NODES_ON_TOP_LEVEL = 6;
    static constexpr Mode mode = Mode::PROBABILISTIC;

    // factory utilities

    // builds an explicit skip list without any sentinel nodes
    static seq<seq<node_t*>> MakeLayers(seq<K> const& keys) {
        seq<seq<node_t*>> layers{MakeInitialLayer(keys)};
        while (!layers.back().empty()) {
            layers.push_back(MakeNextLayer(layers.back()));
        }
        return layers;
    }

    static seq<node_t*> MakeInitialLayer(seq<K> const& keys) {
        auto layer = parlay::map(keys, [](K key) { node_t::New(key); });
        parlay::parallel_for(0, layer.size() - 1, [&](size_t i) { layer[i]->right = layer[i + 1]; });
        return layer;
    }

    static seq<node_t*> MakeNextLayer(seq<node_t*> const& layer) {
        if (layer.empty()) { return layer; }
        auto next_layer = SiftLayer(layer);
        parlay::parallel_for(0, next_layer.size(), [&](size_t i) {
            next_layer[i] = node_t::New(next_layer[i]->key, nullptr, next_layer[i]);
        });
        parlay::parallel_for(0, next_layer.size() - 1, [&](size_t i) {
            next_layer[i]->right = next_layer[i + 1];
        });
        return next_layer;
    }

    static seq<node_t*> SiftLayer(seq<node_t*> const& layer) {
        return parlay::filter(layer, [](node_t* node) {
            if constexpr (mode == Mode::PROBABILISTIC) {
                return node->IsSentinel() || util::TossFairCoin();
            } else {
                return true; // TODO
            }
        });
    }

    void CoerceHeightAtLeast(size_t min_height) {
        if (GetHeight() >= min_height) return;
        size_t diff = min_height - GetHeight();

        auto right_sentinels = parlay::tabulate(diff, []() { node_t::NewRightSentinel(); });
        auto left_sentinels = parlay::tabulate(diff, [&](size_t i) { node_t::NewLeftSentinel(right_sentinels[i]); });

        parlay::parallel_for(0, diff, [&](size_t i) {
            left_sentinels[i]->right = right_sentinels[i];
            left_sentinels[i]->SetDown(i > 0 ? left_sentinels[i - 1] : heads_.back());
            right_sentinels[i]->SetDown(i > 0 ? right_sentinels[i - 1] : tails_.back());
        });

        heads_.append(std::move(left_sentinels));
        tails_.append(std::move(right_sentinels));
        lens_.append(diff, 2);
    }

    // it's not actually the size of the subtree that gets accumulated in each node;
    // rather something like number of leaves in this subtree if we only keep top k layers
    void CalcSubtreeSizes(node_t* node, size_t height, size_t target_height) {
        util::Assert(node != nullptr && node->right != nullptr);
        bool go_right = (!node->right->has_parent);
        bool go_down = (height != target_height);
        node->subtree_size = static_cast<size_t>(!go_down);
        if (go_right && go_down) {
            size_t right_size;
            size_t down_size;
            parlay::par_do(
                    [&]() { right_size = CalcSubtreeSizes(node->right, height, target_height); },
                    [&]() { down_size = CalcSubtreeSizes(node->down, height - 1, target_height); }
            );
            node->subtree_size += right_size + down_size;
        } else if (go_right) {
            node->subtree_size += CalcSubtreeSizes(node->right, height, target_height);
        } else if (go_down) {
            node->subtree_size += CalcSubtreeSizes(node->down, height - 1, target_height);
        }
        return node->subtree_size;
    }

    void TraverseFromTopAndCopyTargetLayer(node_t* node, size_t height, size_t offset, size_t target_height, seq<node_t *>& target_layer) {
        util::Assert(node != nullptr && node->right != nullptr);
        // if v->down == node->right for some node v, then node->right will be reached from v
        bool go_right = (!node->right->has_parent);
        bool go_down = (height != target_height);
        if (!go_down) target_layer[offset] = node;
        if (go_right && go_down) {
            parlay::par_do(
                    [&]() {
                        TraverseFromTopAndCopyTargetLayer(node->right, height, offset + node->subtree_size,
                                                          target_height, target_layer);
                    },
                    [&]() {
                        TraverseFromTopAndCopyTargetLayer(node->down, height - 1, offset, target_height, target_layer);
                    }
            );
        } else if (go_right) {
            TraverseFromTopAndCopyTargetLayer(node->right, height, offset + node->subtree_size, target_height,
                                              target_layer);
        } else if (go_down) {
            TraverseFromTopAndCopyTargetLayer(node->down, height - 1, offset, target_height, target_layer);
        }
    }

    seq<node_t*> GetLayerAsSeq(size_t height) {
        util::Assert(height < GetHeight());
        size_t total = CalcSubtreeSizes(heads_.back(), GetHeight() - 1, height);
        util::Assert(total == lens_[height]);
        seq<node_t*> layer(lens_[height]);
        TraverseFromTopAndCopyTargetLayer(heads_.back(), GetHeight() - 1, 0, height, layer);
        return layer;
    }

    seq<node_t*> GetTopNthLayerAsSeq(size_t depth) {
        util::Assert(depth < GetHeight());
        return GetLayerAsSeq(GetHeight() - 1 - depth);
    }


    void Merge(seq<seq<node_t*>> layers) {
        util::Assert(!layers.empty() && parlay::none_of(layers, [](auto const &layer) { return layer.empty(); }));

        CoerceHeightAtLeast(layers.size());

        // TODO! but right now, sleep
    }

    inline size_t GetHeight() { return heads_.size(); }

    seq<node_t*> heads_; // each layer's left sentinel node, bottom to top;
    seq<node_t*> tails_; // ------------ right ---------------------------;
    seq<size_t>  lens_;  // each layer's length, including sentinels
};

}

#endif //PBSL_SKIP_LIST_HPP
