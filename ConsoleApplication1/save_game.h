#pragma once
#include <string>
#include <fstream>
#include <cassert>
#include "main_game.h"
#include "game.h"
//memory layout:

/*
SaveHeader
    uint32_t version
    char[32] namestring
    uint64_t total_size
    size_t node_count
    size_t Nodes_offset
    size_t LogicBlock_offset
    size_t LogicBlock_size

SaveHeader.node_count * {
    NodeData
        NodeType type (enum uint32_t)
        char label[64]
        Vector2 pos (2 * float)
        Vector2 size (2 * float)
        Color color (4 * uint8_t)
        size_t abs_node_offset
        uint32_t input_count
        uint32_t output_count
        size_t inputs_offset
        size_t outputs_offset
        size_t total_size

        NodeData.input_count * {
            InputData
                uint32_t target_id
                char[56] name
        }

        NodeData.output_count * {
            OutputData
                uint32_t id
                char[56] name
                bool state
        }
}

LogicBlock (raw bytes, size determined by game.logicblock_size())
*/

struct VersionField {
    VersionField(size_t version_number) : version_number(version_number) {};
    size_t version_number;
};

struct alignas(size_t) SaveHeader {
    VersionField version = 0;
    char namestring[64] = "a binary savefile for logisim";
    size_t total_size;
    size_t node_count;
    size_t Nodes_offset;
    size_t LogicBlock_offset;
    size_t LogicBlock_size;
};

struct alignas(size_t) NodeData {
    NodeType type;
    char label[64];
    Vector2 pos;
    Vector2 size;
    Color color;
    size_t abs_node_offset;
    uint32_t input_count;
    uint32_t output_count;
    size_t inputs_offset;
    size_t outputs_offset;
    size_t total_size;
};

struct alignas(size_t) InputData {
    uid_t target_id;
    char name[56];
    uint32_t pad;
};

struct alignas(size_t) OutputData {
    uid_t id;
    char name[56];
    bool state;
    uint8_t pad1;
    uint16_t pad2;
};

class SaveBuilder {
public:
    SaveBuilder() {}
    ~SaveBuilder() {}

    void save_game(std::string filePath = "gamesave.bin");

private:
    std::vector<uint8_t> buffer;
    size_t current_absolute_offset = 0;

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

