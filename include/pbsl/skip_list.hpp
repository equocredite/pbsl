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
#include "common/util.hpp"

namespace pbsl {

class SkipList {
  public:
    using Key = size_t;
    template<typename T> using Seq = util::types::Seq<T>;
    using NodeAllocator = parlay::type_allocator<Node>;

    static auto FromOrderedKeys(Seq<Key> const& keys) -> SkipList {
        assert(!keys.empty());
        auto nodes = CreateNodes(keys, true).first;
        return {nodes.front(), nodes.back()};
    }

    ~SkipList() {
        if (Height() == 0) {
            NodeAllocator::destroy(left_sentinel_);
            NodeAllocator::destroy(right_sentinel_);
            return;
        }
        for (Node* node = left_sentinel_; node != nullptr;) {
            Node* next = node->Next(0);
            NodeAllocator::destroy(node);
            node = next;
        }
    }

    auto InsertOrdered(Seq<Key> const& keys) -> void {
        assert(!keys.empty());
        auto [nodes, height] = CreateNodes(keys, false);
        //std::cout << "other height = " << height << std::endl;
        Merge(nodes, height);
    }

//    auto PrettyPrint(std::ostream& out) const -> void {
//        size_t n = 0;
//        size_t cell_len = 2;
//        for (Node* node = left_sentinel_; node != nullptr; node = node->Next(0)) {
//            ++n;
//            if (node == left_sentinel_ || node == right_sentinel_) continue;
//            cell_len = std::max(cell_len, std::to_string(node->key).size());
//        }
//        std::vector<std::string> lines;
//        std::unordered_map<Key, size_t> offset_by_key;
//        for (size_t level = 0; level < Height(); ++level) {
//            size_t prev_offset = 0;
//            for (Node* node = left_sentinel_; node != nullptr; node = node->Next(level)) {
//                size_t offset = offset_by_key.try_emplace(node->key, prev_offset + cell_len + 3).first->first;
//                for (size_t i = )
//            }
//        }
//
//        for (size_t level = 0; level < Height(); ++level) {
//
//        }

    auto Height() const -> size_t { return left_sentinel_->Height(); }

    auto IsEmpty() const -> bool { return Height() == 0 || left_sentinel_->Next(0) == right_sentinel_; }

    // debug
    auto DebugGetNodes(size_t level = 0) const -> std::vector<Node*> {
        std::vector<Node*> nodes;
        for (Node* node = left_sentinel_; node != nullptr; node = node->Next(level)) {
            nodes.push_back(node);
        }
        return nodes;
    }

  //private:
    SkipList(Node* left_sentinel, Node* right_sentinel)
        : left_sentinel_(left_sentinel)
        , right_sentinel_(right_sentinel) {
        assert(left_sentinel != nullptr && right_sentinel != nullptr);
    }

    static auto CreateNode(Key key) -> Node* { return NodeAllocator::create(key, GenerateHeight()); }

    static auto CreateNodes(Seq<Key> const& keys, bool sentinelled = false) -> std::pair<Seq<Node*>, size_t> {
        auto nodes = parlay::map(keys, CreateNode);
        size_t height = (*parlay::max_element(nodes, [](auto lhs, auto rhs) {
            return lhs->Height() < rhs->Height();
        }))->Height();
        if (sentinelled) {
            auto [left_sentinel, right_sentinel] = CreateSentinels(height);
            // TODO: create array of n + 2 nodes right away (with sentinels at both ends),
            //  then adjust the sentinels' heights
            nodes.insert(nodes.begin(), left_sentinel);
            nodes.push_back(right_sentinel);
        }
        auto layer = nodes;
        for (size_t level = 0; level < height;) {
            // TODO: FillLinks after this loop for all levels in parallel?
            FillLinks(layer, level++);
            layer = FilterNodesHigherThan(layer, level);
        }
        return {nodes, height};
    }

    static auto CreateSentinels(size_t height) -> std::pair<Node*, Node*> {
        Node* left_sentinel = NodeAllocator::create(util::Constants::MIN_KEY, height);
        Node* right_sentinel = NodeAllocator::create(util::Constants::MAX_KEY, height);
        return {left_sentinel, right_sentinel};
    }

    auto CoerceHeightAtLeast(size_t min_height) -> void {
        if (Height() >= min_height) return;
        left_sentinel_->Resize(min_height, right_sentinel_);
        right_sentinel_->Resize(min_height, nullptr);
    }

    auto CountDescendantsAtLevelImpl(Node* node, size_t level, size_t target_level) -> size_t {
        assert(node != nullptr);
        assert(level >= target_level);
        Node* right = node->Next(level);
        bool go_right = right != nullptr && right->Height() <= level + 1;
        bool go_down = level > target_level;
        node->subtree_size[level] = static_cast<size_t>(!go_down);
        size_t right_size = 0;
        size_t down_size = 0;
        if (go_right && go_down) {
            parlay::par_do(
                    [&]() { right_size = CountDescendantsAtLevelImpl(right, level, target_level); },
                    [&]() { down_size = CountDescendantsAtLevelImpl(node, level - 1, target_level); }
            );
        } else if (go_right) {
            right_size = CountDescendantsAtLevelImpl(right, level, target_level);
        } else if (go_down) {
            down_size = CountDescendantsAtLevelImpl(node, level - 1, target_level);
        }
        return (node->subtree_size[level] += right_size + down_size);
    }

    auto CountDescendantsAtLevel(size_t level) -> size_t {
        return CountDescendantsAtLevelImpl(left_sentinel_, Height() - 1, level);
    }

    auto CopyLayerImpl(Node* node, size_t level, size_t offset, size_t target_level, Seq<Node*>& target_layer) const -> void {
        assert(node != nullptr);
        assert(level >= target_level);
        // if v->down_link_ == node->right_link_ for some node v, then node->right_link_ will be reached from v
        Node* right = node->Next(level);
        bool go_right = right != nullptr && right->Height() <= level + 1;
        bool go_down = level > target_level;
        if (!go_down) target_layer[offset] = node;
        if (go_right && go_down) {
            parlay::par_do(
                    [&]() {
                        CopyLayerImpl(right, level, offset + node->subtree_size[level] - right->subtree_size[level],
                                      target_level, target_layer);
                    },
                    [&]() {
                        CopyLayerImpl(node, level - 1, offset, target_level, target_layer);
                    }
            );
        } else if (go_right) {
            CopyLayerImpl(right, level, offset + node->subtree_size[level] - right->subtree_size[level], target_level,
                          target_layer);
        } else if (go_down) {
            CopyLayerImpl(node, level - 1, offset, target_level, target_layer);
        }
    }

    auto CopyLayer(size_t level, Seq<Node*>& target_layer) const -> void {
        CopyLayerImpl(left_sentinel_, Height() - 1, 0, level, target_layer);
    }

    auto GetLayer(size_t level) -> Seq<Node*> {
        //return GetLayerWithLinearSpan(level);
        assert(level < Height());
        size_t total = CountDescendantsAtLevel(level);
        Seq<Node*> layer(total);
        CopyLayer(level, layer);
        return layer;
    }

    // debug
    auto GetLayerWithLinearSpan(size_t level) const -> Seq<Node*> {
        Seq<Node*> nodes;
        for (Node* node = left_sentinel_; node != nullptr; node = node->Next(level)) {
            nodes.push_back(node);
        }
        return nodes;
    }

    auto Merge(Seq<Node*>& nodes, size_t height) -> void {
        //std::cout << "!1" << std::endl;
        CoerceHeightAtLeast(height);
        //std::cout << "!2" << std::endl;
        size_t crit_level = Height() - height;
        //std::cout << "!3" << std::endl;
        auto crit_layer = GetLayer(crit_level);
        //std::cout << "!4" << std::endl;
        MergeHigherLevels(crit_layer, nodes, crit_level);
        //std::cout << "!5" << std::endl;
        MergeLowerLevels(crit_layer, nodes, crit_level);
        //std::cout << "!6" << std::endl;
    }

    auto MergeHigherLevels(Seq<Node*> left, Seq<Node*> right, size_t crit_level) -> void {
        for (size_t level = crit_level + 1; level < Height(); ++level) {
            left = FilterNodesHigherThan(left, level);
            right = FilterNodesHigherThan(right, level);
            if (right.empty()) break;
            MergeLayer(left, right, level);
        }
    }

    static auto FindStartingNodesInCritLayer(Seq<Node*>& crit_layer, Seq<Node*>& nodes) -> Seq<Node*> {
        auto old_nodes = parlay::map(crit_layer, [&](Node const* node) { return std::make_pair(node, false); });
        auto new_nodes = parlay::map(nodes, [&](Node const* node) { return std::make_pair(node, true); });
        auto merged = parlay::merge(old_nodes, new_nodes, [&](auto const& lhs, auto const& rhs) {
            return lhs.first->key < rhs.first->key;
        });
        auto is_new = parlay::map(merged, [&](auto const& x) { return x.second; });
        auto sums = parlay::map(merged, [&](auto const& x) { return static_cast<size_t>(!x.second); });
        parlay::scan_inplace(sums);
        auto indices = parlay::pack(sums, is_new);
        return parlay::map(indices, [&](size_t i) { return crit_layer[i - 1]; });
    }

    auto PrepareInsert(Node* node, size_t level, Node* new_node) -> void {
        while (true) {
            if (new_node->Height() > level) {
                if (node->key >= new_node->prev_key[level]) {
                    new_node->new_prev[level] = node;
                }
                if (new_node->Next(level) == nullptr || new_node->Next(level)->key > node->Next(level)->key) {
                    new_node->new_next[level] = node->Next(level);
                }
            }
            if (level == 0) break;
            --level;
            while (node->Next(level)->key < new_node->key) node = node->Next(level);
        }
    }

    auto MergeLowerLevels(Seq<Node*>& crit_layer, Seq<Node*>& nodes, size_t crit_level) -> void {
        //std::cout << "!7" << std::endl;
        auto starting_nodes = FindStartingNodesInCritLayer(crit_layer, nodes);
        //std::cout << "!8" << std::endl;
        parlay::parallel_for(0, nodes.size(), [&](size_t i) { PrepareInsert(starting_nodes[i], crit_level, nodes[i]); });
        //std::cout << "!9" << std::endl;
        parlay::parallel_for(0, nodes.size(), [&](size_t i) {
            auto node = nodes[i];
            parlay::parallel_for(0, node->Height(), [&](size_t level) {
               if (node->new_prev[level] != nullptr) {
                   node->new_prev[level]->next[level] = node;
                   node->new_prev[level] = nullptr;
               }
               if (node->new_next[level] != nullptr) {
                   node->next[level] = node->new_next[level];
                   node->new_next[level] = nullptr;
               }
            });
        });
    }

    static auto MergeLayer(Seq<Node*>& left, Seq<Node*>& right, size_t level) -> void {
        auto order = parlay::merge(left, right, [&](Node const* lhs, Node* const rhs) {
            return lhs->key < rhs->key;
        });
        FillLinks(order, level);
    }

    static auto FillLinks(Seq<Node*> nodes, size_t level) -> void {
        assert(!nodes.empty());
        parlay::parallel_for(0, nodes.size() - 1, [&](size_t i) {
            nodes[i]->next[level] = nodes[i + 1];
            nodes[i + 1]->prev_key[level] = nodes[i]->key;
        });
    }

    static auto FilterNodesHigherThan(Seq<Node*> const& nodes, size_t height) -> Seq<Node*> {
        return parlay::filter(nodes, [&](Node* const node) { return node->Height() > height; });
    }

    static auto GenerateHeight() -> size_t { return util::random::NextGeometric() + 1; }

    Node* const left_sentinel_;
    Node* const right_sentinel_;
};

}

#endif //PBSL_SKIP_LIST_HPP
