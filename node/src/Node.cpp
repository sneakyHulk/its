#include "Node.h"

#include <iostream>

/**
 * @brief Outputs the name of the current node type.
 */
void Node::name() { std::cout << typeid(*this).name() << std::endl; }
