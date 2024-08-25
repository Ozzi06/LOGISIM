#pragma once
#include "Node.h"

struct LightBulb : public Node {
    LightBulb(std::vector<Node*>* container, Vector2 pos = { 0,0 }) : Node(container, pos, { 0, 0 }, YELLOW) {
        label = "Light Bulb";

        size = Vector2{ 100, 100 };
        outputs.clear();
        inputs.clear();
        inputs.push_back(Input_connector(this, 0, ""));
    }
    LightBulb(const LightBulb* base) : Node(base) {}

    virtual void draw() override;

    virtual void add_input() override {}
    virtual void remove_input() override {}

    Node* copy() const override { return new LightBulb(this); }

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return { 0 }; }

    virtual std::string get_type_str() const override { return"LightBulb"; }
    virtual NodeType get_type() const override { return NodeType::LightBulb; }

    virtual bool isOutput() const override { return true; }
};

struct SevenSegmentDisplay : public Node {
    SevenSegmentDisplay(std::vector<Node*>* container, Vector2 pos = { 0,0 });
    SevenSegmentDisplay(const SevenSegmentDisplay* base) : Node(base) {}

    virtual void draw() override;

    virtual void add_input() override {}
    virtual void remove_input() override {}

    Node* copy() const override { return new SevenSegmentDisplay(this); }

    virtual std::string get_type_str() const override { return"SevenSegmentDisplay"; }
    virtual NodeType get_type() const override { return NodeType::SevenSegmentDisplay; }
    virtual std::string get_label() const override { return std::string(label); }
    virtual Texture get_texture() const override { return { 0 }; }

    virtual bool isOutput() const override { return false; }

protected:
    virtual void recompute_size() override;
};