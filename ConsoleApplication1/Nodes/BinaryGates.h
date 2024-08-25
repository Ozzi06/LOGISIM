#pragma once
#include "Node.h"

struct BinaryLogicGate : public Node {
    BinaryLogicGate(std::vector<Node*>* container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size(), ""));

        recompute_size();
    }
    BinaryLogicGate(const BinaryLogicGate* base) : Node(base) {}

    virtual void add_input() override;

    virtual void remove_input() override;
};

struct GateAND : public BinaryLogicGate {
    GateAND(std::vector<Node*>* container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "AND";
    }
    GateAND(const GateAND* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateAND(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateAND"; }
    virtual NodeType get_type() const override { return NodeType::GateAND; }
};

struct GateOR : public BinaryLogicGate {
    GateOR(std::vector<Node*>* container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "OR";
    }
    GateOR(const GateOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateOR"; }
    virtual NodeType get_type() const override { return NodeType::GateOR; }
};

struct GateNAND : public BinaryLogicGate {
    GateNAND(std::vector<Node*>* container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "NAND";
    }
    GateNAND(const GateNAND* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateNAND(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateNAND"; }
    virtual NodeType get_type() const override { return NodeType::GateNAND; }
};

struct GateNOR : public BinaryLogicGate {
    GateNOR(std::vector<Node*>* container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "NOR";
    }
    GateNOR(const GateNOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateNOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateNOR"; }
    virtual NodeType get_type() const override { return NodeType::GateNOR; }
};

struct GateXOR : public BinaryLogicGate {
    GateXOR(std::vector<Node*>* container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "XOR";
    }
    GateXOR(const GateXOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateXOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateXOR"; }
    virtual NodeType get_type() const override { return NodeType::GateXOR; }
};

struct GateXNOR : public BinaryLogicGate {
    GateXNOR(std::vector<Node*>* container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "XNOR";
    }
    GateXNOR(const GateXNOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateXNOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateXNOR"; }
    virtual NodeType get_type() const override { return NodeType::GateXNOR; }
};