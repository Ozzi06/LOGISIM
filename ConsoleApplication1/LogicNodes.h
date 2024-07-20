#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include <fstream>
#include "main_game.h"
#include "vector_tools.h"

struct NodeHeader {
	NodeType type;
    uint32_t output_count;
	uint32_t input_count;
	uint32_t child_count;
	uint32_t total_size;
};
/*
Node layout:

NodeHeader
input_count * inputs
output_count * outputs
output_count * new_outputs

node_data, depending on type and may be varible size

child_count * children
*/
union input {
    uint32_t target_offset;

    struct output_location_index {
        uint16_t node_pos;
        uint16_t connector_index;
    } location;

    input(uint32_t offset) : target_offset(offset) {}
    input(uint16_t node_pos, uint16_t connector_index) {
        location.node_pos = node_pos;
        location.connector_index = connector_index;
    }
};
static_assert(sizeof(input) == sizeof(uint32_t), "input size mismatch");

struct output {
    bool value;
};
struct target {
    uint32_t target_offset;
};

class LogicBlock {
public:
    LogicBlock() : data(nullptr), size(0) {}

    void allocate(size_t byte_size) {
        data = std::make_unique<uint8_t[]>(byte_size);
        size = byte_size;
    }

    template<typename T>
    T* get(size_t offset) {
        return reinterpret_cast<T*>(data.get() + offset);
    }

    const NodeHeader* get_header() const {
        return reinterpret_cast<const NodeHeader*>(data.get());
    }

    void save_to_file(const std::string& filename) const {
        std::ofstream file(filename, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.get()), size);
    }

    static LogicBlock load_from_file(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary | std::ios::ate); // opens as binary and puts cursour at end
        std::streamsize file_size = file.tellg(); // gets cursor location at end to get size
        file.seekg(0, std::ios::beg);

        LogicBlock block;
        block.allocate(file_size);
        file.read(reinterpret_cast<char*>(block.data.get()), file_size);

        return block;
    }

    size_t get_size() const { return size; }

private:
    std::unique_ptr<uint8_t[]> data;
    size_t size;
};

class LogicBlockBuilder {
public:
    LogicBlockBuilder() : current_offset(0) {}

    size_t add_node(const Node& node) {
        size_t node_offset = current_offset;
        size_t header_offset = current_offset;

        // Reserve space for header
        auto header = add<NodeHeader>();

        // Fill in header
        header->type = node.get_type();
        header->output_count = node.outputs.size();
        header->input_count = node.inputs.size();
        if (node.get_children()) header->child_count = (*node.get_children()).size();
        else header->child_count = 0;

        size_t inputs_offset = current_offset;

        // add inputs
        for (const Input_connector& inconn : node.inputs) {
            int pos = get_position<Node>(*node.get_container(), inconn.target->host);
            assert(pos >= 0);
            assert(pos <= UINT16_MAX);
            add<input>(input(static_cast<uint16_t>(pos), inconn.target->index)); // is converted later
        }

        size_t outputs_offset = current_offset;

        // add outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        // Add node data
        switch (node.get_type()) 
        {
        case NodeType::FunctionNode: {
            const FunctionNode& funnode = static_cast<const FunctionNode&>(node);

            // add input targets to nodedata
            size_t intargs_offset = current_offset;
            for (Node* intarg : funnode.input_targs) {
                int pos = get_position<Node>(*funnode.get_children(), intarg);
                assert(pos >= 0);
                add<target>({ uint32_t(pos) }); // is converted later
            }
            assert(((current_offset - intargs_offset) / sizeof(target) == reinterpret_cast<NodeHeader*>(&buffer[header_offset])->input_count), 
                "the size of inputs should be the same as intargs");

            // add output targets to nodedata
            size_t outtargs_offset = current_offset;
            for (Node* outtarg : funnode.output_targs) {
                int pos = get_position<Node>(*funnode.get_children(), outtarg);
                assert(pos >= 0);
                add<target>({ uint32_t(pos) }); // is converted later
            }
            assert(((current_offset - outtargs_offset) / sizeof(target) == reinterpret_cast<NodeHeader*>(&buffer[header_offset])->output_count), 
                "the size of outputs should be the same as outtargs");

            
            // Add child nodes recursively
            size_t children_offset = current_offset;

            for (const Node* child : *funnode.get_children()) {
                add_node(*child);
            }

            // Convert targets into offsets
            size_t input_count = node.inputs.size();
            size_t output_count = node.outputs.size();
            target* intargs = reinterpret_cast<target*>(&buffer[intargs_offset]);
            target* outtargs = reinterpret_cast<target*>(&buffer[outtargs_offset]);

            size_t converted_inputs = 0;
            size_t converted_outputs = 0;
            uint32_t curr_child_offset = children_offset;

            // Iterate over each child of the funnode
            for (size_t i = 0; i < (*funnode.get_children()).size(); i++) {
                // Update input target offsets
                for (size_t j = converted_inputs; j < input_count; j++) {
                    if ((intargs + j)->target_offset == i) {
                        ++converted_inputs;
                        (intargs + j)->target_offset = curr_child_offset;
                    }
                }

                // Update output target offsets
                for (size_t j = converted_outputs; j < output_count; j++) {
                    if ((outtargs + j)->target_offset == i) {
                        ++converted_outputs;
                        (outtargs + j)->target_offset = curr_child_offset;
                    }
                }


                // Connect the inputs of the child nodes by iterating all children, jumping to the correct output location
                NodeHeader* curr_child_header = reinterpret_cast<NodeHeader*>(&buffer[curr_child_offset]);

                input* curr_child_inputs = reinterpret_cast<input*>(&buffer[
                    curr_child_offset + sizeof(NodeHeader) + 
                        curr_child_header->output_count * sizeof(output)
                ]);

                // Connect the inputs of the child nodes
                std::vector<bool> converted_inputs(curr_child_header->input_count, false);
                uint32_t curr_inner_child_offset = children_offset;

                for (size_t j = 0; j < (*funnode.get_children()).size(); j++) {
                    NodeHeader* inner_child_header = reinterpret_cast<NodeHeader*>(&buffer[curr_inner_child_offset]);
                    output* inner_child_outputs = reinterpret_cast<output*>(&buffer[
                        curr_inner_child_offset + sizeof(NodeHeader)
                    ]);

                    for (size_t input_idx = 0; input_idx < curr_child_header->input_count; input_idx++) {
                        if (!converted_inputs[input_idx]) {
                            if (curr_child_inputs[input_idx].location.node_pos == j) {
                                // This input should be connected to an output of the current inner child
                                uint32_t output_offset = curr_inner_child_offset + sizeof(NodeHeader) +
                                    curr_child_inputs[input_idx].location.connector_index * sizeof(output);

                                curr_child_inputs[input_idx].target_offset = output_offset;
                                converted_inputs[input_idx] = true;
                            }
                        }
                    }

                    curr_inner_child_offset += inner_child_header->total_size;
                }

                // Check if all inputs were converted
                assert(std::all_of(converted_inputs.begin(), converted_inputs.end(), [](bool v) { return v; }) &&
                    "Not all inputs were converted");



                // Move to the next child offset
                curr_child_offset += reinterpret_cast<NodeHeader*>(&buffer[curr_child_offset])->total_size;
            }

            // Ensure all inputs and outputs are converted
            assert(converted_inputs == input_count);
            assert(converted_outputs == output_count);
            break;
        }
        case NodeType::Bus: {
            static_assert(false, "TODO!");
            break;
        }
        default:
            break;
        }

        header->total_size = current_offset - node_offset;
        return node_offset;
    }


    LogicBlock build() {
        LogicBlock block;
        block.allocate(current_offset);
        std::memcpy(block.get<void>(0), buffer.data(), current_offset);
        return block;
    }

private:
    std::vector<uint8_t> buffer;
    size_t current_offset;

    template<typename T>
    T* add() {
        size_t size = sizeof(T);
        buffer.resize(buffer.size() + size);
        T* ptr = reinterpret_cast<T*>(&buffer[current_offset]);
        current_offset += size;
        return ptr;
    }

    template<typename T>
    void add(const T& data) {
        *add<T>() = data;
    }
};

