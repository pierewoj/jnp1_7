#ifndef TREE_H
#define TREE_H

#include <memory>
#include <iosfwd>
#include <functional>
#include <tuple>
#include <limits>
#include <queue>
#include <cassert>

using namespace std::placeholders;

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
    Tree<T> filter_helper(Functor predicate, bool can_be_rightmost, std::queue<Tree> &toAppend) {
        auto filter_left = [&]() -> Tree<T> { return left().filter_helper(predicate, false, toAppend); };
        auto filter_right = [&]() -> Tree<T> { return right().filter_helper(predicate, can_be_rightmost, toAppend); };

        if (empty()) {
            if (toAppend.empty() || !can_be_rightmost) {
                return Tree();
            }
            auto first = toAppend.front();
            toAppend.pop();
            return first.filter_helper(predicate, true, toAppend);
        }

        if (predicate(value())) {
            auto left_filtered = filter_left();
            auto right_filtered = filter_right();
            return createValueNode(value(), left_filtered, right_filtered);
        }

        if (left().empty() && !right().empty()) {
            return filter_right();
        }

        if (!left().empty() && right().empty()) {
            return filter_left();
        }

        if (left().empty() && right().empty()) {
            return Tree();
        }

        toAppend.push(right());
        return left().filter_helper(predicate, can_be_rightmost, toAppend);
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
public:
    Tree(NodePtr root) : m_root(std::move(root)) {}

    Tree() = default;

    Tree(Tree &&) = default;

    Tree(Tree const &) = default;

    Tree(T value, Tree left, Tree right);

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
        using resType = std::tuple<bool, T, T>;
        auto fun = [](T v, resType l, resType r) -> resType {
            bool l_is_bst = std::get<0>(l);
            auto l_min = std::get<1>(l);
            auto l_max = std::get<2>(l);
            auto r_is_bst = std::get<0>(r);
            auto r_min = std::get<1>(r);
            auto r_max = std::get<2>(r);
            auto is_bst = l_is_bst && r_is_bst && l_max <= v && v <= r_min;
            return std::make_tuple(is_bst, l_min, r_max);
        };
        auto result = fold<resType>(fun, std::make_tuple(true, std::numeric_limits<T>::max(),
                                                         std::numeric_limits<T>::min()));
        return std::get<1>(result);
    }

    bool print(Order traversal = inorder) {
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
struct Tree<T>:: Node {
    NodePtr m_left;
    NodePtr m_right;

    Node(T value, NodePtr left, NodePtr right) :
            m_value(value),
            m_left(std::move(left)),
            m_right(std::move(right)) {}

    Node(std::function<T()> value_creator, NodePtr left, NodePtr right) :
            m_value_creator(value_creator),
            m_left(std::move(left)),
            m_right(std::move(right)) {}

    Node() : m_left{}, m_right{} {}

    Node(T value) : m_value(value) {}

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