#ifndef TREE_H
#define TREE_H

#include <memory>
#include <iosfwd>
#include <functional>
#include <tuple>
#include <limits>
using namespace std::placeholders;

using Action = std::function<void()>;
using Order = std::function<void(Action,Action,Action)>;

template <typename T>
class Tree {
public:
    struct Node;
    using NodePtr = std::shared_ptr<Node const>;
    struct Node {
        Node (T const  value,  NodePtr const left, NodePtr const right) :
                m_value(value), m_left(std::move(left)), m_right(std::move(right)) {}
        Node() : m_value{ }, m_left{}, m_right{} {}
        Node(T const value) : m_value(value) {}
        T const  m_value;
        NodePtr const m_left;
        NodePtr const m_right;
    };

    NodePtr const m_root;

    Tree(NodePtr root) : m_root(std::move(root)) {}

    Tree() = default;

    Tree(Tree &&) = default;

    Tree(Tree const &) = default;

    Tree(T value, Tree left, Tree right);

    static Order inorder, preorder, postorder;

    Tree left() const {
        if (empty()) {
            throw std::logic_error("Tree::left(): Empty BST");
        }
        return Tree(m_root->m_left);
    }

    Tree right() const {
        if (empty()) {
            throw std::logic_error("Tree:right() Empty BST");
        }
        return Tree(m_root->m_right);
    }

    T value() const {
        if (empty()) {
            throw std::logic_error("Tree:right() Empty BST");
        }
        return m_root->m_value;
    }

    bool empty() const {
        return m_root == nullptr;
    }

    static Tree createEmptyNode() {
        return Tree();
    }

    static Tree createValueNode(T value) {
        auto ptr = std::make_shared<Node>(Node(std::move(value)));
        return Tree(ptr);
    }

    static Tree createValueNode(T value, Tree left, Tree right) {
        auto ptr = std::make_shared<Node>(Node(value, left.m_root, right.m_root));
        return Tree(ptr);
    }

    template<typename R>
    R fold(std::function<R(T,R,R)> operation, R init) const {
        if(empty())
        {
            return init;
        }
        auto value = m_root->m_value;
        auto l_result = left().fold(operation, init);
        auto r_result = right().fold(operation, init);
        return operation(value, l_result, r_result);
    }

    Tree<T> map(std::function<T(T)> transformer) const
    {
        if(empty())
            return Tree();
        auto mapped_value = transformer(m_root->m_value);
        return createValueNode(mapped_value, left().map(transformer), right().map(transformer));
    }

    T accumulate(std::function<T(T,T)> operation, T a, Order traversal) const
    {
        auto res = a;
        accumulate_helper(operation, res, traversal);
        return res;
    }

    void accumulate_helper(std::function<T(T,T)> operation, T& a, Order traversal) const
    {
        if (empty()) {
            return;
        }
        auto nodeFun = [&]{ a = operation(a, value()); };
        auto leftFun = [&]{ left().accumulate_helper(operation, a, traversal); };
        auto rightFun = [&]{ right().accumulate_helper(operation, a, traversal); };
        traversal(nodeFun,leftFun,rightFun);
    }

    void apply(std::function<void(T)> operation, Order traversal) const
    {
        if (empty()) {
            return;
        }
        auto nodeFun = [&]{operation(value());};
        auto leftFun = [&]{left().apply(operation, traversal);};
        auto rightFun = [&]{right().apply(operation, traversal);};
        traversal(nodeFun,leftFun,rightFun);
    }

    size_t height() const {
        auto fun = [](T v, size_t l, size_t r)->size_t{return std::max(l,r)+1;};
        return fold<size_t>(fun, 0);
    }

    size_t size() const {
        auto fun = [](T v, size_t l, size_t r)->size_t{return l+r+1;};
        return fold<size_t>(fun, 0);
    }

    bool is_bst() const {
        using resType = std::tuple<bool,T,T>;
        auto fun = [](T v, resType l, resType r)->resType
        {
            bool l_is_bst = std::get<0>(l);
            auto l_min = std::get<1>(l);
            auto l_max = std::get<2>(l);
            auto  r_is_bst = std::get<0>(r);
            auto r_min = std::get<1>(r);
            auto r_max = std::get<2>(r);
            auto is_bst =  l_is_bst && r_is_bst && l_max <= v && v <= r_min;
            return std::make_tuple(is_bst, l_min, r_max);
        };
        auto result = fold<resType>(fun, std::make_tuple(true,std::numeric_limits<T>::max(),std::numeric_limits<T>::min()));
        return std::get<1>(result);
    }
};

template<typename T>
Order Tree<T>::inorder = [](Action node, Action left, Action right)->void{
    left(); node(); right();
};

template<typename T>
Order Tree<T>::preorder = [](Action node, Action left, Action right)->void{
    node(); left(); right();
};

template<typename T>
Order Tree<T>::postorder = [](Action node, Action left, Action right)->void{
    node(); right(); left();
};

#endif /* TREE_H */