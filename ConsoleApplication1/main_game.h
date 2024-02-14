#pragma once
#include "gui_node_editor.h"
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

void DrawInputConnector(const Input_connector& conn);
void DrawOutputConnector(const Output_connector& conn);

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

    unsigned int screenWidth = 1920;
    unsigned int screenHeight = 1080;

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

private:
    bool area_selected = false;
    Vector2 first_corner = { 0,0 };
};

void NodeNetworkFromJson(const json& nodeNetworkJson, std::vector<Node*>& nodes);

void NormalizeNodeNetworkPosTocLocation(std::vector<Node*>& nodes, Vector2 targpos);

struct Node {
public:
    Node(Vector2 pos = { 0,0 }, Vector2 size = { 0,0 }, Color color = { 0,0,0 }, std::vector<Input_connector> in = {}, std::vector<Output_connector> out = {});
    
    Node(const Node* base);

    virtual size_t get_max_outputs() const { return 1024; }
    virtual void reserve_outputs() { outputs.reserve(get_max_outputs()); }

    virtual Node* copy() const = 0;

    virtual Texture get_texture() const = 0;

    virtual void draw();

    virtual std::string get_label() const = 0;

    GuiNodeEditorState editor_state;
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
    ~Node();

    virtual std::string get_type() const = 0;

    virtual json to_JSON() const;

    void load_JSON(const json& nodeJson);

    virtual void load_extra_JSON(const json& nodeJson) {}

    virtual bool isInput() const { return false; }
    virtual bool isOutput() const { return false; }

protected:
    virtual void recompute_size() {
        size = Vector2{ 100, std::max(100 + 30 * float(inputs.size()), 100 + 30 * float(outputs.size())) };
    }

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

    json to_JSON() const;
};

struct GateAND : public Node {
    GateAND(Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}): Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "AND";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size()));

        size = Vector2{ 100, 100 + 30 * float(inputs.size()) };
    }
    GateAND(const GateAND* base): Node(base) {}

    virtual void add_input() override {
        inputs.push_back(Input_connector(this, inputs.size())); recompute_size();
    }
    virtual void remove_input() override { if(inputs.size() > 1) inputs.pop_back();  recompute_size();
    }


    Node* copy() const override { return new GateAND(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateAND"; }

    //Node* from_json(const json& j);
};

struct GateOR : public Node {
    GateOR(Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}): Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "OR";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size()));

        size = Vector2{ 100, 100 + 30 * float(inputs.size()) };
    }
    GateOR(const GateOR* base) : Node(base) {}

    virtual void add_input() override {
        inputs.push_back(Input_connector(this, inputs.size())); recompute_size();
    }
    virtual void remove_input() override {
        if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    }

    Node* copy() const override { return new GateOR(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateOR"; }
};

struct GateNAND : public Node {
    GateNAND(Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}): Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "NAND";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
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

struct GateNOR : public Node {
    GateNOR(Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}): Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "NOR";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size()));

        size = Vector2{ 100, 100 + 30 * float(inputs.size()) };
    }
    GateNOR(const GateNOR* base) : Node(base) {}

    virtual void add_input() override {
        inputs.push_back(Input_connector(this, inputs.size())); recompute_size();
    }
    virtual void remove_input() override {
        if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    }

    Node* copy() const override { return new GateNOR(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateNOR"; }
};

struct GateXOR : public Node {
    GateXOR(Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}): Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "XOR";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size()));

        size = Vector2{ 100, 100 + 30 * float(inputs.size()) };
    }
    GateXOR(const GateXOR* base) : Node(base) {}

    virtual void add_input() override {
        inputs.push_back(Input_connector(this, inputs.size())); recompute_size();
    }
    virtual void remove_input() override {
        if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    }

    Node* copy() const override { return new GateXOR(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateXOR"; }
};

struct GateXNOR : public Node {
    GateXNOR(Vector2 pos = { 0,0 }, size_t input_count = 2, std::vector<Input_connector> input_connectors = {}): Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "XNOR";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
        inputs.insert(inputs.end(), input_connectors.begin(), input_connectors.end());

        while (inputs.size() < input_count)
            inputs.push_back(Input_connector(this, inputs.size()));

        size = Vector2{ 100, 100 + 30 * float(inputs.size()) };
    }
    GateXNOR(const GateXNOR* base) : Node(base) {}

    virtual void add_input() override {
        inputs.push_back(Input_connector(this, inputs.size())); recompute_size();
    }
    virtual void remove_input() override {
        if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    }

    Node* copy() const override { return new GateXNOR(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;

    virtual Texture get_texture() const override { return texture; }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateXNOR"; }
};

struct GateBUFFER : public Node {
    GateBUFFER(Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "BUFFER";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
        
        inputs.push_back(Input_connector(this, 0, input));

        size = Vector2{ 100, 100 + 50 * 1 };

    }
    GateBUFFER(const GateBUFFER* base): Node(base) {}

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

    Node* copy() const override { return new GateBUFFER(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;
    virtual Texture get_texture() const override {
        return texture;
    }

    virtual void pretick() override;

    virtual std::string get_type() const override { return"GateBUFFER"; }
};

struct GateNOT : public Node {
    GateNOT(Vector2 pos = { 0,0 }, Output_connector* input = nullptr) : Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "NOT";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
        inputs.push_back(Input_connector(this, 0, input));

        size = Vector2{ 100, 100 + 50 * 1 };

    }

    GateNOT(const GateNOT* base) : Node(base) {}

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

    Node* copy() const override { return new GateNOT(this); }

    virtual std::string get_label() const override { return std::string(label); }

    static Texture texture;
    virtual Texture get_texture() const override {
        return texture;
    }

    virtual void pretick();

    virtual std::string get_type() const override { return"GateNOT"; }
};

struct PushButton : public Node {
    virtual Rectangle getButtonRect(size_t id);
    virtual void draw() override;

    PushButton(Vector2 pos = { 0,0 }) : Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "Push Button";
        recompute_size();
    }
    PushButton(const PushButton* base) : Node(base) {}

    virtual void add_input() override {
        if (outputs.size() == outputs.capacity()) return;
        outputs.push_back(Output_connector(this, outputs.size())); recompute_size();
    }
    virtual void remove_input() override {
        if (outputs.size() > 1) {
            Game& game = Game::getInstance();
            for (Node* node : game.nodes) {
                for (auto& conn:node->inputs) {
                    if (conn.target == &outputs.back()) conn.target = nullptr;
                }
            }
            outputs.pop_back();  recompute_size();
        }
    }

    Node* copy() const override { return new PushButton(this); }

    virtual std::string get_label() const override { return std::string(label); }


    virtual Texture get_texture() const override { return{ 0 }; }

    virtual void not_clicked() override {
        for (size_t i = 0; i < outputs.size(); i++) {
            outputs[i].state = false;
        }
    }

    virtual void clicked(Vector2 pos) override {
        for (size_t i = 0; i < outputs.size(); i++) {
            if(CheckCollisionPointRec(pos, getButtonRect(i))) outputs[i].state = true;
            else outputs[i].state = false;
        }
    }

    virtual void pretick() override {}
    virtual void tick() override {}

    virtual std::string get_type() const override { return"PushButton"; }

    virtual bool isInput() const override { return true; }

    virtual void recompute_size() override;
};

struct ToggleButton : public Node {
    virtual Rectangle getButtonRect(size_t id);
    virtual void draw() override;

    ToggleButton(Vector2 pos = { 0,0 }) : Node(pos, { 0, 0 }, ColorBrightness(BLUE, -0.4f)) {
        label = "Toggle Button";

        recompute_size();
        outputs[0].state = false;
    }
    ToggleButton(const ToggleButton* base) : Node(base) {
        for (size_t i = 0; i < outputs.size(); i++) {
            outputs[i].state = base->outputs[i].state;
        }
    }

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

    Node* copy() const override { return new ToggleButton(this); }

    virtual std::string get_label() const override { return std::string(label); }


    virtual Texture get_texture() const override { return{ 0 }; }

    virtual void not_clicked() override {}

    virtual void clicked(Vector2 pos) override {
        for (size_t i = 0; i < outputs.size(); i++) {
            if (CheckCollisionPointRec(pos, getButtonRect(i))) outputs[i].state = !outputs[i].state;
        }
    }

    virtual void pretick() {}
    virtual void tick() override {}

    //virtual json to_JSON() const override;

    //virtual void load_extra_JSON(const json& nodeJson) override;

    virtual std::string get_type() const override { return"ToggleButton"; }

    virtual bool isInput() const { return true; }

    virtual void recompute_size() override;
};

struct StaticToggleButton : public ToggleButton {

    StaticToggleButton(Vector2 pos = { 0,0 }) : ToggleButton(pos) {
        label = "Static Toggle Button";
    }
    StaticToggleButton(const StaticToggleButton* base) : ToggleButton(base) {
    }

    Node* copy() const override { return new StaticToggleButton(this); }

    virtual std::string get_type() const override { return"StaticToggleButton"; }

    virtual bool isInput() const override { return false; }
};

struct LightBulb : public Node {
    LightBulb(Vector2 pos = { 0,0 }) : Node(pos, { 0, 0 }, YELLOW) {
        label = "Light Bulb";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());

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

    virtual Texture get_texture() const override { return {0}; }

    virtual void pretick() override {}

    virtual std::string get_type() const override { return"LightBulb"; }

    virtual bool isOutput() const override { return true; }
};

struct SevenSegmentDisplay : public Node {
    SevenSegmentDisplay(Vector2 pos = { 0,0 }) : Node(pos, { 0, 0 }, ColorBrightness(GRAY, -0.7f)) {
        label = "7-Segment Disp";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());

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

struct FunctionInput_node : public Node {

    FunctionInput_node(Vector2 pos = { 0,0 }) : Node(pos, { 0, 0 }, BLUE) {
        label = "Push Button";

        size = Vector2{ 200, 200 };
    }
    FunctionInput_node(const FunctionInput_node* base) : Node(base) {}

    virtual void add_input() override {}
    virtual void remove_input() override {}

    Node* copy() const override { return new FunctionInput_node(this); }

    virtual std::string get_label() const override { return std::string(label); }


    virtual Texture get_texture() const override { return{ 0 }; }

    virtual void pretick() override {}
    virtual void tick() override {}

    virtual std::string get_type() const override { return"FunctionInput_node"; }

    virtual bool isInput() const override { return true; }
};

class FunctionNode : public Node {
public:
    FunctionNode(Vector2 pos = {0, 0}) : Node(pos, {0, 0}, GRAY) {
        label = "Function";
        strcpy_s(editor_state.TextBoxNodeLabel, 256, label.c_str());
    }

    FunctionNode(const FunctionNode* base);

    ~FunctionNode() {
        for (auto& node : nodes) {
            delete node;
        }
    }


    virtual void draw() override;

    virtual void add_input() override {}
    virtual void remove_input() override {}

    Node* copy() const override { return new FunctionNode(this); }

    virtual std::string get_label() const override { return std::string(label); }

    virtual Texture get_texture() const override { return {0}; }


    std::vector<Node*> nodes;
    
    std::vector<Node*> input_targs;
    std::vector<Node*> output_targs;
     

    virtual json to_JSON() const override;

    virtual void load_extra_JSON(const json& nodeJson) override;

    virtual void pretick() override;

    virtual void tick() override;
    
    virtual std::string get_type() const override { return"FunctionNode"; }

protected:
    virtual void recompute_size() override;
};

class NodeFactory {
public:
    static Node* createNode(const std::string& nodeName) {
        static std::unordered_map<std::string, std::function<Node* ()>> factoryMap = {
            {"GateAND", []() -> Node* { return new GateAND(); }},
            {"GateOR", []() -> Node* { return new GateOR(); }},
            {"GateNAND", []() -> Node* { return new GateNAND(); }},
            {"GateNOR", []() -> Node* { return new GateNOR(); }},
            {"GateXOR", []() -> Node* { return new GateXOR(); }},
            {"GateXNOR", []() -> Node* { return new GateXNOR(); }},
            {"GateBUFFER", []() -> Node* { return new GateBUFFER(); }},
            {"GateNOT", []() -> Node* { return new GateNOT(); }},
            {"PushButton", []() -> Node* { return new PushButton(); }},
            {"ToggleButton", []() -> Node* { return new ToggleButton(); }},
            {"StaticToggleButton", []() -> Node* { return new StaticToggleButton(); }},
            {"LightBulb", []() -> Node* { return new LightBulb(); }},
            {"SevenSegmentDisplay", []() -> Node* { return new SevenSegmentDisplay(); }},
            {"FunctionNode", []() -> Node* { return new FunctionNode(); }},
        };

        auto it = factoryMap.find(nodeName);
        if (it != factoryMap.end()) {
            return it->second(); // Instantiate the node
        }


        return nullptr; // Return nullptr if nodeName is not found
    }
};