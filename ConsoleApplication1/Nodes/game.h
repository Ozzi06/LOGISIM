#pragma once

#include "gui_ui.h"
#include <vector>
#include "random_id.h"

#include <utility>
#include "LogicBlocks.h"
#include "raylib.h"
#include "nlohmann/json.hpp"

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

class LogicBlock;

class Game {
private:
    // Private constructor to prevent instantiation
    Game() {
        camera = { 0 };
        regular = { 0 };
    }

    // Static instance of the class
    static Game instance;

private:
    std::unique_ptr<LogicBlock> logicblock = nullptr;
public:
    bool has_updated = true;
    bool run_on_block = false;
    template<typename T>
    T* get_logicblock(size_t offset) {
        return reinterpret_cast<T*>(logicblock->get_data(offset));
    }
    size_t logicblock_size() { return logicblock->get_size(); }
    void network_change() {
        run_on_block = false;
    }

    void build_logic_block();

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

    void add_function_node();
    void add_subassebly();

    void handle_input();

    void save_json(std::string filePath = "gamesave.json");

    void save_bin(std::string filePath = "gamesave.bin");


    bool hovering_above_gui = false;

    bool get_efficient_simulation() const { return efficient_simulation; }
    bool efficient_simulation = false;
private:
    bool area_selected = false;
    Vector2 first_corner = { 0,0 };
};

void NodeNetworkFromJson(const json& nodeNetworkJson, std::vector<Node*>* nodes);

void NodeNetworkFromBinary(std::filesystem::path filepath, std::vector<Node*>* nodes);

void NormalizeNodeNetworkPosToLocation(std::vector<Node*>& nodes, Vector2 targpos);
