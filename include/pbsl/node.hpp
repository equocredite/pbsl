#ifndef PBSL_NODE_HPP
#define PBSL_NODE_HPP

#include <parlay/alloc.h>

#include "util.hpp"
#include "common/debug.hpp"

namespace pbsl {

struct Node {
    using K = config::Key;
    template<typename T> using Seq = util::types::Seq<T>;

    // -----------------

    K const key;
    Seq<Node*> next;

    // auxiliary -------

    Seq<size_t> subtree_size;
    Seq<K> prev_key;
    Seq<Node*> new_prev;
    Seq<Node*> new_next;

    // -----------------

    Node(K key, size_t height)
            : key(key)
            , next(height, nullptr)
            , subtree_size(height, 0)
            , prev_key(height, 0)
            , new_prev(height, nullptr)
            , new_next(height, nullptr)
            {
        assert(height > 0);
    }

    auto Height() const -> size_t { return next.size(); }

    auto Next(size_t level) const -> Node* {
        assert(level < Height());
        return next[level];
    }

    // TODO: store an explicit flag?
    auto IsSentinel() const -> bool { return IsLeftSentinel() || IsRightSentinel(); }

    auto IsLeftSentinel() const -> bool { return key == util::Constants::MIN_KEY; }

    auto IsRightSentinel() const -> bool { return key == util::Constants::MAX_KEY; }

    auto Resize(size_t height, Node* right) -> void {
        next.resize(height, right);
        subtree_size.resize(height, 0);
    }

//    auto TraverseRightAndGetLastNode() const -> Node* {
//        return right_link_ == nullptr ? this : right_link_->TraverseRightAndGetLastNode();
//    }

    // expected complexity should be O(1), not sure about reality;
    // normally, we shouldn't need to call this function for the last node in a layer;
    //
    // TODO: for each layer in skip list, store a flag indicating whether the layer has been changed,
    //  and if not, use cached distance
//    size_t CalcDistanceToNearestNodeWithParent() {
//        assert(right_link_ != nullptr);
//        size_t dist = 1;
//        auto node = right_link_;
//        while (!node->has_parent_) {
//            ++dist;
//            node = right_link_;
//        }
//        return dist;
//    }
};

}

#endif //PBSL_NODE_HPP
