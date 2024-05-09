#pragma once
#include <vector>

class LogicNode;

class NodeContainer {
public:
    std::vector<LogicNode> nodes;
    std::vector<void*> external_ptrs;

    void pretick();
    bool tick(); // returns true if any node returns true when ticked

    bool has_changed = true;

    LogicNode* addNode(
        void (*pretickFunc)(LogicNode&),
        bool (*tickFunc)(LogicNode&),
        void (*destructor)(LogicNode&)
    );

    void removeNode(int16_t idx);
};

using pretick_ptr = void (*)(LogicNode&);
using tick_ptr = bool (*)(LogicNode&);
using destructor_ptr = void (*)(LogicNode&);

class LogicNode {
public:
    LogicNode(
        NodeContainer* container,
        pretick_ptr pretickFunc,
        tick_ptr tickFunc,
        destructor_ptr destructor
    ) : container(container), pretickFunc(pretickFunc), tickFunc(tickFunc), destructor(destructor) {}

    ~LogicNode() {
        destructor(*this);
    }

    NodeContainer* container;

    uint16_t external_ptr_idx = 0;

    uint32_t new_outputs = 0;
    uint32_t outputs = 0;
    uint16_t input_idxs[32] = {0}; //11 bits for indexing in container, 5 for correct output in node

    void pretick() {
        pretickFunc(*this);
    }
    bool tick() {
        return tickFunc(*this);
    }

    bool output_val(char idx) {
        return outputs & (01 << (input_idxs[idx] & 0x1F));
    }

    bool new_output_val(char idx) {
        return new_outputs & (01 << (input_idxs[idx] & 0x1F));
    }

    bool get_input_target_val(char idx) {
        return container->nodes[input_idxs[idx] >> 5].outputs & (01 << (input_idxs[idx] & 0x1F));
    }

private:
    void (*pretickFunc)(LogicNode&);
    bool (*tickFunc)(LogicNode&); // should return true if it's output changed or it needs to run again
    void (*destructor)(LogicNode&);
};