#include "NodeFactory.h"
#include "BinaryGates.h"
#include "UnaryGates.h"
#include "Buttons.h"
#include "Displays.h"
#include "Bus.h"
#include "FunctionNode.h"

Node* NodeFactory::createNode(std::vector<Node*>* container, const std::string& nodeName) {
    static std::unordered_map<std::string, std::function<Node* (std::vector<Node*>* container)>> factoryMap = {
        { "GateAND", [](std::vector<Node*>* container) -> Node* { return new GateAND(container); } },
        { "GateOR", [](std::vector<Node*>* container) -> Node* { return new GateOR(container); } },
        { "GateNAND", [](std::vector<Node*>* container) -> Node* { return new GateNAND(container); } },
        { "GateNOR", [](std::vector<Node*>* container) -> Node* { return new GateNOR(container); } },
        { "GateXOR", [](std::vector<Node*>* container) -> Node* { return new GateXOR(container); } },
        { "GateXNOR", [](std::vector<Node*>* container) -> Node* { return new GateXNOR(container); } },
        { "GateBUFFER", [](std::vector<Node*>* container) -> Node* { return new GateBUFFER(container); } },
        { "GateNOT", [](std::vector<Node*>* container) -> Node* { return new GateNOT(container); } },
        { "PushButton", [](std::vector<Node*>* container) -> Node* { return new PushButton(container); } },
        { "ToggleButton", [](std::vector<Node*>* container) -> Node* { return new ToggleButton(container); } },
        { "StaticToggleButton", [](std::vector<Node*>* container) -> Node* { return new StaticToggleButton(container); } },
        { "LightBulb", [](std::vector<Node*>* container) -> Node* { return new LightBulb(container); } },
        { "SevenSegmentDisplay", [](std::vector<Node*>* container) -> Node* { return new SevenSegmentDisplay(container); } },
        { "FunctionNode", [](std::vector<Node*>* container) -> Node* { return new FunctionNode(container); } },
        { "Bus", [](std::vector<Node*>* container) -> Node* { return new Bus(container); } },
    };

    auto it = factoryMap.find(nodeName);
    if (it != factoryMap.end()) {
        return it->second(container); // Instantiate the node
    }


    return nullptr; // Return nullptr if nodeName is not found
}
