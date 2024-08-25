#pragma once
#include "Node.h"

struct UnaryLogicGate : public Node {
    UnaryLogicGate(std::vector<Node*>* container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        inputs.push_back(Input_connector(this, 0, "", input));
        recompute_size();
    }
    UnaryLogicGate(const UnaryLogicGate* base) : Node(base) {}


    virtual void add_input() override;
    virtual void remove_input() override;


    virtual std::vector<Input_connector*> connected_inputs(size_t output_idx) {
        std::vector<Input_connector*> a;
        a.push_back(&inputs[output_idx]);
        return a;
    }
};

struct GateBUFFER : public UnaryLogicGate {
    GateBUFFER(std::vector<Node*>* container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : UnaryLogicGate(container, pos, input) {
        label = "BUFFER";
    }
    GateBUFFER(const GateBUFFER* base) : UnaryLogicGate(base) {}

    Node* copy() const override { return new GateBUFFER(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;
    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateBUFFER"; }
    virtual NodeType get_type() const override { return NodeType::GateBUFFER; }
};

struct GateNOT : public UnaryLogicGate {
    GateNOT(std::vector<Node*>* container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : UnaryLogicGate(container, pos, input) {
        label = "NOT";
    }
    GateNOT(const GateNOT* base) : UnaryLogicGate(base) {}

    Node* copy() const override { return new GateNOT(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;
    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateNOT"; }
    virtual NodeType get_type() const override { return NodeType::GateNOT; }
};
