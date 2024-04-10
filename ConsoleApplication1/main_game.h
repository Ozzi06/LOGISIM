#pragma once
#include "gui_ui.h"
#include <vector>
#include "random_id.h"

#include "nlohmann/json.hpp"
#include <utility>

using json = nlohmann::json;

struct Output_connector;
struct Input_connector;
struct Node;
class Game;

struct GuiNodeEditorState;

enum EditMode {
    SELECT,
    EDIT,
    CONNECT,
    INTERACT,
};

class Game {
private:
    // Private constructor to prevent instantiation
    Game() {
        camera = { 0 };
        regular = { 0 };
    }

    // Static instance of the class
    static Game instance;
public:
    // Public function to get the instance
    static Game& getInstance() {
        return instance;
    }

    Camera2D camera;

    unsigned int screenWidth = 500;
    unsigned int screenHeight = 500;

    Font regular;
    EditMode edit_mode = EDIT;

    bool sim_playing = false;
    bool warp = false;
    float targ_sim_hz = 16;
    float real_sim_hz = 0;
    

    std::vector<Node*> nodes;
    std::vector<Node*> clipboard;

    std::vector< Input_connector* >selected_inputs;
    std::vector< Output_connector* >selected_outputs;

    void draw();

    void pretick();

    void tick();

    void unselect_all();

    void remove_node(Node* node);
    void delete_selected_nodes();
    void copy_selected_nodes();
    void paste_nodes();

    void handle_input();

    void save(std::string filePath = "gamesave.json");
    void load(std::string filePath = "gamesave.json");

    bool hovering_above_gui = false;

    bool get_efficient_simulation() const { return efficient_simulation; }
    bool efficient_simulation = false;

private:
    bool area_selected = false;
    Vector2 first_corner = { 0,0 };

};

void NodeNetworkFromJson(const json& nodeNetworkJson, std::vector<Node*>& nodes);

void NormalizeNodeNetworkPosTocLocation(std::vector<Node*>& nodes, Vector2 targpos);

struct Node {
public:
    Node(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Vector2 size = { 0,0 }, Color color = { 0,0,0 }, std::vector<Input_connector> in = {}, std::vector<Output_connector> out = {});
    
    Node(const Node* base);

    virtual ~Node() {}

    virtual size_t get_max_outputs() const { return 32; }
    virtual void reserve_outputs() { outputs.reserve(get_max_outputs()); }

    virtual Node* copy() const = 0;

    virtual Texture get_texture() const = 0;

    virtual void draw();

    virtual bool show_node_editor();

    virtual std::string get_label() const { return std::string(label); }
    virtual void change_label(const char* newlabel) {
        label = newlabel;
    }

    bool has_changed = true;

    bool is_selected;
    Vector2 pos;
    Vector2 size;
    const Color color;

    std::vector<Input_connector> inputs;
    std::vector<Output_connector> outputs;

    std::string label;

    virtual Input_connector* select_input(Vector2 select_pos);
    virtual Output_connector* select_output(Vector2 select_pos);

    virtual std::vector<Input_connector*> select_inputs(Rectangle select_area);
    virtual std::vector<Output_connector*> select_outputs(Rectangle select_area);


    virtual void not_clicked() {}
    virtual void clicked(Vector2 pos) {}

    virtual void add_input() = 0;
    virtual void remove_input() = 0;

    virtual void pretick() = 0;
    virtual void tick();

    virtual std::string get_type() const = 0;

    virtual json to_JSON() const;

    void load_JSON(const json& nodeJson);

    virtual void load_extra_JSON(const json& nodeJson) {}

    virtual bool isInput() const { return false; }
    virtual bool isOutput() const { return false; }

    virtual bool is_cyclic() const { return false; }
    virtual int delay() const { return 1; }

    virtual std::vector<Input_connector*> connected_inputs(size_t output_idx) {
        std::vector<Input_connector*> input_nodes;
        for (size_t i = 0; i < inputs.size(); i++) {
            input_nodes.push_back(&inputs[i]);
        }
        return input_nodes;
    }

    void move_to_container(std::vector<Node*> * new_container) {
        container = new_container;
    }

protected:
    virtual void recompute_size() {
        size = Vector2{ 100, std::max(100 + 30 * float(inputs.size()), 100 + 30 * float(outputs.size())) };
    }
    std::vector<Node*> * container;

};

struct Input_connector {
    Input_connector(Node* host, size_t index, Output_connector* target = nullptr, uid_t target_id = 0) : host(host), target(target), index(index), target_id(target_id) {}
    Node* host;
    Output_connector* target;
    uid_t target_id;
    size_t index;

    Vector2 get_connection_pos() const{
        const float width = 30;
        float spacing = 30;

        float pos_y = host->pos.y + ((host->inputs.size() - 1) * spacing / 2.0f) - (index * spacing);
        float pos_x = host->pos.x - host->size.x / 2.0f - width;
        return Vector2{ pos_x, pos_y };
    }

    void draw() const;

    json to_JSON() const;
};

struct Output_connector {
    Output_connector(Node* host, size_t index, bool state = false, uid_t id = generate_id()) : host(host), index(index), state(state), new_state(false), id(id) { }
    Node* host;
    size_t index;
    bool state;
    bool new_state;

    uid_t id;

    Vector2 get_connection_pos() const {
        const float width = 30.0f;
        const float spacing = 30.0f;

        float pos_y = host->pos.y + ((host->outputs.size() - 1) * spacing / 2.0f) - (index * spacing);
        float pos_x = host->pos.x + host->size.x / 2.0f + width;
        return Vector2{pos_x, pos_y};
    }

    void draw() const;

    json to_JSON() const;
};

struct BinaryLogicGate : public Node {
    BinaryLogicGate(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size()));

        recompute_size();
    }
    BinaryLogicGate(const BinaryLogicGate* base) : Node(base) {}

    virtual void add_input() override {
        inputs.push_back(Input_connector(this, inputs.size())); recompute_size();
    }

    virtual void remove_input() override {
        if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    }
};

struct GateAND : public BinaryLogicGate {
    GateAND(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "AND";
    }
    GateAND(const GateAND* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateAND(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateAND"; }
};

struct GateOR : public BinaryLogicGate {
    GateOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "OR";
    }
    GateOR(const GateOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateOR"; }
};

struct GateNAND : public Node {
    GateNAND(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "NAND";
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size()));

        size = Vector2{ 100, 100 + 30 * float(inputs.size()) };
    }
    GateNAND(const GateNAND* base) : Node(base) {}

    virtual void add_input() override {
        inputs.push_back(Input_connector(this, inputs.size())); recompute_size();
    }
    virtual void remove_input() override {
        if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    }

    Node* copy() const override { return new GateNAND(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateNAND"; }
};

struct GateNOR : public BinaryLogicGate {
    GateNOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "NOR";
    }
    GateNOR(const GateNOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateNOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateNOR"; }
};

struct GateXOR : public BinaryLogicGate {
    GateXOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "XOR";
    }
    GateXOR(const GateXOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateXOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateXOR"; }
};

struct GateXNOR : public BinaryLogicGate {
    GateXNOR(std::vector<Node*> * container, Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}) : BinaryLogicGate(container, pos, input_count, input_connectors) {
        label = "XNOR";
    }
    GateXNOR(const GateXNOR* base) : BinaryLogicGate(base) {}

    Node* copy() const override { return new GateXNOR(this); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateXNOR"; }
};

struct UnaryLogicGate : public Node {
    UnaryLogicGate(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        inputs.push_back(Input_connector(this, 0, input));
        recompute_size();
    }
    UnaryLogicGate(const UnaryLogicGate* base) : Node(base) {}


    virtual void add_input() override {
        if (outputs.size() == outputs.capacity()) return;
        inputs.push_back(Input_connector(this, inputs.size()));
        outputs.push_back(Output_connector(this, outputs.size(), false));
        recompute_size();
    }
    virtual void remove_input() override {
        Game& game = Game::getInstance();
        if (inputs.size() > 1) {
            inputs.pop_back();
            for (Node* node : game.nodes) {
                for (Input_connector& input : node->inputs) {
                    if (input.target == &outputs.back()) input.target = nullptr;
                }
            }
            outputs.pop_back();
        }
        recompute_size();
    }


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

    virtual void pretick() override;

    virtual std::string get_type() const override { return"GateBUFFER"; }
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

    virtual void pretick() override;

    virtual std::string get_type() const override { return"GateNOT"; }
};

struct Bus : public Node {
    Bus(std::vector<Node*> * container, Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(container, pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "BUS_0";
        inputs.push_back(Input_connector(this, 0, input));
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

    virtual void add_input() override {
        if (outputs.size() == outputs.capacity()) return;
        inputs.push_back(Input_connector(this, inputs.size()));
        outputs.push_back(Output_connector(this, outputs.size(), false));
        recompute_size();
        find_connections();


    }

    virtual void remove_input() override {
        Game& game = Game::getInstance();
        if (inputs.size() > 1) {
            inputs.pop_back();
            for (Node* node : game.nodes) {
                for (Input_connector& input : node->inputs) {
                    if (input.target == &outputs.back()) input.target = nullptr;
                }
            }
            outputs.pop_back();
        }
        recompute_size();
        find_connections();
    }

    virtual void change_label(const char* newlabel) override {
        label = newlabel;
        find_connections();
    }

    Node* copy() const override { return new Bus(this); }

    virtual json to_JSON() const override;

    virtual void load_extra_JSON(const json& nodeJson) override;

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return{ 0 }; }

    virtual void pretick() override;
    virtual void tick() override;

    virtual std::string get_type() const override { return"Bus"; }

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

    virtual void add_input() override {
        if (outputs.size() == outputs.capacity()) return;
        outputs.push_back(Output_connector(this, outputs.size())); recompute_size();
    }

    virtual void remove_input() override {
        if (outputs.size() > 1) {
            Game& game = Game::getInstance();
            for (Node* node : game.nodes) {
                for (auto& conn : node->inputs) {
                    if (conn.target == &outputs.back()) conn.target = nullptr;
                }
            }
            outputs.pop_back();  recompute_size();
        }
    }

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return{ 0 }; }

    virtual void not_clicked() = 0;
    virtual void clicked(Vector2 pos) = 0;

    virtual void pretick() override {}
    virtual void tick() override { has_changed = false; }

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

    virtual std::string get_type() const override { return"PushButton"; }
};

struct ToggleButton : public Button {
    ToggleButton(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : Button(container, pos) {
        label = "Toggle Button";
    }
    ToggleButton(const ToggleButton* base) : Button(base) {}

    Node* copy() const override { return new ToggleButton(this); }

    virtual void not_clicked() override {}

    virtual void clicked(Vector2 pos) override;

    virtual std::string get_type() const override { return"ToggleButton"; }
};

struct StaticToggleButton : public ToggleButton {

    StaticToggleButton(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : ToggleButton(container, pos) {
        label = "Static Toggle Button";
    }
    StaticToggleButton(const StaticToggleButton* base) : ToggleButton(base) {
    }

    Node* copy() const override { return new StaticToggleButton(this); }

    virtual std::string get_type() const override { return"StaticToggleButton"; }

    virtual bool isInput() const override { return false; }
};

struct LightBulb : public Node {
    LightBulb(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : Node(container, pos, { 0, 0 }, YELLOW) {
        label = "Light Bulb";

        size = Vector2{ 100, 100 };
        outputs.clear();
        inputs.clear();
        inputs.push_back(Input_connector(this, 0));
    }
    LightBulb(const LightBulb* base) : Node(base) {}

    virtual void draw() override;

    virtual void add_input() override {}
    virtual void remove_input() override {}

    Node* copy() const override { return new LightBulb(this); }

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return { 0 }; }

    virtual void pretick() override { has_changed = false; }

    virtual std::string get_type() const override { return"LightBulb"; }

    virtual bool isOutput() const override { return true; }
};

struct SevenSegmentDisplay : public Node {
    SevenSegmentDisplay(std::vector<Node*> * container, Vector2 pos = { 0,0 }) : Node(container, pos, { 0, 0 }, ColorBrightness(GRAY, -0.7f)) {
        label = "7-Segment Disp";

        outputs.clear();
        inputs.clear();
        for(size_t i = 0; i < 7; i++)
            inputs.push_back(Input_connector(this, i));
        recompute_size();
    }
    SevenSegmentDisplay(const SevenSegmentDisplay* base) : Node(base) {}

    virtual void draw() override;

    virtual void add_input() override {}
    virtual void remove_input() override {}

    Node* copy() const override { return new SevenSegmentDisplay(this); }

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return { 0 }; }

    virtual void pretick() override {}

    virtual std::string get_type() const override { return"SevenSegmentDisplay"; }

    virtual bool isOutput() const override { return false; }

protected:
    virtual void recompute_size() override;
};

class FunctionNode : public Node {
public:
    
    FunctionNode(std::vector<Node*> * container, Vector2 pos = {0, 0}) : Node(container, pos, {0, 0}, ColorBrightness(GRAY, -0.6f)), is_single_tick(false), is_cyclic_val(true) {
        label = "Function";
    }

    FunctionNode(const FunctionNode* base);

    ~FunctionNode();

    virtual void add_input() override {}
    virtual void remove_input() override {}

    Node* copy() const override { return new FunctionNode(this); }

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return {0}; }

    virtual bool show_node_editor();

    std::vector<Node*> nodes;
    
    std::vector<Node*> input_targs;
    std::vector<Node*> output_targs;
     

    virtual json to_JSON() const override;

    virtual void load_extra_JSON(const json& nodeJson) override;

    virtual void draw() override;

    virtual void pretick() override;

    virtual void tick() override;

    virtual bool is_cyclic() const override;
    virtual int delay() const override;

    void sort_linear();
    
    virtual std::string get_type() const override { return"FunctionNode"; }

protected:
    virtual void recompute_size() override;
    
private:
    bool is_single_tick;
    bool is_cyclic_val;
    std::string delay_str;
};

class NodeFactory {
public:
    static Node* createNode(std::vector<Node*> * container, const std::string& nodeName) {
        static std::unordered_map<std::string, std::function<Node* ()>> factoryMap = {
            {"GateAND", [&container]() -> Node* { return new GateAND(container); }},
            {"GateOR", [&container]() -> Node* { return new GateOR(container); }},
            {"GateNAND", [&container]() -> Node* { return new GateNAND(container); }},
            {"GateNOR", [&container]() -> Node* { return new GateNOR(container); }},
            {"GateXOR", [&container]() -> Node* { return new GateXOR(container); }},
            {"GateXNOR", [&container]() -> Node* { return new GateXNOR(container); }},
            {"GateBUFFER", [&container]() -> Node* { return new GateBUFFER(container); }},
            {"GateNOT", [&container]() -> Node* { return new GateNOT(container); }},
            {"PushButton", [&container]() -> Node* { return new PushButton(container); }},
            {"ToggleButton", [&container]() -> Node* { return new ToggleButton(container); }},
            {"StaticToggleButton", [&container]() -> Node* { return new StaticToggleButton(container); }},
            {"LightBulb", [&container]() -> Node* { return new LightBulb(container); }},
            {"SevenSegmentDisplay", [&container]() -> Node* { return new SevenSegmentDisplay(container); }},
            {"FunctionNode", [&container]() -> Node* { return new FunctionNode(container); }},
            {"Bus", [&container]() -> Node* { return new Bus(container); }},
        };

        auto it = factoryMap.find(nodeName);
        if (it != factoryMap.end()) {
            return it->second(); // Instantiate the node
        }


        return nullptr; // Return nullptr if nodeName is not found
    }
};