#pragma once
#include <vector>

class LogicNode;

using pretick_ptr = void (*)(LogicNode&);
using tick_ptr = bool (*)(LogicNode&);
using destructor_ptr = void (*)(LogicNode&);

class NodeConnectionIndex {
private:
    uint16_t combinedIndex;

    static const uint16_t OUTPUT_MASK = 0b11111;
    static const int NODE_SHIFT = 5;

public:
    NodeConnectionIndex(int nodeIndex, uint8_t outputIndex) {
        setIndex(nodeIndex, outputIndex);
    }

    NodeConnectionIndex() : combinedIndex(0) {}

    void setIndex(int nodeIndex, uint8_t outputIndex) {
        combinedIndex = (nodeIndex << NODE_SHIFT) | (outputIndex & OUTPUT_MASK);
    }

    int getContainerIndex() const {
        return combinedIndex >> NODE_SHIFT;
    }

    uint8_t getConnectorIndex() const {
        return combinedIndex & OUTPUT_MASK;
    }
};

class LogicNodeContainer {
public:
    std::vector<LogicNode> nodes;
    std::vector<void*> external_ptrs;

    void pretick();
    bool tick(); // returns true if any node returns true when ticked

    void reset() {
        nodes.clear();
        external_ptrs.clear();
    }

    bool has_changed = true;

    LogicNode* add_node(
        void (*pretickFunc)(LogicNode&),
        bool (*tickFunc)(LogicNode&),
        void (*destructor)(LogicNode&)
    );

    uint16_t store_external_ptr(void* ptr) {
        external_ptrs.push_back(ptr);
        return external_ptrs.size() - 1;
    }

    void* get_external_ptr(uint16_t idx) {
        return external_ptrs[idx];
    }

    bool connect_node(LogicNode* from_ptr, uint8_t from_idx, LogicNode* to_ptr, uint8_t to_idx);
};

class LogicNode {
public:
    LogicNode(
        LogicNodeContainer* container,
        pretick_ptr pretickFunc,
        tick_ptr tickFunc,
        destructor_ptr destructor
    ) : container(container), pretickFunc(pretickFunc), tickFunc(tickFunc), destructor(destructor) {}

    ~LogicNode() {
        destructor(*this); // used to manage external pointers
    }

    uint16_t add_external_ptr(void* ptr) {
        return container->store_external_ptr(ptr);
    }

    LogicNodeContainer* container; //64 bits

    uint16_t external_ptr_idx = UINT16_MAX;
    uint64_t new_outputs = 0;
    uint64_t outputs = 0;
    NodeConnectionIndex input_idxs[32] = {}; //11 bits for indexing in container, 5 for correct output in node, 512 bits total
    uint32_t connected_inputs = 0;
    

    void pretick() {
        pretickFunc(*this);
    }
    bool tick() {
        return tickFunc(*this);
    }

    bool output_val(uint8_t idx) {
        return outputs      & (01 << idx);
    }

    bool new_output_val(uint8_t idx) {
        return new_outputs  & (01 << idx);
    }

    bool get_input_target_val(uint8_t idx) {
        return 
            container->nodes[input_idxs[idx].getContainerIndex()] // node pointed to
            .outputs & (01 << (input_idxs[idx].getConnectorIndex())); // connector of node
    }



private:
    void (*pretickFunc)(LogicNode&);
    bool (*tickFunc)(LogicNode&); // should return true if it's output changed or it needs to run again
    void (*destructor)(LogicNode&);
};
int a = (sizeof(LogicNode) * 8);