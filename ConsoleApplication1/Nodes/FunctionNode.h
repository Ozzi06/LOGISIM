#pragma once
#include "Node.h"

class FunctionNode : public Node {
public:
    //constructors and destructors
    FunctionNode(std::vector<Node*>* container, Vector2 pos = { 0, 0 }) : Node(container, pos, { 0, 0 }, ColorBrightness(GRAY, -0.6f)), is_single_tick(false) { label = "Function"; }
    FunctionNode(const FunctionNode* base);
    ~FunctionNode();

    Node* copy() const override { return new FunctionNode(this); }

    virtual void draw() override;
    virtual bool show_node_editor() override;

    virtual void add_input() override {}
    virtual void remove_input() override {}

    virtual std::string get_type_str() const override { return"FunctionNode"; }
    virtual NodeType get_type() const override { return NodeType::FunctionNode; }
    virtual std::string get_label() const override { return std::string(label); }
    virtual Texture get_texture() const override { return { 0 }; }
    virtual const std::vector<Node*>* get_children() const override { return &nodes; }


    virtual json to_JSON() const override;
    virtual void load_extra_JSON(const json& nodeJson) override;
    virtual void load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr) override;
    virtual void recompute_size() override;

    virtual int delay() const override;
    virtual bool is_cyclic() const override;


    void load_from_nodes();
    void sort_linear();

    std::vector<Node*> nodes;
    std::vector<Node*> input_targs;
    std::vector<Node*> output_targs;

    LogicBlock* get_node_data_save() { return &node_data_save; }
    bool has_node_data_save() { return node_data_save.get_size() > 0; }
    void allocate_node_data_save(const uint8_t* node);
private:
    LogicBlock node_data_save;

    std::optional<bool> is_cyclic_val;
    bool is_single_tick;
    std::string delay_str;
};