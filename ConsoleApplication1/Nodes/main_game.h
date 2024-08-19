#pragma once
#include "gui_ui.h"
#include <vector>

#include "nlohmann/json.hpp"
#include <utility>
#include "LogicBlocks.h"
#include "raylib.h"
#include <filesystem>
#include "Node.h"

using json = nlohmann::json;

struct Output_connector;
struct Input_connector;
struct Node;
class Game;

struct GuiNodeEditorState;

class LogicBlock;


struct BinaryLogicGate : public Node {
    BinaryLogicGate(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
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
    GateAND(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
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
    GateOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
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
    GateNOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
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
    GateXOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
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
    GateXNOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "XNOR";
    }
    GateXNOR(const GateXNOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateXNOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual std::string get_type_str() const override { return"GateXNOR"; }
    virtual NodeType get_type() const override { return NodeType::GateXNOR; }
};

struct UnaryLogicGate : public Node {
    UnaryLogicGate(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
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
    GateBUFFER(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : UnaryLogicGate(container, pos, input) {
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
    GateNOT(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : UnaryLogicGate(container, pos, input) {
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

struct Bus : public Node {
    Bus(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "BUS_0";
        inputs.push_back(Input_connector(this, 0, "", input));
        recompute_size();
        find_connections();
        
    }
    Bus(const Bus* base) : Node(base) {
        find_connections();
    }

    void find_connections() {
        for (Node* node : *container) {
            if (node) {
                Bus* bus = dynamic_cast<Bus*>(node);
                if (bus && bus != this) {
                    if (bus->label == label) {
                        bus_values_has_updated = bus->bus_values_has_updated;
                        bus_values = bus->bus_values;
                        while ((*bus_values).size() < inputs.size()) {
                            (*bus_values).push_back(false);
                        }
                        return;
                    }
                }
            }
        }

        bus_values_has_updated = std::make_shared<bool>();
        bus_values = std::make_shared<std::vector<bool>>();
        while ((*bus_values).size() < inputs.size()) {
            (*bus_values).push_back(false);
        }
    }

    virtual void add_input() override;

    virtual void remove_input() override;

    virtual void change_label(const char* newlabel) override;

    Node* copy() const override { return new Bus(this); }

    virtual json to_JSON() const override;

    virtual void load_extra_JSON(const json& nodeJson) override;
    virtual void load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr) override;

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return{ 0 }; }

    virtual std::string get_type_str() const override { return"Bus"; }
    virtual NodeType get_type() const override { return NodeType::BusNode; }

    virtual std::vector<Input_connector*> connected_inputs(size_t output_idx) {
        std::vector<Input_connector*> conned;

        for (Node* node : *container) {
            Bus* bus = dynamic_cast<Bus*>(node);
            if (bus) {
                if (bus->label == label && bus->inputs.size() > output_idx) {
                    conned.push_back(&bus->inputs[output_idx]);
                }
            }
        }
        return conned;
    }

    size_t bus_values_size() const { return bus_values->size(); }
    std::weak_ptr<const std::vector<bool>> get_bus_values() const{ return bus_values; };
private:
    std::shared_ptr<std::vector<bool>> bus_values;
    std::shared_ptr<bool> bus_values_has_updated;
};

struct Button :public Node {
    virtual Rectangle getButtonRect(size_t id);
    virtual void draw() override;

    Button(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Color color = ColorBrightness(BLUE, -0.4f)) : Node(container, pos, { 0, 0 }, color) {
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
    PushButton(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : Button(container, pos) {
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
    ToggleButton(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : Button(container, pos) {
        label = "Toggle Button";
    }
    ToggleButton(const ToggleButton* base) : Button(base) {}

    Node* copy() const override { return new ToggleButton(this); }

    virtual void not_clicked() override {}

    virtual void clicked(Vector2 pos) override;

    virtual std::string get_type_str() const override { return"ToggleButton"; }
    virtual NodeType get_type() const override { return NodeType::ToggleButton; }
};

struct StaticToggleButton : public ToggleButton {

    StaticToggleButton(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : ToggleButton(container, pos) {
        label = "Static Toggle Button";
    }
    StaticToggleButton(const StaticToggleButton* base) : ToggleButton(base) {
    }

    Node* copy() const override { return new StaticToggleButton(this); }

    virtual std::string get_type_str() const override { return"StaticToggleButton"; }

    virtual bool isInput() const override { return false; }
};

struct LightBulb : public Node {
    LightBulb(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : Node(container, pos, { 0, 0 }, YELLOW) {
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

class FunctionNode : public Node {
public:
    //constructors and destructors
    FunctionNode(std::vector<Node*> * container, Vector2 pos = {0, 0}) : Node(container, pos, {0, 0}, ColorBrightness(GRAY, -0.6f)), is_single_tick(false) {label = "Function";}
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
    virtual Texture get_texture() const override { return {0}; }
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