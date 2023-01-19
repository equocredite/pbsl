#ifndef PBSL_NODE_HPP
#define PBSL_NODE_HPP

#include <parlay/alloc.h>

#include "util.hpp"

namespace pbsl {

template<typename K>
struct Node {
    K key;
    Node* right = nullptr;
    bool has_parent = false;

    // auxiliary temporary fields

    size_t subtree_size{};

    static Node* New(K key, Node* right = nullptr, Node* down = nullptr) {
        return node_allocator::create(key, right, down);
    }

    static Node* NewLeftSentinel(Node* right = nullptr, Node* down = nullptr) {
        return New(MINUS_INFTY, right, down);
    }

    static Node* NewRightSentinel(Node* down = nullptr) {
        return Node::New(PLUS_INFTY, nullptr, down);
    }

    void SetDown(Node* that) {
        down = that;
        if (that != nullptr) has_parent = true;
    }

    bool IsSentinel() { // TODO: store an explicit flag?
        return key == PLUS_INFTY || key == MINUS_INFTY;
    }

    // expected complexity should be O(1), not sure about reality;
    // normally, we shouldn't need to call this function for the last node in a layer;
    //
    // TODO: for each layer in skip list, store a flag indicating whether the layer has been changed,
    //  and if not, use cached distance
//    size_t CalcDistanceToNearestNodeWithParent() {
//        util::Assert(right != nullptr);
//        size_t dist = 1;
//        auto node = right;
//        while (!node->has_parent) {
//            ++dist;
//            node = right;
//        }
//        return dist;
//    }

  private:
    using node_allocator = parlay::type_allocator<Node>;

    static constexpr K PLUS_INFTY  = std::numeric_limits<K>::max();
    static constexpr K MINUS_INFTY = std::numeric_limits<K>::min();

    Node* down = nullptr;
};

}

#endif //PBSL_NODE_HPP
