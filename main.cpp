#include <iostream>
#include <functional>
#include <sstream>
#include <string>
#include "tree.h"

int main(int argc, const char * argv[]) {
    auto two = Tree<int>::createValueNode(2);
    auto one = Tree<int>::createValueNode(1, Tree<int>::createEmptyNode(), two);
    //auto four = Tree<int>::createValueNode(4);
    auto five = Tree<int>::createValueNode(5,
                                           Tree<int>::createValueNode(4),
                                           Tree<int>::createEmptyNode());
    auto root = Tree<int>::createValueNode(3, one, five);
    Tree<int> tree(root);
    std::cout<<tree.m_root->m_value<<" "<<tree.height()<<" "<<tree.size()<<" "<<tree.is_bst()<<"\n";
    int x = 2;
    Tree<int> minus_x = tree.map([&](int e)->int{return e - x;});
    std::cout<<minus_x.m_root->m_value<<" "<<tree.height()<<" "<<tree.size()<<" "<<tree.is_bst()<<"\n";

    int sum1 = tree.accumulate(std::plus<int>(), 0, Tree<int>::inorder);
    std::cout<<sum1<<std::endl;
    int sum2 = tree.accumulate(std::plus<int>(), 0, Tree<int>::preorder);
    std::cout<<sum2<<std::endl;
    int sum3 = tree.accumulate(std::plus<int>(), 0, Tree<int>::postorder);
    std::cout<<sum3<<std::endl;

    tree.apply([](int e){std::cout << e << ":";}, Tree<int>::inorder); std::cout<<std::endl;
    tree.apply([](int e){std::cout << e << ":";}, Tree<int>::preorder); std::cout<<std::endl;
    tree.apply([](int e){std::cout << e << ":";}, Tree<int>::postorder); std::cout<<std::endl;
}