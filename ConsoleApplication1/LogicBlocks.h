#pragma once
#include "LogicNodes.h"
#include <type_traits>
#include <map>

class LogicBlock {
public:
    LogicBlock() : data(nullptr), size(0) {}

    LogicBlock(const LogicBlock& base) : data(nullptr), size(0) {
        allocate(base.size);
        std::memcpy(data.get(), base.data.get(), size);
    }

    LogicBlock(LogicBlock&& other) noexcept : size(other.size) {
        data = std::move(other.data);
    }

    void allocate(size_t byte_size) {
        data = std::make_unique<uint8_t[]>(byte_size);
        size = byte_size;
    }

    const NodeHeader* get_header() const {
        return reinterpret_cast<const NodeHeader*>(data.get());
    }

    void save_to_file(const std::string& filename) const {
        assert(!"TODO");
        std::ofstream file(filename, std::ios::binary);
        file.write(reinterpret_cast<const char*>(data.get()), size);
    }

    static LogicBlock load_from_file(const std::string& filename) {
        assert(!"TODO");
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
    
    size_t parse(uint8_t* container, size_t node_offset = 0, std::string indent = "", size_t node_number = 0);

    size_t get_size() const { return size; }

    bool pretick(uint8_t* container, bool update_all, offset node_offset);
    void tick(uint8_t* container, bool update_all, offset node_offset);

    uint8_t* get_data(size_t offset) {
        return &data[offset];
    }


private:
    std::unique_ptr<uint8_t[]> data;
    size_t size;
};


class LogicBlockBuilder {
public:
    LogicBlockBuilder() : current_absolute_offset(0) {}
    ~LogicBlockBuilder() {}
    size_t add_root(std::vector<Node*> nodes);

    size_t add_function_root(std::vector<Node*> nodes);

    struct BusOffsetStruct {
        offset shared_outputs_offset;  // Assuming 'offset' is of type int for illustration
        offset shared_new_outputs_offset;
    };
    typedef std::map<const std::string, const BusOffsetStruct> bus_map_t;

    void add_node(Node& node, bus_map_t& bus_map, size_t abs_container_offset);
    
    LogicBlock* build() {
        LogicBlock* block = new LogicBlock;
        block->allocate(current_absolute_offset);
        std::memcpy(block->get_data(0), buffer.data(), current_absolute_offset);
        return block;
    }
private:
    std::vector<uint8_t> buffer;
    size_t current_absolute_offset;

    void connect_children(uint8_t* container, size_t child_count);

    template<typename T>
    T* add() {
        size_t size = sizeof(T);
        assert(buffer.size() == current_absolute_offset);
        assert(buffer.size() % alignof(T) == 0 && "Misaligned writing");
        buffer.resize(buffer.size() + size);
        T* ptr = reinterpret_cast<T*>(&buffer[current_absolute_offset]);
        current_absolute_offset += size;
        return ptr;
    }

    template<typename T>
    void add(const T& data) {
        *add<T>() = data;
    }

    void add_raw(uint8_t* data, size_t size) {
        assert(data && size);
        assert(buffer.size() == current_absolute_offset);
        size_t old_size = buffer.size();
        buffer.resize(buffer.size() + size);
        std::memcpy(&buffer[old_size], data, size);
        current_absolute_offset += size;
    }

    template<typename T>
    T* get_at_abs(size_t absolute_offset) {
        assert(absolute_offset < buffer.size());
        assert(absolute_offset % alignof(T) == 0 && "Misaligned access");
        return reinterpret_cast<T*>(&buffer[absolute_offset]);
    }

    void add_padding(uint32_t alignment) {
        assert(buffer.size() == current_absolute_offset);
        uint32_t misalignment = current_absolute_offset % alignment;
        if (misalignment != 0) {
            uint32_t padding = alignment - misalignment;
            current_absolute_offset += padding;
            buffer.resize(buffer.size() + padding);
        }
        assert(buffer.size() == current_absolute_offset);
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

    output get_input_val(size_t index, offset inputs_offset, offset node_offset, uint8_t* buffer);
}