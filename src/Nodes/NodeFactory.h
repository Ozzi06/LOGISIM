#pragma once
#include "Node.h"

struct NodeFactory {
    static Node* createNode(std::vector<Node*>* container, const std::string& nodeName);
};