#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <fstream>
#include <iostream>
#include <iomanip> // for std::hex and std::setw
#include <cassert>

struct Node;

enum class NodeType : uint32_t {
    GateAND,
    GateOR,
    GateNAND,
    GateNOR,
    GateXOR,
    GateXNOR,
    GateBUFFER,
    GateNOT,
    PushButton,
    ToggleButton,
    StaticToggleButton,
    LightBulb,
    SevenSegmentDisplay,
    FunctionNode,
    Bus,
    RootNode
};

std::string toString(NodeType type);

typedef uint32_t offset;

union connector_target {
    offset target_offset;

    struct connector_location_index {
        uint16_t node_pos;
        uint16_t connector_index;
    } location;

    connector_target(offset offset) : target_offset(offset) {}
    connector_target(uint16_t node_pos, uint16_t connector_index) {
        location.node_pos = node_pos;
        location.connector_index = connector_index;
    }
};
static_assert(sizeof(connector_target) == sizeof(offset));

typedef connector_target input;
typedef bool output;

struct NodeHeader {
	NodeType type;
	uint32_t total_size;
};

struct BinaryGateHeader : public NodeHeader {
    uint16_t input_count;
    bool output;
    bool new_output;
    //input_count * input
};
struct UnaryGateHeader : public NodeHeader {
    uint16_t input_output_count;
    //input_output_count * input
    //input_output_count * output //outputs
    //input_output_count * output //new_outputs
};
struct FunctionNodeHeader : public NodeHeader {
    uint16_t input_targ_node_count; //not number of connections but nodes
    uint16_t output_targ_node_count; //not number of connections but nodes
    uint16_t input_count;
    uint16_t output_count;
    uint16_t child_count;
    bool has_changed;
    //input_targ_count * offset
    //output_targ_node_count * offset
    //input_count * input
    //output_count * ouput //outputs
    //children
};

struct BusNodeHeader : public NodeHeader {
    uint16_t input_output_count;
    offset shared_outputs_offset;
    offset shared_new_outputs_offset;
    //input_count * input
    //if first with name:
    //shared_outputs
    //shared_new_outputs
};
struct InputNodeHeader : public NodeHeader {
    uint16_t output_count;
    //output_count * ouput
};
struct OutputNodeHeader : public NodeHeader {
    uint16_t input_count;
    //input_count * input
};
struct GenericNodeHeader : public NodeHeader {
    uint16_t input_count;
    uint16_t output_count;
    //input_count * input
    //output_count * ouput //outputs
    //output_count * ouput // new_outputs
};
struct RootNodeHeader : public NodeHeader {
    uint16_t child_count;
    //children
};