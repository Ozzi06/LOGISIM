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
    BusNode,
    RootFunctionNode
};

std::string toString(NodeType type);

bool isInputType(NodeType type);

bool isOutputType(NodeType type);

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
    offset inputs_offset;
    //input_count * input
};
struct UnaryGateHeader : public NodeHeader {
    uint16_t input_output_count;
    offset inputs_offset;           // relative to start of node
    offset outputs_offset;          // relative to start of node
    offset new_outputs_offset;      // relative to start of node
    //input_output_count * input    // relative to container
    //input_output_count * output   //outputs
    //input_output_count * output   //new_outputs
};

struct BusNodeHeader : public NodeHeader {
    uint16_t input_output_count;
    uint16_t shared_output_count;
    offset inputs_offset;               // relative to node
    offset shared_outputs_offset;       // relative to container
    offset shared_new_outputs_offset;   // relative to container
    bool is_first_node_in_bus;
    //input_count * input               // relative to container
    //if first with name:
    //shared_outputs
    //shared_new_outputs
};

struct InputNodeHeader : public NodeHeader {
    uint16_t output_count;
    offset outputs_offset;
    //output_count * output
};
struct OutputNodeHeader : public NodeHeader {
    uint16_t input_count;
    offset inputs_offset;
    //input_count * input
};

struct FunctionNodeHeader : public NodeHeader {
    uint16_t input_targ_node_count; //not number of connections but nodes
    uint16_t output_targ_node_count; //not number of connections but nodes
    uint16_t input_count;
    uint16_t output_count;
    uint16_t child_count;
    bool has_changed;
    offset intargs_offset;              // relative to start of node
    offset outtargs_offset;             // relative to start of node
    offset inputs_offset;               // relative to start of node
    offset outputs_offset;              // relative to start of node
    offset children_offset;             // relative to start of node
    //input_targ_count * offset         // relative to start of node
    //output_targ_node_count * offset   // relative to start of node
    //input_count * input               // relative to container
    //output_count * output
    //children
};

// contains all the info to create a function node out of it
/*
struct RootNodeHeader : public NodeHeader {
    uint16_t input_targ_node_count; //not number of connections but nodes
    uint16_t output_targ_node_count; //not number of connections but nodes
    uint16_t input_count;
    uint16_t output_count;

    uint16_t child_count;

    offset intargs_offset;              // relative to start of node
    offset outtargs_offset;             // relative to start of node

    offset children_offset;

    //input_targ_count * offset         // relative to start of node
    //output_targ_node_count * offset   // relative to start of node
    //children
};*/
//auto i = sizeof(RootNodeHeader);