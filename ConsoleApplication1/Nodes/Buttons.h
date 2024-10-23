#pragma once
#include "Node.h"

struct Button :public Node {
    virtual Rectangle getButtonRect(size_t id);
    virtual void draw() override;

    Button(std::vector<Node*>* container, Vector2 pos = { 0,0 }, Color color = ColorBrightness(BLUE, -0.4f)) : Node(container, pos, { 0, 0 }, color) {
        recompute_size();
    }
    Button(const Button* base) : Node(base) {}

    virtual void add_input() override;

    virtual void remove_input() override;

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return{ 0 }; }

    virtual void not_clicked() = 0;
    virtual void clicked(Vector2 pos) = 0;

    virtual void set_output_state(size_t index, bool new_state);

    virtual bool isInput() const override { return true; }

    virtual void recompute_size() override;
};

struct PushButton : public Button {
    PushButton(std::vector<Node*>* container, Vector2 pos = { 0,0 }) : Button(container, pos) {
        label = "Push Button";
    }
    PushButton(const PushButton* base) : Button(base) {}

    Node* copy() const override { return new PushButton(this); }

    virtual void not_clicked() override;

    virtual void clicked(Vector2 pos) override;

    virtual std::string get_type_str() const override { return"PushButton"; }
    virtual NodeType get_type() const override { return NodeType::PushButton; }
};

struct ToggleButton : public Button {
    ToggleButton(std::vector<Node*>* container, Vector2 pos = { 0,0 }) : Button(container, pos) {
        label = "Toggle Button";
    }
    ToggleButton(const ToggleButton* base) : Button(base) {}

    Node* copy() const override { return new ToggleButton(this); }

    virtual void not_clicked() override {}

    virtual void clicked(Vector2 pos) override;
    virtual void load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr) override;

    virtual std::string get_type_str() const override { return"ToggleButton"; }
    virtual NodeType get_type() const override { return NodeType::ToggleButton; }
};

struct StaticToggleButton : public ToggleButton {

    StaticToggleButton(std::vector<Node*>* container, Vector2 pos = { 0,0 }) : ToggleButton(container, pos) {
        label = "Static Toggle Button";
    }
    StaticToggleButton(const StaticToggleButton* base) : ToggleButton(base) {
    }

    Node* copy() const override { return new StaticToggleButton(this); }

    virtual std::string get_type_str() const override { return"StaticToggleButton"; }
    virtual NodeType get_type() const override { return NodeType::StaticToggleButton; }

    virtual bool isInput() const override { return false; }
};
