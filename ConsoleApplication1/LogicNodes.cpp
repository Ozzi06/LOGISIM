#include "LogicNodes.h"

std::string toString(NodeType type)
{
    switch (type) {
    case NodeType::GateAND: return "GateAND";
    case NodeType::GateOR: return "GateOR";
    case NodeType::GateNAND: return "GateNAND";
    case NodeType::GateNOR: return "GateNOR";
    case NodeType::GateXOR: return "GateXOR";
    case NodeType::GateXNOR: return "GateXNOR";
    case NodeType::GateBUFFER: return "GateBUFFER";
    case NodeType::GateNOT: return "GateNOT";
    case NodeType::PushButton: return "PushButton";
    case NodeType::ToggleButton: return "ToggleButton";
    case NodeType::StaticToggleButton: return "StaticToggleButton";
    case NodeType::LightBulb: return "LightBulb";
    case NodeType::SevenSegmentDisplay: return "SevenSegmentDisplay";
    case NodeType::FunctionNode: return "FunctionNode";
    case NodeType::Bus: return "Bus";
    case NodeType::RootNode: return "RootNode";
    default: return "Unknown NodeType";
    }
}
