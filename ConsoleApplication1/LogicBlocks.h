#pragma once
#include "LogicNodes.h"

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

    void hexdump() {
        for (size_t i = 0; i < size; ++i) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                << static_cast<int>(data[i]) << ' ';
        }
        std::cout << std::dec << std::endl; // Reset to decimal format
    }
    
    size_t parse(size_t offset = 0, std::string indent = "") {
        assert(offset < size);
        NodeHeader* header = get<NodeHeader>(offset);
        std::cout << indent << "header:\n"
            << indent << "  type: " << toString(header->type) << "\n"
            << indent << "  total_size: " << header->total_size << "\n";

        if (header->type == NodeType::FunctionNode) {
            FunctionNodeHeader* header = get<FunctionNodeHeader>(offset);
            std::cout << indent << "children:\n";
            indent.append("  ");
            size_t new_offset = get_children_offset(offset);

            for (size_t i = 0; i < header->child_count; i++) {
                new_offset += parse(new_offset, indent);
            }
        }
        if (header->type == NodeType::RootNode) {
            RootNodeHeader* header = get<RootNodeHeader>(offset);
            std::cout << indent << "children:\n";
            indent.append("  ");
            size_t new_offset = get_children_offset(offset);

            for (size_t i = 0; i < header->child_count; i++) {
                new_offset += parse(new_offset, indent);
            }
        }

        return header->total_size;
    }

    size_t get_children_offset(size_t header_offset) {
        NodeHeader* header = get<NodeHeader>(header_offset);
        switch (header->type) {
        case NodeType::FunctionNode: {
            FunctionNodeHeader* header = get<FunctionNodeHeader>(header_offset);
            return header_offset + sizeof(FunctionNodeHeader)
                + header->input_targ_node_count * sizeof(offset)
                + header->output_targ_node_count * sizeof(offset)
                + header->input_count * sizeof(input)
                + 2 * header->output_count * sizeof(output);
            break;
        }
        case NodeType::RootNode: {
            RootNodeHeader* header = get<RootNodeHeader>(header_offset);
            return header_offset + sizeof(RootNodeHeader);
            break;
        }
        default:
            assert(false && "only function nodes and root nodes have children");
            break;
        }

    }

    size_t get_size() const { return size; }

private:
    std::unique_ptr<uint8_t[]> data;
    size_t size;
};

class LogicBlockBuilder {
public:
    LogicBlockBuilder() : current_offset(0) {}

    size_t add_root(const std::vector<Node*> nodes);

    size_t add_function_root(const std::vector<Node*> nodes);

    size_t add_node(const Node& node);

    size_t input_count(offset header_offset) {
        NodeHeader* header = get_at<NodeHeader>(header_offset);
        switch (header->type)
        {
        case NodeType::GateAND:
        case NodeType::GateOR:
        case NodeType::GateNAND:
        case NodeType::GateNOR:
        case NodeType::GateXOR:
        case NodeType::GateXNOR:
            return get_at<BinaryGateHeader>(header_offset)->input_count;
            break;

        case NodeType::GateBUFFER:
        case NodeType::GateNOT:
            return get_at<UnaryGateHeader>(header_offset)->input_output_count;
            break;

        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton: {
            return 0;
        }
        case NodeType::LightBulb: {
            return get_at<OutputNodeHeader>(header_offset)->input_count;
        }
        case NodeType::SevenSegmentDisplay: {
            return get_at<GenericNodeHeader>(header_offset)->input_count;
            break;
        }

        case NodeType::FunctionNode: {
            return get_at<FunctionNodeHeader>(header_offset)->input_count;
            break;
        }

        case NodeType::Bus:
            return get_at<BusNodeHeader>(header_offset)->input_count;
            break;

        case NodeType::RootNode:
            assert(false && "not a child");
        default:
            assert(false && "unreachable");
        }
    }
    offset outputs_offset(offset header_offset) {
        NodeHeader* header = get_at<NodeHeader>(header_offset);
        switch (header->type) {
            // Binary Gates
        case NodeType::GateAND:
        case NodeType::GateOR:
        case NodeType::GateNAND:
        case NodeType::GateNOR:
        case NodeType::GateXOR:
        case NodeType::GateXNOR: {
            BinaryGateHeader* gate = get_at<BinaryGateHeader>(header_offset);
            return header_offset + sizeof(BinaryGateHeader) + gate->input_count * sizeof(input);
        }

                               // Unary Gates
        case NodeType::GateBUFFER:
        case NodeType::GateNOT: {
            UnaryGateHeader* gate = get_at<UnaryGateHeader>(header_offset);
            return header_offset + sizeof(UnaryGateHeader) + gate->input_output_count * sizeof(input);
        }

                              // Input Nodes (only have outputs)
        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton: {
            InputNodeHeader* input = get_at<InputNodeHeader>(header_offset);
            return header_offset + sizeof(InputNodeHeader);
        }

                                         // Output Nodes (only have inputs, no outputs)
        case NodeType::LightBulb:
            return 0; // No outputs

            // Generic Node
        case NodeType::SevenSegmentDisplay: {
            GenericNodeHeader* generic = get_at<GenericNodeHeader>(header_offset);
            return header_offset + sizeof(GenericNodeHeader) + generic->input_count * sizeof(input);
        }

        case NodeType::FunctionNode: {
            FunctionNodeHeader* func = get_at<FunctionNodeHeader>(header_offset);
            return header_offset + sizeof(FunctionNodeHeader) +
                func->input_targ_node_count * sizeof(offset) +
                func->output_targ_node_count * sizeof(offset) +
                func->input_count * sizeof(input);
        }

        case NodeType::Bus: {
            BusNodeHeader* bus = get_at<BusNodeHeader>(header_offset);
            return bus->shared_outputs_offset;
        }

        case NodeType::RootNode:
            assert(false && "RootNode can't be a child");
            return 0; // RootNode doesn't have outputs

        default:
            assert(false && "Invalid NodeType");
            return 0; // Return 0 for invalid types, though this line should never be reached due to the assert
        }
    }

    offset inputs_offset(offset header_offset) {
        NodeHeader* header = get_at<NodeHeader>(header_offset);
        switch (header->type) {
            // Binary Gates
        case NodeType::GateAND:
        case NodeType::GateOR:
        case NodeType::GateNAND:
        case NodeType::GateNOR:
        case NodeType::GateXOR:
        case NodeType::GateXNOR: {
            return header_offset + sizeof(BinaryGateHeader);
        }

                               // Unary Gates
        case NodeType::GateBUFFER:
        case NodeType::GateNOT: {
            return header_offset + sizeof(UnaryGateHeader);
        }

                              // Input Nodes (only have outputs, no inputs)
        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton:
            return 0; // No inputs

            // Output Nodes
        case NodeType::LightBulb: {
            return header_offset + sizeof(OutputNodeHeader);
        }

                                // Generic Node
        case NodeType::SevenSegmentDisplay: {
            return header_offset + sizeof(GenericNodeHeader);
        }

        case NodeType::FunctionNode: {
            FunctionNodeHeader* func = get_at<FunctionNodeHeader>(header_offset);
            return header_offset + sizeof(FunctionNodeHeader) +
                func->input_targ_node_count * sizeof(offset) +
                func->output_targ_node_count * sizeof(offset);
        }

        case NodeType::Bus: {
            return header_offset + sizeof(BusNodeHeader);
        }

        case NodeType::RootNode:
            assert(false && "RootNode can't be a child");
            return 0; // RootNode doesn't have inputs

        default:
            assert(false && "Invalid NodeType");
            return 0; // Return 0 for invalid types, though this line should never be reached due to the assert
        }
    }

    LogicBlock* build() {
        LogicBlock* block = new LogicBlock;
        block->allocate(current_offset);
        std::memcpy(block->get<void>(0), buffer.data(), current_offset);
        return block;
    }
private:
    std::vector<uint8_t> buffer;
    size_t current_offset;

    void connect_children(size_t children_offset, size_t child_count);

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

    template<typename T>
    T* get_at(size_t offset) {
        assert(offset < buffer.size());
        return reinterpret_cast<T*>(&buffer[offset]);
    }
};

namespace LogicblockTools {
    template<typename T>
    T* get_at(size_t offset, uint8_t* buffer) {
        return reinterpret_cast<T*>(&buffer[offset]);
    }

    size_t input_count(offset header_offset, uint8_t* buffer) {
        NodeHeader* header = get_at<NodeHeader>(header_offset, buffer);
        switch (header->type)
        {
        case NodeType::GateAND:
        case NodeType::GateOR:
        case NodeType::GateNAND:
        case NodeType::GateNOR:
        case NodeType::GateXOR:
        case NodeType::GateXNOR:
            return get_at<BinaryGateHeader>(header_offset, buffer)->input_count;
            break;

        case NodeType::GateBUFFER:
        case NodeType::GateNOT:
            return get_at<UnaryGateHeader>(header_offset, buffer)->input_output_count;
            break;

        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton: {
            return 0;
        }
        case NodeType::LightBulb: {
            return get_at<OutputNodeHeader>(header_offset, buffer)->input_count;
        }
        case NodeType::SevenSegmentDisplay: {
            return get_at<GenericNodeHeader>(header_offset, buffer)->input_count;
            break;
        }

        case NodeType::FunctionNode: {
            return get_at<FunctionNodeHeader>(header_offset, buffer)->input_count;
            break;
        }

        case NodeType::Bus:
            return get_at<BusNodeHeader>(header_offset, buffer)->input_count;
            break;

        case NodeType::RootNode:
            assert(false && "not a child");
        default:
            assert(false && "unreachable");
        }
    }
    offset outputs_offset(offset header_offset, uint8_t* buffer) {
        NodeHeader* header = get_at<NodeHeader>(header_offset, buffer);
        switch (header->type) {
            // Binary Gates
        case NodeType::GateAND:
        case NodeType::GateOR:
        case NodeType::GateNAND:
        case NodeType::GateNOR:
        case NodeType::GateXOR:
        case NodeType::GateXNOR: {
            BinaryGateHeader* gate = get_at<BinaryGateHeader>(header_offset, buffer);
            return header_offset + sizeof(BinaryGateHeader) + gate->input_count * sizeof(input);
        }

                               // Unary Gates
        case NodeType::GateBUFFER:
        case NodeType::GateNOT: {
            UnaryGateHeader* gate = get_at<UnaryGateHeader>(header_offset, buffer);
            return header_offset + sizeof(UnaryGateHeader) + gate->input_output_count * sizeof(input);
        }

                              // Input Nodes (only have outputs)
        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton: {
            InputNodeHeader* input = get_at<InputNodeHeader>(header_offset, buffer);
            return header_offset + sizeof(InputNodeHeader);
        }

                                         // Output Nodes (only have inputs, no outputs)
        case NodeType::LightBulb:
            return 0; // No outputs

            // Generic Node
        case NodeType::SevenSegmentDisplay: {
            GenericNodeHeader* generic = get_at<GenericNodeHeader>(header_offset, buffer);
            return header_offset + sizeof(GenericNodeHeader) + generic->input_count * sizeof(input);
        }

        case NodeType::FunctionNode: {
            FunctionNodeHeader* func = get_at<FunctionNodeHeader>(header_offset, buffer);
            return header_offset + sizeof(FunctionNodeHeader) +
                func->input_targ_node_count * sizeof(offset) +
                func->output_targ_node_count * sizeof(offset) +
                func->input_count * sizeof(input);
        }

        case NodeType::Bus: {
            BusNodeHeader* bus = get_at<BusNodeHeader>(header_offset, buffer);
            return bus->shared_outputs_offset;
        }

        case NodeType::RootNode:
            assert(false && "RootNode can't be a child");
            return 0; // RootNode doesn't have outputs

        default:
            assert(false && "Invalid NodeType");
            return 0; // Return 0 for invalid types, though this line should never be reached due to the assert
        }
    }

    offset inputs_offset(offset header_offset, uint8_t* buffer) {
        NodeHeader* header = get_at<NodeHeader>(header_offset, buffer);
        switch (header->type) {
            // Binary Gates
        case NodeType::GateAND:
        case NodeType::GateOR:
        case NodeType::GateNAND:
        case NodeType::GateNOR:
        case NodeType::GateXOR:
        case NodeType::GateXNOR: {
            return header_offset + sizeof(BinaryGateHeader);
        }

                               // Unary Gates
        case NodeType::GateBUFFER:
        case NodeType::GateNOT: {
            return header_offset + sizeof(UnaryGateHeader);
        }

                              // Input Nodes (only have outputs, no inputs)
        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton:
            return 0; // No inputs

            // Output Nodes
        case NodeType::LightBulb: {
            return header_offset + sizeof(OutputNodeHeader);
        }

                                // Generic Node
        case NodeType::SevenSegmentDisplay: {
            return header_offset + sizeof(GenericNodeHeader);
        }

        case NodeType::FunctionNode: {
            FunctionNodeHeader* func = get_at<FunctionNodeHeader>(header_offset, buffer);
            return header_offset + sizeof(FunctionNodeHeader) +
                func->input_targ_node_count * sizeof(offset) +
                func->output_targ_node_count * sizeof(offset);
        }

        case NodeType::Bus: {
            return header_offset + sizeof(BusNodeHeader);
        }

        case NodeType::RootNode:
            assert(false && "RootNode can't be a child");
            return 0; // RootNode doesn't have inputs

        default:
            assert(false && "Invalid NodeType");
            return 0; // Return 0 for invalid types, though this line should never be reached due to the assert
        }
    }
}