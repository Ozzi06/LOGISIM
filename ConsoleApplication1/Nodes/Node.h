#pragma once
#include <vector>
#include "gui_ui.h"
#include "raylib.h"
#include "nlohmann/json.hpp"
#include <utility>
#include "LogicBlocks.h"
#include "random_id.h"

using json = nlohmann::json;

struct Output_connector;
struct Input_connector;

struct Node {
public:
    // Constructors and Destructor
    Node(std::vector<Node*>* container, Vector2 pos = { 0,0 }, Vector2 size = { 0,0 }, Color color = { 0,0,0 }, std::vector<Input_connector> in = {}, std::vector<Output_connector> out = {});
    Node(const Node* base);
    virtual ~Node() {}

    // Core Virtual Functions
    virtual Node* copy() const = 0;
    virtual Texture get_texture() const = 0;
    virtual void draw();
    virtual bool show_node_editor();
    virtual std::string get_type_str() const = 0;
    virtual NodeType get_type() const = 0;

    // Label Management
    virtual std::string get_label() const { return std::string(label); }
    virtual void change_label(const char* newlabel);

    // Input/Output Management
    virtual size_t get_max_outputs() const { return 32; }
    virtual void reserve_outputs() { outputs.reserve(get_max_outputs()); }
    virtual void add_input() = 0;
    virtual void remove_input() = 0;
    virtual Input_connector* select_input(Vector2 select_pos);
    virtual Output_connector* select_output(Vector2 select_pos);
    virtual std::vector<Input_connector*> select_inputs(Rectangle select_area);
    virtual std::vector<Output_connector*> select_outputs(Rectangle select_area);
    virtual std::vector<Input_connector*> connected_inputs(size_t output_idx);

    // State and Properties
    bool has_changed = true;
    bool is_selected;
    Vector2 pos;
    Vector2 size;
    const Color color;
    std::vector<Input_connector> inputs;
    std::vector<Output_connector> outputs;
    std::string label;

    // Interaction Handlers
    virtual void not_clicked() {}
    virtual void clicked(Vector2 pos) {}

    // Serialization
    virtual json to_JSON() const;
    void load_JSON(const json& nodeJson);
    void load_Bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr);
    virtual void load_extra_JSON(const json& nodeJson) {}
    virtual void load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr) {}

    // Node Properties
    virtual const std::vector<Node*>* get_children() const { return nullptr; }
    virtual bool isInput() const { return false; }
    virtual bool isOutput() const { return false; }
    virtual bool is_cyclic() const { return false; }
    virtual int delay() const { return 1; }

    // Container Management
    void move_to_container(std::vector<Node*>* new_container) { container = new_container; }
    const std::vector<Node*>* get_container() const { return container; }

    // Offset Management
    bool has_offset() const { return has_offset_val; }
    void set_abs_node_offset(offset new_node_offset) { abs_node_offset = new_node_offset; has_offset_val = true; }
    size_t get_abs_node_offset() const { return abs_node_offset; }

    // State Update
    void update_state_from_logicblock();

protected:
    // Size Computation
    virtual void recompute_size() {
        size = Vector2{ 100, std::max(100 + 30 * float(inputs.size()), 100 + 30 * float(outputs.size())) };
    }

    // Container
    std::vector<Node*>* container;

    // Offset
    size_t abs_node_offset = 0;
    bool has_offset_val = false;
};

struct Input_connector {
    Input_connector(Node* host, size_t index, std::string name, Output_connector* target = nullptr, uid_t target_id = 0) : host(host), target(target), index(index), name(name), target_id(target_id) {}
    Node* host;
    Output_connector* target;
    uid_t target_id;
    size_t index;
    std::string name;

    Vector2 get_connection_pos() const {
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
    Output_connector(Node* host, size_t index, std::string name, bool state = false, uid_t id = generate_id()) : host(host), index(index), state(state), new_state(false), name(name), id(id) { }
    Node* host;
    size_t index;
    std::string name;
    bool state;
    bool new_state;

    uid_t id;

    Vector2 get_connection_pos() const {
        const float width = 30.0f;
        const float spacing = 30.0f;

        float pos_y = host->pos.y + ((host->outputs.size() - 1) * spacing / 2.0f) - (index * spacing);
        float pos_x = host->pos.x + host->size.x / 2.0f + width;
        return Vector2{ pos_x, pos_y };
    }

    void draw() const;

    json to_JSON() const;
};