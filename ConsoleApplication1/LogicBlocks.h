#pragma once
#include "LogicNodes.h"
#include <type_traits>
#include <map>

class LogicBlock {
public:
    LogicBlock() : data(nullptr), size(0) {}

    void allocate(size_t byte_size) {
        data = std::make_unique<uint8_t[]>(byte_size);
        size = byte_size;
    }


    output get_input_val(size_t index, offset inputs_offset) {
        input* target_input = get_at<input>(inputs_offset + index * sizeof(input));
        offset output_offset = target_input->target_offset;
        assert(output_offset < size);
        if (!output_offset) return false;
        output* input_val = get_at<output>(output_offset);
        return *input_val;
    }

    template<typename T>
    T* get_at(offset offset) {
        assert(offset % alignof(T) == 0 && "Misaligned access");
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
                << static_cast<char>(data[i]) << ' ';
        }
        std::cout << std::dec << std::endl; // Reset to decimal format
    }
    
    size_t parse(offset node_offset = 0, std::string indent = "", size_t node_number = 0);

    offset get_children_offset(size_t header_offset) {
        NodeHeader* header = get_at<NodeHeader>(header_offset);
        switch (header->type) {
        case NodeType::FunctionNode: {
            FunctionNodeHeader* header = get_at<FunctionNodeHeader>(header_offset);
            return header->children_offset;
            break;
        }
        default:
            assert(false && "only function nodes and root nodes have children");
            break;
        }

    }

    size_t get_size() const { return size; }

    bool pretick(bool update_all, offset node_offset = 0);
    void tick(bool update_all, offset node_offset = 0);

private:
    std::unique_ptr<uint8_t[]> data;
    size_t size;
};


class LogicBlockBuilder {
public:
    LogicBlockBuilder() : current_offset(0) {}
    ~LogicBlockBuilder() {}
    size_t add_root(std::vector<Node*> nodes);

    size_t add_function_root(std::vector<Node*> nodes);

    struct OffsetStruct {
        offset shared_outputs_offset;  // Assuming 'offset' is of type int for illustration
        offset shared_new_outputs_offset;
    };
    typedef std::map<const std::string, const OffsetStruct> bus_map_t;

    offset add_node(Node& node, bus_map_t& map_ref);

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

        case NodeType::GateBUFFER:
        case NodeType::GateNOT:
            return get_at<UnaryGateHeader>(header_offset)->input_output_count;

        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton: {
            return 0;
        }
        case NodeType::LightBulb:
        case NodeType::SevenSegmentDisplay: {
            return get_at<OutputNodeHeader>(header_offset)->input_count;
        }

        case NodeType::FunctionNode: {
            return get_at<FunctionNodeHeader>(header_offset)->input_count;
        }

        case NodeType::BusNode: {
            return get_at<BusNodeHeader>(header_offset)->input_output_count;
        }
        case NodeType::RootNode: {
            return 0;
        }

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
            offset byteDistance = (char*)&gate->output - (char*)get_at<size_t>(0);
            return byteDistance;
        }

                               // Unary Gates
        case NodeType::GateBUFFER:
        case NodeType::GateNOT: {
            UnaryGateHeader* header = get_at<UnaryGateHeader>(header_offset);
            offset outputs_offset = header->outputs_offset;
            return outputs_offset;
        }

                              // Input Nodes (only have outputs)
        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton: {
            return get_at<InputNodeHeader>(header_offset)->outputs_offset;
        }

                                         // Output Nodes (only have inputs, no outputs)
        case NodeType::LightBulb:
        case NodeType::SevenSegmentDisplay: {
            return 0;
        }

        case NodeType::FunctionNode: {
            return get_at<FunctionNodeHeader>(header_offset)->outputs_offset;
        }

        case NodeType::BusNode: {
            return get_at<BusNodeHeader>(header_offset)->shared_outputs_offset;
        }

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
            return get_at<BinaryGateHeader>(header_offset)->inputs_offset;
        }

                               // Unary Gates
        case NodeType::GateBUFFER:
        case NodeType::GateNOT: {
            return get_at<UnaryGateHeader>(header_offset)->inputs_offset;
        }

                              // Input Nodes (only have outputs, no inputs)
        case NodeType::PushButton:
        case NodeType::ToggleButton:
        case NodeType::StaticToggleButton:
            return 0; // No inputs

            // Output Nodes
        case NodeType::LightBulb:
        case NodeType::SevenSegmentDisplay: {
            return get_at<OutputNodeHeader>(header_offset)->inputs_offset;
        }

        case NodeType::FunctionNode: {
            return get_at<FunctionNodeHeader>(header_offset)->inputs_offset;
        }

        case NodeType::BusNode: {
            return get_at<BusNodeHeader>(header_offset)->inputs_offset;
        }

        default:
            assert(false && "Invalid NodeType");
            return 0; // Return 0 for invalid types, though this line should never be reached due to the assert
        }
    }

    LogicBlock* build() {
        LogicBlock* block = new LogicBlock;
        block->allocate(current_offset);
        std::memcpy(block->get_at<uint8_t>(0), buffer.data(), current_offset);
        return block;
    }
private:
    std::vector<uint8_t> buffer;
    size_t current_offset;

    void connect_children(size_t children_offset, size_t child_count);

    template<typename T>
    T* add() {
        size_t size = sizeof(T);
        assert(buffer.size() == current_offset);
        assert(buffer.size() % alignof(T) == 0 && "Misaligned writing");
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
        assert(offset % alignof(T) == 0 && "Misaligned access");
        return reinterpret_cast<T*>(&buffer[offset]);
    }

    void add_padding(uint32_t alignment) {
        assert(buffer.size() == current_offset);
        uint32_t misalignment = current_offset % alignment;
        if (misalignment != 0) {
            uint32_t padding = alignment - misalignment;
            current_offset += padding;
            buffer.resize(buffer.size() + padding);
        }
        assert(buffer.size() == current_offset);
    }
};

namespace LogicblockTools {
    template<typename T>
    T* get_at(size_t offset, uint8_t* buffer) {
        return reinterpret_cast<T*>(&buffer[offset]);
    }

    size_t input_count(offset header_offset, uint8_t* buffer);

    size_t output_count(offset header_offset, uint8_t* buffer);

    offset outputs_offset(offset header_offset, uint8_t* buffer);

    offset inputs_offset(offset header_offset, uint8_t* buffer);
}