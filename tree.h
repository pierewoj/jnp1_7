#ifndef TREE_H
#define TREE_H

#include <memory>
#include <iostream>
#include <functional>
#include <limits>
#include <queue>
#include <cassert>

using Action = std::function<void()>;
using Order = std::function<void(Action, Action, Action)>;

template<typename T>
class Tree {
private:
    struct Node;
    using NodePtr = std::shared_ptr<Node>;
    NodePtr m_root;

    Tree left() {
        assert(!empty());
        return Tree(m_root->m_left);
    }

    Tree right() {
        assert(!empty());
        return Tree(m_root->m_right);
    }

    T value() {
        assert(!empty());
        return m_root->get_value();
    }

    bool empty() {
        return m_root == nullptr;
    }

    template<typename Functor>
    Tree<T> filter_helper(Functor predicate, bool can_be_rightmost, std::queue<Tree> &append_queue) {
        if (empty()) {
            if (append_queue.empty() || !can_be_rightmost) {
                return Tree();
            }
            auto first = append_queue.front();
            append_queue.pop();
            return first.filter_helper(predicate, true, append_queue);
        }

        if (predicate(value())) {
            auto left_filtered = left().filter_helper(predicate, false, append_queue);
            auto right_filtered = right().filter_helper(predicate, can_be_rightmost, append_queue);
            return createValueNode(value(), left_filtered, right_filtered);
        }

        if (left().empty() && !right().empty()) {
            return right().filter_helper(predicate, can_be_rightmost, append_queue);
        }

        if (!left().empty() && right().empty()) {
            return left().filter_helper(predicate, false, append_queue);
        }

        if (left().empty() && right().empty()) {
            return Tree();
        }

        append_queue.push(right());
        return left().filter_helper(predicate, can_be_rightmost, append_queue);
    }

    template<typename Functor>
    void accumulate_helper(Functor operation, T &a, Order traversal) {
        if (empty()) {
            return;
        }
        auto nodeFun = [&] { a = operation(a, value()); };
        auto leftFun = [&] { left().accumulate_helper(operation, a, traversal); };
        auto rightFun = [&] { right().accumulate_helper(operation, a, traversal); };
        traversal(nodeFun, leftFun, rightFun);
    }

    static T min(T a, T b, T c) {
        return std::min(std::min(a, b), c);
    }

    static T max(T a, T b, T c) {
        return std::max(std::max(a, b), c);
    }

public:
    Tree() = default;

    Tree(NodePtr root) : m_root(std::move(root)) {}

    Tree(const Tree &t) = default; //deep copy not implemented

    Tree(Tree &&) = default;

    static Order inorder, preorder, postorder;

    static Tree createEmptyNode() {
        return Tree();
    }

    template<typename TVal>
    static Tree createValueNode(TVal value) {
        auto ptr = std::make_shared<Node>(Node(std::move(value)));
        return Tree(ptr);
    }

    template<typename TVal>
    static Tree createValueNode(TVal val, Tree left, Tree right) {
        auto ptr = std::make_shared<Node>(Node(val, left.m_root, right.m_root));
        return Tree(ptr);
    }

    template<typename R, typename Functor>
    R fold(Functor operation, R init) {
        if (empty()) {
            return init;
        }
        auto val = value();
        auto l_result = left().fold(operation, init);
        auto r_result = right().fold(operation, init);
        return operation(val, l_result, r_result);
    }

    template<typename Functor>
    Tree<T> map(Functor transformer) {
        if (empty())
            return Tree();
        auto mapped_value = transformer(value());
        return createValueNode(mapped_value, left().map(transformer), right().map(transformer));
    }

    template<typename Functor>
    Tree<T> lazy_map(Functor transformer) {
        if (empty())
            return Tree();
        auto transformed_val = value();
        auto value_mapper = [=]() -> T { return transformer(transformed_val); };
        return createValueNode(value_mapper, left().lazy_map(transformer), right().lazy_map(transformer));
    }

    template<typename Functor>
    Tree<T> filter(Functor predicate) {
        std::queue<Tree<T>> to_append;
        return filter_helper(predicate, true, to_append);
    }

    template<typename Functor>
    T accumulate(Functor operation, T a, Order traversal) {
        auto res = a;
        accumulate_helper(operation, res, traversal);
        return res;
    }

    template<typename Functor>
    void apply(Functor operation, Order traversal) {
        if (empty()) {
            return;
        }
        auto nodeFun = [&] { operation(value()); };
        auto leftFun = [&] { left().apply(operation, traversal); };
        auto rightFun = [&] { right().apply(operation, traversal); };
        traversal(nodeFun, leftFun, rightFun);
    }

    size_t height() {
        auto fun = [](T v, size_t l, size_t r) -> size_t { return std::max(l, r) + 1; };
        return fold<size_t>(fun, 0);
    }

    size_t size() {
        auto fun = [](T v, size_t l, size_t r) -> size_t { return l + r + 1; };
        return fold<size_t>(fun, 0);
    }

    bool is_bst() {
        using BstAcc = struct BstAcc {
            BstAcc(T min, T max, bool is_bst) : min(min), max(max), is_bst(is_bst) {};
            T min;
            T max;
            bool is_bst;
        };
        auto fun = [](T v, BstAcc l, BstAcc r) -> BstAcc {
            auto is_bst = l.is_bst && r.is_bst && l.max <= v && v <= r.min;
            return BstAcc(min(l.min, r.min, v), max(l.max, r.max, v), is_bst);
        };
        auto result = fold(fun, BstAcc(std::numeric_limits<T>::max(),
                                       std::numeric_limits<T>::min(), true));
        return result.is_bst;
    }

    void print(Order traversal = inorder) {
        apply([](int e) { std::cout << e << " "; }, traversal);
        std::cout << std::endl;
    }
};

template<typename T>
Order Tree<T>::inorder = [](Action node, Action left, Action right) -> void {
    left();
    node();
    right();
};

template<typename T>
Order Tree<T>::preorder = [](Action node, Action left, Action right) -> void {
    node();
    left();
    right();
};

template<typename T>
Order Tree<T>::postorder = [](Action node, Action left, Action right) -> void {
    left();
    right();
    node();
};

template<typename T>
struct Tree<T>::Node {
    NodePtr m_left;
    NodePtr m_right;

    Node(T value, NodePtr left, NodePtr right) :
            m_left(std::move(left)),
            m_right(std::move(right)),
            m_value_created{},
            m_value_creator{},
            m_value(value) {}

    Node(std::function<T()> value_creator, NodePtr left, NodePtr right) :
            m_left(std::move(left)),
            m_right(std::move(right)),
            m_value_created{},
            m_value_creator{value_creator},
            m_value{} {}

    Node() :
            m_left(),
            m_right(),
            m_value_created{},
            m_value_creator{},
            m_value{} {}

    Node(T value) :

            m_left(),
            m_right(),
            m_value_created{},
            m_value_creator{},
            m_value{value} {}

    T get_value() {
        if (m_value_creator != nullptr && !m_value_created) {
            m_value = m_value_creator();
            m_value_created = true;
        }
        return m_value;
    }

private:
    bool m_value_created = false;
    std::function<T()> m_value_creator;
    T m_value;

};

#endif /* TREE_H */