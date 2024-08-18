#include "main_game.h"

#include "vector_tools.h"
#include <fstream>
#include <cassert>

#include <iostream>

#include <algorithm>
#include "vector_tools.h"
#include "raygui.h"
#include "save_game.h"
#include "file_dialogs.h"

Node::Node(std::vector<Node*> * container, Vector2 pos, Vector2 size, Color color, std::vector<Input_connector> in, std::vector<Output_connector> out) : container(container), size(size), color(color), is_selected(false), inputs(in), outputs(out), pos(pos)
{
    reserve_outputs();
    if (outputs.size() == 0)
        outputs.push_back(*new Output_connector(this, 0, ""));
}

Node::Node(const Node* base) : container(base->container), is_selected(false), pos(base->pos), 
                        size(base->size), color(base->color), label(base->label)
{
    reserve_outputs();
    for (size_t i = 0; i < base->inputs.size(); ++i) {
        inputs.push_back(Input_connector(this, i, "", base->inputs[i].target));
    }
    for (size_t i = 0; i < base->outputs.size(); ++i) {
        outputs.push_back(Output_connector(this, i, "", base->outputs[i].state));
    }
}

void Game::draw() {
    // Draw world
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();
    
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(camera);

    if (run_on_block && logicblock) {
        for (Node* node : nodes) {
            node->update_state_from_logicblock();
        }
    }

    for (Node* node : nodes) {
        node->draw();
    }

    if (area_selected) {
        Rectangle area = RectFrom2Points(GetScreenToWorld2D(GetMousePosition(), camera), first_corner);
        DrawRectangleRec(area, Fade(GREEN, 0.05f));
    }

    EndMode2D();

    std::string scientific_number = num_toString(real_sim_hz, 1);
    DrawText(scientific_number.c_str(), 10, 100, 20, WHITE);


    hovering_above_gui = false;

    if (GuiUi()) hovering_above_gui = true;

    if (edit_mode == EDIT) {
        for (Node* node : nodes) {
            if (node->is_selected) {
                if (node->show_node_editor()) hovering_above_gui = true;
            }
        }
    }

    
    EndDrawing();
    if (!hovering_above_gui) {
        handle_input();
    }
}

void Game::pretick()
{
    if (!efficient_simulation) has_updated = true;
    if (!run_on_block) { 
        build_logic_block();
        run_on_block = true;
    }
    if (run_on_block) {
        if (logicblock) {
            logicblock->pretick(logicblock->get_data(0), has_updated, 0);
            return;
        }
        else { 
            run_on_block = false; 
            std::cout << "no serialized block, turning off run on block\n";
        }
    }
    for (Node* node : nodes) {
        node->pretick();
    }
}

void Game::tick()
{
    if (run_on_block) {
        if (logicblock) {
            logicblock->tick(logicblock->get_data(0), has_updated, 0);
        }
        else run_on_block = false;
    }
    else {
        for (Node* node : nodes) {
            node->tick();
        }
    }
    has_updated = false;
}

void Game::unselect_all()
{
    for (Node* node : nodes) {
        node->is_selected = false;
    }
    selected_inputs.clear();
    selected_outputs.clear();
}

void Game::remove_node(Node* node)
{
    nodes.erase(std::remove(nodes.begin(), nodes.end(), node), nodes.end());

    for (Node* gate : nodes) {
        for (Input_connector& inp : gate->inputs) {
            for (Output_connector& out : node->outputs) {
                if (inp.target == &out) {
                    inp.target = nullptr;
                }
            }

        }
    }
}

void Game::delete_selected_nodes() {

    // Delete selected objects and set their pointers to nullptr
    for (Node*& node : nodes) {
        if (node != nullptr && node->is_selected) {

            for (Node* gate : nodes) {
                if (gate == nullptr) continue;
                for (Input_connector& inp : gate->inputs) {
                    for (Output_connector& out : node->outputs) {
                        if (inp.target == &out) {
                            inp.target = nullptr;
                        }
                    }

                }
            }

            delete node;
            node = nullptr;
        }
    }

    // Remove nullptrs from the vector
    nodes.erase(std::remove(nodes.begin(), nodes.end(), nullptr), nodes.end());
}

void Game::copy_selected_nodes()
{
    for (Node* node : clipboard) {
        delete node;
    }
    clipboard.clear();

    size_t* idxs = new size_t[nodes.size()];

    size_t idx = 0;
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i]->is_selected) {
            clipboard.push_back(nodes[i]->copy());
            idxs[i] = idx;
            ++idx;
        }
    }

    for (size_t i = 0; i < clipboard.size(); ++i) {

        for (Input_connector& input : clipboard[i]->inputs) {
            if (input.target && input.target->host->is_selected) {

                size_t target_idx = -1;
                for (size_t p = 0; p < input.target->host->outputs.size(); ++p) {
                    if (&input.target->host->outputs[p] == input.target) target_idx = p;
                }

                assert(target_idx != -1);

                for (size_t j = 0; j < nodes.size(); ++j) {

                    if (nodes[j] == input.target->host) {
                        input.target = &(clipboard[idxs[j]]->outputs[target_idx]);
                    }
                }

            }
        }


    }
    delete[] idxs;
}

void Game::paste_nodes()
{

    size_t i = nodes.size();

    nodes.insert(nodes.end(), clipboard.begin(), clipboard.end());
    clipboard.clear();

    unselect_all();

    for (; i < nodes.size(); ++i) {
        nodes[i]->is_selected = true;
    }

    copy_selected_nodes();
}

void Game::add_subassebly()
{
    std::filesystem::path filepath = open_file_dialog_json_bin();
    if (!(filepath.extension() == ".bin" || filepath.extension() == ".json")) {
        std::cerr << "invalid file extension selected";
        return;
    }

    std::vector<Node*> subassembly;
    if (filepath.extension() == ".bin") {
        NodeNetworkFromBinary(filepath, &subassembly);
    }
    else if (filepath.extension() == ".json") {
        std::ifstream saveFile;
        saveFile.open(filepath);
        if (saveFile.is_open()) {
            json save;
            saveFile >> save;
            saveFile.close();
            NodeNetworkFromJson(save.at("nodes"), &subassembly);
        }
    }
        
    NormalizeNodeNetworkPosToLocation(subassembly, camera.target);
        
    for (Node* node : subassembly) {
        node->move_to_container(&nodes);
    }
        
    nodes.insert(nodes.end(), subassembly.begin(), subassembly.end());
    network_change();
}

void Game::handle_input()
{
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        Vector2 delta = GetMouseDelta();
        delta = Vector2Scale(delta, -1.0f / camera.zoom);

        camera.target = Vector2Add(camera.target, delta);
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0)
    {
        Vector2 mouseWorldPos = GetScreenToWorld2D(GetMousePosition(), camera);
        camera.offset = GetMousePosition();
        // Set the target to match, so that the camera maps the world space point 
        // under the cursor to the screen space point under the cursor at any zoom
        camera.target = mouseWorldPos;

        const float zoomIncrement = 0.2f;
        camera.zoom += (wheel * zoomIncrement * camera.zoom);
        if (camera.zoom > 14.0f)
            camera.zoom = 14.0f;

        if (camera.zoom < 1 / 256.0f)
            camera.zoom = 1 / 256.0f;
    }
    

    switch (edit_mode)
    {
    case SELECT:
    {
        static bool moving_nodes;
        static Vector2 movement = { 0, 0 };
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            first_corner = GetScreenToWorld2D(GetMousePosition(), camera);

            area_selected = false;
            if (IsKeyDown(KEY_LEFT_SHIFT))
                area_selected = true;
        }
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (abs(GetMousePosition() - GetWorldToScreen2D(first_corner, camera)) > 5) {
                moving_nodes = true;
            }

            if (moving_nodes && !area_selected) {

                for (Node* node : nodes) {
                    if (node->is_selected) {
                        node->pos = node->pos - movement;
                    }
                }
                if (IsKeyDown(KEY_LEFT_SHIFT)) {
                    Vector2 delta = GetScreenToWorld2D(GetMousePosition(), camera) - first_corner;
                    if (std::abs(delta.y) > std::abs(delta.x))
                        movement = { 0, delta.y };
                    else
                        movement = { delta.x, 0 };
                }
                else
                    movement = GetScreenToWorld2D(GetMousePosition(), camera) - first_corner;

                for (Node* node : nodes) {
                    if (node->is_selected) {
                        node->pos = node->pos + movement;
                    }
                }
            }
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (!moving_nodes) {

                if (!IsKeyDown(KEY_LEFT_CONTROL)) {
                    for (Node* node : nodes) {
                        node->is_selected = false;
                    }
                }
                for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
                    Node* node = *it;
                    if (CheckCollisionPointRec(first_corner, { node->pos.x - node->size.x / 2.0f - 10, node->pos.y - node->size.y / 2.0f - 10, node->size.x + 20, node->size.y + 20 })) {
                        node->is_selected = true;
                        auto forwit = it.base(); forwit--;
                        moveToBottom(nodes, forwit);
                        break;
                    }
                }
            }
            else if (area_selected) {
                if (!IsKeyDown(KEY_LEFT_CONTROL)) {
                    for (Node* node : nodes) {
                        node->is_selected = false;
                    }
                }
                Rectangle area = RectFrom2Points(GetScreenToWorld2D(GetMousePosition(), camera), first_corner);
                for (auto node:nodes) {

                    if (CheckCollisionRecs({ node->pos.x - node->size.x / 2.0f - 10, node->pos.y - node->size.y / 2.0f - 10, node->size.x + 20, node->size.y + 20 }, area)) {
                        node->is_selected = true;
                    }
                }
            }

            moving_nodes = false;
            movement = { 0, 0 };
        }

        if (IsKeyDown(KEY_LEFT_CONTROL)) {
            if (IsKeyPressed(KEY_C)) {
                copy_selected_nodes();
            }
            else if (IsKeyPressed(KEY_V)) {
                paste_nodes();
                network_change();
            }
        }

        if (IsKeyDown(KEY_DELETE)) {
            delete_selected_nodes();
            network_change();
        }

        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                area_selected = false;
        }

        break;
    }
        

    case EDIT:
    {
        static Vector2 press_pos;
        static bool moved_mouse;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            press_pos = GetScreenToWorld2D(GetMousePosition(), camera);
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (abs(GetMousePosition() - GetWorldToScreen2D(press_pos, camera)) > 5)
                moved_mouse = true;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (!moved_mouse) {

                for (Node* node : nodes) {
                    node->is_selected = false;
                }
                for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
                    Node* node = *it;
                    if (CheckCollisionPointRec(press_pos, { node->pos.x - node->size.x / 2.0f - 10, node->pos.y - node->size.y / 2.0f - 10, node->size.x + 20, node->size.y + 20 })) {
                        node->is_selected = true; 
                        auto forwit = it.base(); forwit--;
                        moveToBottom(nodes, forwit);
                        break;
                    }
                }
            }
            moved_mouse = false;
        }
        break;
    }

    case CONNECT:
    {
        static bool moved_mouse;

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            first_corner = GetScreenToWorld2D(GetMousePosition(), camera);

            area_selected = false;
            if (IsKeyDown(KEY_LEFT_SHIFT))
                area_selected = true;
        }

        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (abs(GetMousePosition() - GetWorldToScreen2D(first_corner, camera)) > 5)
                moved_mouse = true;
        }

        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            if (!IsKeyDown(KEY_LEFT_CONTROL)) {
                selected_inputs.clear();
                selected_outputs.clear();
            }

            if (!moved_mouse) {
                bool did_connect = false;

                for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
                    Node* node = *it;

                    Output_connector* outcon = node->select_output(GetScreenToWorld2D(GetMousePosition(), camera));
                    if (outcon) {
                        selected_outputs.push_back(outcon);
                        break;
                    }

                    Input_connector* incon = node->select_input(GetScreenToWorld2D(GetMousePosition(), camera));
                    if (incon) {
                        selected_inputs.push_back(incon);
                        break;
                    }
                }
            }

            else if (area_selected) {

                Rectangle area = RectFrom2Points(GetScreenToWorld2D(GetMousePosition(), camera), first_corner);
                for (Node* node : nodes) {
                    auto selout = node->select_outputs(area);
                    selected_outputs.insert(selected_outputs.end(), selout.begin(), selout.end());

                    auto selin = node->select_inputs(area);
                    selected_inputs.insert(selected_inputs.end(), selin.begin(), selin.end());
                }
            }

            moved_mouse = false;
        }

        if (IsKeyDown(KEY_ENTER)) {
            if (!selected_outputs.empty()) {

                std::sort(selected_inputs.begin(), selected_inputs.end(), [](Input_connector* a, Input_connector* b) {
                    return a->get_connection_pos().y > b->get_connection_pos().y; // Return true if 'a' should come before 'b'
                    });

                std::sort(selected_outputs.begin(), selected_outputs.end(), [](Output_connector* a, Output_connector* b) {
                    return a->get_connection_pos().y > b->get_connection_pos().y; // Return true if 'a' should come before 'b'
                    });

                if (selected_outputs.size() > 1) {
                    for (size_t i = 0; i < std::min(selected_inputs.size(), selected_outputs.size()); i++) {
                        selected_inputs[i]->target = selected_outputs[i];
                        network_change();
                    }
                }
                else {
                    for (size_t i = 0; i < selected_inputs.size(); i++) {
                        selected_inputs[i]->target = selected_outputs[0];
                        network_change();
                    }
                }
            }
            
        }

        if (IsKeyReleased(KEY_DELETE)) {
            for (auto& input : selected_inputs) {
                input->target = nullptr;
                network_change();
            }
            if (!selected_outputs.empty()) {
                for (Node* node : nodes) {
                    for (Input_connector& incon : node->inputs) {
                        // Check if incon.target is in selected_outputs
                        for (auto& selected_output : selected_outputs) {
                            if (incon.target == selected_output) {
                                incon.target = nullptr;
                                network_change();
                                break; // No need to check other selected_outputs if a match is found
                            }
                        }
                    }
                }
            }
        }

        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            area_selected = false;
        }

        break;
    }

    case INTERACT:
    {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            static Vector2 press_pos;
            press_pos = GetScreenToWorld2D(GetMousePosition(), camera);
            for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
                Node* node = *it;
                if (CheckCollisionPointRec(press_pos, { node->pos.x - node->size.x / 2.0f - 10, node->pos.y - node->size.y / 2.0f - 10, node->size.x + 20, node->size.y + 20 })) {
                    node->clicked(press_pos);
                }
                else node->not_clicked();
            }
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            for (Node* node : nodes) {
                node->not_clicked();
            }
        }
        break;
    }
    default:
        #ifndef NDEBUG
        assert(0 && "impossible to reach");
        #endif
        break;
    }
}

void Game::save_json(std::string filePath)
{
    if (filePath.empty()) {
        std::cout << "No file path selected\n";
        return;
    }

    std::filesystem::path filepath(filePath);
    std::string filename = filepath.stem().string();

    json myJson = {
        {"label", filename.c_str()},
        {"camera", camera},
        {"nodes", json::array()}
    };

    for(Node* node : nodes)
        myJson["nodes"].push_back(node->to_JSON());

    std::ofstream outputFile(filePath);

    if (outputFile.is_open()) {
        // Write the JSON data to the file
        outputFile << std::setw(4) << myJson << std::endl;

        // Close the file stream
        outputFile.close();

        std::cout << "JSON data saved to file: " << filePath << std::endl;
    }
    else {
        std::cerr << "Error opening file for writing: " << filePath << std::endl;
    }

}

void connect_node_network(std::vector<Node*>* nodes) {
    for (Node* node : *nodes) {
        for (Input_connector& input : node->inputs) {

            bool found = false;
            uid_t id = input.target_id;
            if (id) {
                for (Node* node2 : *nodes) {
                    if (found) break;
                    for (Output_connector& output : node2->outputs) {
                        if (output.id == id) {
                            input.target = &output;
                            found = true;
                            break;
                        }
                    }
                }
            }

        }
    }
}

void NodeNetworkFromJson(const json& nodeNetworkJson, std::vector<Node*> * nodes) {
    for (const auto& node : nodeNetworkJson) {
        // Each node is a JSON object where the key is the gate type
        for (auto it = node.begin(); it != node.end(); ++it) {
            std::string nodeType = it.key(); // Get the gate type (e.g., "GateAND")
            json nodeJson = it.value(); // Get the JSON object representing the node

            Node* node = NodeFactory::createNode(nodes, nodeType);
            assert(node && "node not created");
            node->load_JSON(nodeJson);
            nodes->push_back(node);
        }
    }

    connect_node_network(nodes);
}

void NodeNetworkFromBinary(std::filesystem::path filepath, std::vector<Node*>* nodes)
{
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "file didn't open properly";
        return;
    }
    std::vector<uint8_t> save;
    {
        std::streamsize size = file.tellg();
        save.resize(size);
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(save.data()), size);
    }
    file.close();

#define GET_AT(type, pos) reinterpret_cast<type*>(&save[pos])

    SaveHeader* saveheader = GET_AT(SaveHeader, 0);
    assert(saveheader->version.version_number == 0);

    //load network
    size_t curr_node_offset = saveheader->Nodes_offset;
    for (size_t i = 0; i < saveheader->node_count; i++) {
        NodeData* nodedata = GET_AT(NodeData, curr_node_offset);
        
        Node* node = NodeFactory::createNode(nodes, toString(nodedata->type));
        assert(node && "node not created");

        const RootNodeHeader* logic_block_root = reinterpret_cast<const RootNodeHeader*>(save.data() + saveheader->LogicBlock_offset);
        assert(logic_block_root->type == NodeType::RootNode);

        node->load_Bin(GET_AT(uint8_t, curr_node_offset), save.data());
        nodes->push_back(node);

        curr_node_offset += nodedata->total_size;
    }

    connect_node_network(nodes);
#undef GET_AT
}

void NormalizeNodeNetworkPosToLocation(std::vector<Node*>& nodes, Vector2 targpos)
{
    if (nodes.empty()) return;
    Vector2 origin1 = nodes[0]->pos;
    for (Node* node : nodes) {
        node->pos = node->pos - origin1 + targpos;
    }
}

void Game::save_bin(std::string filePath)
{
    SaveBuilder savebuilder = SaveBuilder();
    savebuilder.save_game(filePath);
}

void Game::build_logic_block()
{
    // save current state before rebuild
    for (Node* node : nodes) {
        if (!node->has_offset()) continue;
        FunctionNode* funnode = dynamic_cast<FunctionNode*>(node);
        if (funnode && funnode->has_function_data()) {
            size_t abs_offset = funnode->get_abs_node_offset();
            FunctionNodeHeader* header = get_logicblock<FunctionNodeHeader>(abs_offset);
            assert(header->type == NodeType::FunctionNode);
            uint8_t* function_data = reinterpret_cast<uint8_t*>(header);
            funnode->allocate_function_data(function_data);
        }
    }

    LogicBlockBuilder builder;
    builder.add_root(nodes);

    // Record start time
    auto start = std::chrono::high_resolution_clock::now();

    logicblock = std::unique_ptr<LogicBlock>(builder.build());


    // Record end time
    auto end = std::chrono::high_resolution_clock::now();
    // Calculate duration

    if (logicblock->get_header()->total_size < 3000) {
        logicblock->hexdump();
        logicblock->parse(logicblock->get_data(0));
    }
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "\nCreated logicblock out of current configuration. Size: " << logicblock->get_header()->total_size << " bytes. Duration: " << duration.count() << " microseconds" << std::endl;
    has_updated = true;
}

void Node::draw()
{
    Game& game = Game::getInstance();
    float roundness = 0.1f;
    int segments = 50;
    float lineThick = 10;
    Rectangle rec = { pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };


    DrawRectangleRec(rec, color);

    if (game.camera.zoom > 1 / 10.0f && !is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(color, -0.2f));
    if (is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(GREEN, -0.2f));

    //draw icon
    if (game.camera.zoom > 0.43f) {
        float texture_scale = 0.1f;

        float texture_pos_x = pos.x - get_texture().width / 2.0f * texture_scale;
        float texture_pos_y = pos.y - get_texture().height / 2.0f * texture_scale;
        DrawTextureEx(get_texture(), { texture_pos_x, texture_pos_y }, 0.0f, texture_scale, ColorBrightness(WHITE, -0.4f));
    }

    //draw name
    if (game.camera.zoom > 1 / 10.0f) {
        DrawTextEx(game.regular, label.c_str(), { pos.x - size.x / 2, pos.y + size.y / 2.0f + lineThick + 2 }, 30, 1.0f, WHITE); // Draw text using font and additional parameters
    }

    //draw inputs
    for (const Input_connector& inconn : inputs) inconn.draw();

    //draw outputs
    for (const Output_connector& outconn : outputs) outconn.draw();
}

bool Node::show_node_editor()
{
    Game& game = Game::getInstance();
    Vector2 Pos = GetWorldToScreen2D(pos + Vector2{ size.x / 2 , 0 }, game.camera);

    const static float area_width = 176;
    const static float margin = 4;
    const static float content_w = area_width - 2*margin;

    static float area_height = 232;

    Rectangle area = { Pos.x + 0, Pos.y + 0, area_width, area_height };

    static bool TextBoxNodeLabelEditMode = false;
    const static size_t buffersize = 256;
    char TextBoxNodeLabel[256] = "";
    strcpy_s(TextBoxNodeLabel, buffersize, label.c_str());

    GuiPanel(area, "Node Settings");

    float curr_el_h;
    float current_depth = 30;

    {   // label
        curr_el_h = 32;
        float current_x = Pos.x + margin;
        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 32, 16 }, "Label:");
        current_x += 32 + margin;

        if (GuiTextBox(Rectangle{ current_x, Pos.y + current_depth, Pos.x + margin + content_w - current_x, 32 }, TextBoxNodeLabel, buffersize, TextBoxNodeLabelEditMode)) {
            TextBoxNodeLabelEditMode = !TextBoxNodeLabelEditMode;
        }
        if (label.c_str() != TextBoxNodeLabel) {
            change_label(TextBoxNodeLabel);
        }
        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    {   // Connectors
        curr_el_h = 32;
        float current_x = Pos.x + margin;

        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "Connectors:");
        current_x += 64 + margin;

        if (GuiButton(Rectangle{ current_x, Pos.y + current_depth, 32, 32 }, "#121#")) add_input();
        current_x += 32 + margin;

        if (GuiButton(Rectangle{ current_x, Pos.y + current_depth, 32, 32 }, "#120#")) remove_input();
        current_x += 32 + margin;

        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    {   // Has_changed
        curr_el_h = 32;
        float current_x = Pos.x + margin;

        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "has_changed:");
        current_x += 64 + margin;

        if (has_changed) {
            GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "true");
            current_x += 64 + margin;
        }
        else {
            GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "false");
            current_x += 64 + margin;
        }

        current_depth += curr_el_h;
    }
    
    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    
    area_height = current_depth;
    return CheckCollisionPointRec(GetMousePosition(), area);
}

Input_connector* Node::select_input(Vector2 select_pos)
{
    const size_t width = 30;
    float lineThick = 8;
    float spacing = 30;

    for (Input_connector& conn : inputs) {

        Vector2 pos = {
            conn.host->pos.x - conn.host->size.x / 2 - width,
            conn.host->pos.y + ((float)conn.host->inputs.size() - 1.0f) * spacing / 2.0f - conn.index * spacing - lineThick / 2.0f
        };

        Rectangle rec = {
            pos.x,
            pos.y,
            width,
            lineThick
        };

        if (CheckCollisionPointRec(select_pos, rec))
            return &conn;

    }
    return nullptr;
}

Output_connector* Node::select_output(Vector2 select_pos)
{
    for (Output_connector& conn : outputs) {
        const size_t width = 30;
        float lineThick = 8;
        float spacing = 30;

        Vector2 pos = {
            conn.host->pos.x + conn.host->size.x / 2.0f,
            conn.host->pos.y + (conn.host->outputs.size() - 1) * spacing / 2.0f - conn.index * spacing - lineThick / 2.0f
        };

        Rectangle rec = {
            pos.x,
            pos.y,
            width,
            lineThick
        };

        if (CheckCollisionPointRec(select_pos, rec))
            return &conn;

    }
    return nullptr;
}

std::vector<Input_connector*> Node::select_inputs(Rectangle select_area)
{
    std::vector<Input_connector*> incons;

    const size_t width = 30;
    float lineThick = 8;
    float spacing = 30;

    for (Input_connector& conn : inputs) {

        Vector2 pos = {
            conn.host->pos.x - conn.host->size.x / 2 - width,
            conn.host->pos.y + ((float)conn.host->inputs.size() - 1.0f) * spacing / 2.0f - conn.index * spacing - lineThick / 2.0f
        };

        Rectangle rec = {
            pos.x,
            pos.y,
            width,
            lineThick
        };

        if (CheckCollisionRecs(select_area, rec))
            incons.push_back(&conn);

    }

    return incons;
}

std::vector<Output_connector*> Node::select_outputs(Rectangle select_area)
{
    std::vector<Output_connector*> outcons;

    for (Output_connector& conn : outputs) {
        const size_t width = 30;
        float lineThick = 8;
        float spacing = 30;

        Vector2 pos = {
            conn.host->pos.x + conn.host->size.x / 2.0f,
            conn.host->pos.y + (conn.host->outputs.size() - 1) * spacing / 2.0f - conn.index * spacing - lineThick / 2.0f
        };

        Rectangle rec = {
            pos.x,
            pos.y,
            width,
            lineThick
        };

        if (CheckCollisionRecs(select_area, rec))
            outcons.push_back(&conn);
    }

    return outcons;
}

void Node::tick()
{
    has_changed = false;
    for (Output_connector& conn : outputs) {
        if (conn.new_state != conn.state) {
            has_changed = true;
            conn.state = conn.new_state;
        }
    }
}

Texture GateAND::texture = Texture{ 0 };

Texture GateOR::texture = Texture{ 0 };

Texture GateNAND::texture = Texture{ 0 };

Texture GateNOR::texture = Texture{ 0 };

Texture GateXOR::texture = Texture{ 0 };

Texture GateXNOR::texture = Texture{ 0 };

Texture GateBUFFER::texture = Texture{ 0 };

Texture GateNOT::texture = Texture{ 0 };

Game Game::instance;

void GateNOT::pretick()
{
    for (size_t i = 0; i < inputs.size(); i++) {
        if (inputs[i].target)
            outputs[i].new_state = !inputs[i].target->state;
        else
            outputs[i].new_state = true;

    }
}

void GateAND::pretick()
{
    if (inputs.empty()) { outputs[0].new_state = false; return; }
    bool retval = true;

    for (Input_connector& in : inputs) {
        if (!in.target || !in.target->state) retval = false;
    }
    outputs[0].new_state = retval;

    return;
}

void GateBUFFER::pretick()
{
    for (size_t i = 0; i < inputs.size(); i++) {
        if (inputs[i].target)
            outputs[i].new_state = inputs[i].target->state;
        else
            outputs[i].new_state = false;
    }
}

void GateOR::pretick()
{
    if (inputs.empty()) { outputs[0].new_state = false; return; }
    bool retval = false;

    for (Input_connector& in : inputs) {
        if (in.target && in.target->state) retval = true;
    }
    outputs[0].new_state = retval;
    return;
}

void GateNAND::pretick()
{
    if (inputs.empty()) { outputs[0].new_state = true; return; }
    bool retval = false;

    for (Input_connector& in : inputs) {
        if (!in.target || !in.target->state) retval = true;
    }
    outputs[0].new_state = retval;
    return;
}

void GateNOR::pretick()
{
    if (inputs.empty()) { outputs[0].new_state = true; return; }
    bool retval = true;

    for (Input_connector& in : inputs) {
        if (in.target && in.target->state) retval = false;
    }
    outputs[0].new_state = retval;
    return;
}

void GateXOR::pretick()
{
    if (inputs.empty()) { outputs[0].new_state = false; return; }
    bool retval = false;

    for (Input_connector& in : inputs) {
        if (in.target && in.target->state) retval = !retval;
    }

    outputs[0].new_state = retval;
    return;
}

void GateXNOR::pretick()
{
    if (inputs.empty()) { outputs[0].new_state = true; return; }
    bool retval = true;

    for (Input_connector& in : inputs) {
        if (in.target && in.target->state) retval = !retval;
    }
    outputs[0].new_state = retval;
    return;
}

void Button::draw()
{
    Game& game = Game::getInstance();
    float roundness = 0.1f;
    int segments = 50;
    float lineThick = 10;
    Rectangle rec = { pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };

    size_t button_count = outputs.size();
    for (size_t i = 0; i < button_count; i++) {
        Rectangle rec = getButtonRect(i);
        if (outputs[i].state)
            DrawRectangleRec(rec, Color{ 219, 42, 2, 255 });
        else
            DrawRectangleRec(rec, Color{ 252, 57, 13, 255 });
        if (game.camera.zoom > 0.4 && i < button_count - 1)
            DrawLineEx({ rec.x, rec.y }, { rec.x + rec.width, rec.y }, lineThick / 1.0f, ColorBrightness(color, -0.2f));
    }


    if (game.camera.zoom > 1 / 10.0f && !is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(color, -0.2f));
    if (is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(GREEN, -0.6f));

    //draw icon
    if (game.camera.zoom > 0.43f) {
        float texture_scale = 0.1f;

        float texture_pos_x = pos.x - get_texture().width / 2.0f * texture_scale;
        float texture_pos_y = pos.y - get_texture().height / 2.0f * texture_scale;
        DrawTextureEx(get_texture(), { texture_pos_x, texture_pos_y }, 0.0f, texture_scale, ColorBrightness(WHITE, -0.4f));
    }

    //draw name
    if (game.camera.zoom > 1 / 10.0f) {
        DrawTextEx(game.regular, label.c_str(), { pos.x - size.x / 2, pos.y + size.y / 2.0f + lineThick + 2 }, 30, 1.0f, WHITE); // Draw text using font and additional parameters
    }

    //draw inputs
    for (const Input_connector& conn : inputs)
        conn.draw();

    //draw outputs
    for (const Output_connector& conn : outputs)
        conn.draw();
}

Rectangle Button::getButtonRect(size_t id)
{
    Rectangle rec {
        pos.x - size.x / 2.0f, 
        
        pos.y + size.y / 2.0f - (id + 1) * size.y / outputs.size()
        
        , size.x, size.y / outputs.size()
    };
    return rec;
}

void Button::recompute_size()
{
    size = Vector2{ 130.0f, 100.0f + 30.0f * outputs.size() };
}

void ToggleButton::clicked(Vector2 pos)
{
    has_changed = false;
    for (size_t i = 0; i < outputs.size(); i++) {
        if (CheckCollisionPointRec(pos, getButtonRect(i))) { 
            set_output_state(i, !outputs[i].state);
            has_changed = true;
        }
    }
}

json Node::to_JSON() const {

    json jOutputs = json::array();
    for (const auto& output : outputs) {
        jOutputs.push_back(output.to_JSON());
    }

    json jInputs = json::array();
    for (const auto& input : inputs) {
        jInputs.push_back(input.to_JSON());
    }

    return 
    {
           
        {get_type_str(),
            {
                {"pos.x", pos.x},
                {"pos.y", pos.y},
                {"size.x", size.x},
                {"size.y", size.y},
                {"label", label},
                {"outputs", jOutputs},
                {"inputs", jInputs}
            }
        }
    };
}

void Node::load_JSON(const json& nodeJson) {
    try {
        label = nodeJson.at("label").get<std::string>();
        pos.x = nodeJson.at("pos.x").get<float>();
        pos.y = nodeJson.at("pos.y").get<float>();

        size.x = nodeJson.at("size.x").get<float>();
        size.y = nodeJson.at("size.y").get<float>();

        inputs.clear();
        {
            size_t i = 0;
            for (const json& inputJson : nodeJson.at("inputs")) {
                unsigned long target = inputJson.at("Input_connector").at("target").get<unsigned long>();
                inputs.push_back(Input_connector(this, i, "", nullptr, target));
                i++;
            }
        }

        outputs.clear();
        {
            size_t i = 0;
            for (const json& inputJson : nodeJson.at("outputs")) {
                unsigned long id = inputJson.at("Output_connector").at("id").get<unsigned long>();
                bool state = inputJson.at("Output_connector").at("state").get<bool>();
                outputs.push_back(Output_connector(this, i, "", state, id));
                i++;
            }
        }
        
    }
    catch (const json::exception& e) {
        // Handle or log error, e.g., missing key or wrong type
        std::cerr << "JSON parsing error: " << e.what() << '\n';
    }

    load_extra_JSON(nodeJson);
}

void Node::load_Bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr)
{
    const NodeData* node_data = reinterpret_cast<const NodeData*>(node_data_ptr);
    label = node_data->label;
    pos = node_data->pos;
    size = node_data->size;

    abs_node_offset = node_data->abs_node_offset;

    inputs.clear();
    for (size_t i = 0; i < node_data->input_count; i++) {
        const InputData* inputdata = reinterpret_cast<const InputData*>(save_ptr + node_data->inputs_offset + i * sizeof(InputData));
        inputs.push_back(Input_connector(this, i, inputdata->name, nullptr, inputdata->target_id));

    }

    outputs.clear();
    for (size_t i = 0; i < node_data->output_count; i++) {
        const OutputData* outputdata = reinterpret_cast<const OutputData*>(save_ptr + node_data->outputs_offset + i * sizeof(OutputData));
        outputs.push_back(Output_connector(this, i, outputdata->name, outputdata->state, outputdata->id));
    }

    load_extra_bin(node_data_ptr, save_ptr);
}

void Node::update_state_from_logicblock()
{
    using namespace LogicblockTools;
    assert(abs_node_offset != 0);
    Game& game = Game::getInstance();
    uint8_t* logicblock_ptr = game.get_logicblock<uint8_t>(0);

    NodeHeader* header = get_at<NodeHeader>(abs_node_offset, logicblock_ptr);

    assert(header->type == get_type());
    switch (header->type) {
        // Binary Gates
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(abs_node_offset, logicblock_ptr);
        outputs[0].state = header->output;
        break;
    }

                           // Unary Gates
    case NodeType::GateBUFFER:
    case NodeType::GateNOT: 

    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton:

    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay:

    case NodeType::FunctionNode: {
        offset abs_outputs_offset = abs_node_offset + outputs_offset(abs_node_offset, logicblock_ptr);
        assert(output_count(abs_node_offset, logicblock_ptr) == outputs.size());

        for (size_t i = 0; i < outputs.size(); i++) {
            outputs[i].state = *game.get_logicblock<output>(abs_outputs_offset + i * sizeof(output));
        }
        break;
    }

    case NodeType::BusNode: {
        //TODO this is jank but should work, creates absolute offset as long as it's the child of the root node
        offset abs_outputs_offset = sizeof(RootNodeHeader) + outputs_offset(abs_node_offset, logicblock_ptr);
        assert(output_count(abs_node_offset, logicblock_ptr) == outputs.size());

        for (size_t i = 0; i < outputs.size(); i++) {
            outputs[i].state = *game.get_logicblock<output>(abs_outputs_offset + i * sizeof(output));
        }
        break;
    }

    default:
        assert(false && "Invalid NodeType");
    }
}

void Output_connector::draw() const
{
    Game& game = Game::getInstance();
    const size_t width = 30;
    float lineThick = 8;
    float height_spacing = 30;

    Vector2 startPos = {
        host->pos.x + host->size.x / 2.0f,
        host->pos.y + (host->outputs.size() - 1) * height_spacing / 2.0f - index * height_spacing
    };

    Vector2 endPos = { startPos.x + width ,startPos.y, };

    Color color = GRAY;
    for (auto output : game.selected_outputs) {
        if (output == this) {
            color = GREEN;
            break;
        }
    }

    DrawLineEx(startPos, endPos, lineThick, color);

    // draw name
    {
        float text_spacing = 2.0f;

        Vector2 pos = {
            host->pos.x + host->size.x / 2.0f,
            host->pos.y + (host->outputs.size() - 1) * height_spacing / 2.0f - index * height_spacing - lineThick / 2.0f
        };

        Font font = GetFontDefault();
        const char* text;
        text = name.c_str();
        Color color = RAYWHITE;
        if (state)
            color = DARKGREEN;
        DrawTextEx(font, text, pos - Vector2{ float(name.size()) * (text_spacing + 7.0f), 0 }, 12, text_spacing, color);
    }
}

json Output_connector::to_JSON() const {
    return json{ 
        {"Output_connector", json::object({  {"id", id}, {"state", state}})}
    };
}

void Input_connector::draw() const
{
    Game& game = Game::getInstance();
    const size_t width = 30;
    float lineThick = 8;
    float height_spacing = 30;

    Vector2 startPos = {
        host->pos.x - host->size.x / 2,
        host->pos.y + ((float)host->inputs.size() - 1.0f) * height_spacing / 2.0f - index * height_spacing
    };
    Vector2 endPos = { startPos.x - width ,startPos.y, };

    Color color = GRAY;
    for (auto input : game.selected_inputs) {
        if (input == this) {
            color = GREEN;
            break;
        }
    }

    DrawLineEx(startPos, endPos, lineThick, color);

    if (target) {
        if (target->state) {
            DrawLineEx(get_connection_pos(), target->get_connection_pos(), lineThick, GREEN);
        }
        else {
            DrawLineEx(get_connection_pos(), target->get_connection_pos(), lineThick, GRAY);
        }
    }

    //Draw name
    {
        float text_spacing = 2.0f;
        Vector2 pos = {
            host->pos.x - host->size.x / 2 - width,
            host->pos.y + ((float)host->inputs.size() - 1.0f) * height_spacing / 2.0f - index * height_spacing - lineThick / 2.0f
        };

        Font font = GetFontDefault();
        const char* text = name.c_str();
        Color color = RAYWHITE;
        if (target && target->state)
            color = DARKGREEN;
        DrawTextEx(font, text, pos + Vector2{ width, 0 }, 12, text_spacing, color);
    }
}

json Input_connector::to_JSON() const {
    return json{
        {"Input_connector", json::object({  {"target", target ? target->id : 0}})}
    };
}

FunctionNode::FunctionNode(const FunctionNode* base): Node(base), is_single_tick(base->is_single_tick), is_cyclic_val(base->is_cyclic_val), function_data(base->function_data)
{
    nodes.clear();
    size_t* idxs = new size_t[base->nodes.size()];

    size_t idx = 0;
    for (size_t i = 0; i < base->nodes.size(); ++i) {
        nodes.push_back(base->nodes[i]->copy());
        nodes[i]->move_to_container(&nodes);
        idxs[i] = idx;
        ++idx;
    }

    for (size_t i = 0; i < nodes.size(); ++i) {

        for (Input_connector& input : nodes[i]->inputs) {
            if (input.target) {

                size_t target_idx = -1;
                for (size_t p = 0; p < input.target->host->outputs.size(); ++p) {
                    if (&input.target->host->outputs[p] == input.target) target_idx = p;
                }

                assert(target_idx != -1);

                for (size_t j = 0; j < base->nodes.size(); ++j) {

                    if (base->nodes[j] == input.target->host) {
                        input.target = &(nodes[idxs[j]]->outputs[target_idx]);
                    }
                }

            }
        }
    }
    delete[] idxs;

    // populate and sort the arrays for where to route the input and output connectors on the function node
    for (Node* node : nodes) {
        if (node->isInput()) {
            input_targs.push_back(node);
        }
        if (node->isOutput()) {
            output_targs.push_back(node);
        }
    }

    std::sort(input_targs.begin(), input_targs.end(), [](Node* a, Node* b) {
        return a->pos.y > b->pos.y; // Return true if 'a' should come before 'b'
        });

    std::sort(output_targs.begin(), output_targs.end(), [](Node* a, Node* b) {
        return a->pos.y > b->pos.y; // Return true if 'a' should come before 'b'
        });

    recompute_size();
}

FunctionNode::~FunctionNode()
{
    for (Node* node : nodes) {
        delete node;
    }
}

bool FunctionNode::show_node_editor()
{
    Game& game = Game::getInstance();
    Vector2 Pos = GetWorldToScreen2D(pos + Vector2{ size.x / 2 , 0 }, game.camera);

    const static float area_width = 176;
    const static float margin = 4;
    const static float content_w = area_width - 2 * margin;

    static float area_height = 232;

    Rectangle area = { Pos.x + 0, Pos.y + 0, area_width, area_height };

    static bool TextBoxNodeLabelEditMode = false;
    const static size_t buffersize = 256;
    char TextBoxNodeLabel[256] = "";
    strcpy_s(TextBoxNodeLabel, buffersize, label.c_str());

    GuiPanel(area, "Node Settings");
    
    float curr_el_h;
    float current_depth = 30;

    {   // label
        curr_el_h = 32;
        float current_x = Pos.x + margin;
        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 32, 16 }, "Label:");
        current_x += 32 + margin;

        if (GuiTextBox(Rectangle{ current_x, Pos.y + current_depth, Pos.x + margin + content_w - current_x, 32 }, TextBoxNodeLabel, buffersize, TextBoxNodeLabelEditMode))
            TextBoxNodeLabelEditMode = !TextBoxNodeLabelEditMode;
        label = TextBoxNodeLabel;
        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    {   // is_cyclic
        curr_el_h = 32;
        float current_x = Pos.x + margin;

        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "is_cyclic:");
        current_x += 64 + margin;

        if (is_cyclic_val.value()) {
            GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "true");
            current_x += 64 + margin;
        }
        else {
            GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "false");
            current_x += 64 + margin;
        }

        current_depth += curr_el_h;
    }


    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    {   // Depth
        curr_el_h = 32;
        if(delay_str.empty())
            delay_str = std::to_string(delay());
        float current_x = Pos.x + margin;

        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "depth:");
        current_x += 64 + margin;

        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, delay_str.c_str());
        current_x += 64 + margin;

        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    {   // Has_changed
        curr_el_h = 32;
        float current_x = Pos.x + margin;

        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "has_changed:");
        current_x += 64 + margin;

        if (has_changed) {
            GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "true");
            current_x += 64 + margin;
        }
        else {
            GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "false");
            current_x += 64 + margin;
        }

        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    {   // Change mode
        curr_el_h = 32;
        float current_x = Pos.x + margin;

        if (is_cyclic()){}
        else if (!is_single_tick) {
            GuiToggle(Rectangle{ current_x, Pos.y + current_depth, 128, 32 }, "make_single_tick", &is_single_tick);
            if (is_single_tick) 
                delay_str = "1";
            current_depth += curr_el_h;
        }
        else {
            GuiToggle(Rectangle{ current_x, Pos.y + current_depth, 128, 32 }, "make_normal_timing", &is_single_tick);
            if (!is_single_tick)
                delay_str = std::to_string(delay());
            current_depth += curr_el_h;
        }
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }


    area_height = current_depth;
    return CheckCollisionPointRec(GetMousePosition(), area);
}

json FunctionNode::to_JSON() const
{

    json jOutputs = json::array();
    for (const auto& output : outputs) {
        jOutputs.push_back(output.to_JSON());
    }

    json jInputs = json::array();
    for (const auto& input : inputs) {
        jInputs.push_back(input.to_JSON());
    }

    json myJson = {
        {get_type_str(),
            {
                {"pos.x", pos.x},
                {"pos.y", pos.y},
                {"size.x", size.x},
                {"size.y", size.y},
                {"label", label},
                {"outputs", jOutputs},
                {"inputs", jInputs},
                {"nodes", json::array()}
            }
        }
    };

    for (Node* node : nodes)
        myJson[get_type_str()]["nodes"].push_back(node->to_JSON());

    return myJson;
}

void FunctionNode::load_extra_JSON(const json& nodeJson)
{
    try {
        // load all the nodes
        nodes.clear();
        if (nodeJson.contains(get_type_str()))
            NodeNetworkFromJson(nodeJson.at(get_type_str()).at("nodes"), &nodes);
        else if (nodeJson.contains("nodes"))
            NodeNetworkFromJson(nodeJson.at("nodes"), &nodes);
        else
            std::cerr << "JSON parsing error: \n";

        load_from_nodes();
        is_cyclic_val = is_cyclic();
        sort_linear();
    }
    catch (const json::exception& e) {
        // Handle or log error, e.g., missing key or wrong type
        std::cerr << "JSON parsing error: " << e.what() << '\n';
    }
}

void FunctionNode::load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr)
{
    const SaveHeader* saveheader = reinterpret_cast<const SaveHeader*>(save_ptr);
    const NodeData* nodedata = reinterpret_cast<const NodeData*>(node_data_ptr);


    const RootNodeHeader* logic_block_root = reinterpret_cast<const RootNodeHeader*>(save_ptr + saveheader->LogicBlock_offset);
    assert(logic_block_root->type == NodeType::RootNode);
    const uint8_t * funheader_ptr = save_ptr + saveheader->LogicBlock_offset + nodedata->abs_node_offset;
    allocate_function_data(funheader_ptr);
}

void FunctionNode::load_from_nodes()
{
    // populate and sort the arrays for where to route the input and output connectors on the function node
    for (Node* node : nodes) {
        if (node->isInput()) {
            input_targs.push_back(node);
        }
        if (node->isOutput()) {
            output_targs.push_back(node);
        }
    }
    std::sort(input_targs.begin(), input_targs.end(), [](Node* a, Node* b) {
        return a->pos.y > b->pos.y; // Return true if 'a' should come before 'b'
        });

    std::sort(output_targs.begin(), output_targs.end(), [](Node* a, Node* b) {
        return a->pos.y > b->pos.y; // Return true if 'a' should come before 'b'
        });

    // create input and output connectors then resize the node
    size_t targ_input_count = 0;
    for (size_t i = 0; i < input_targs.size(); i++) {
        for (size_t j = 0; j < input_targs[i]->outputs.size(); j++) {
            targ_input_count++;
        }
    }
    size_t targ_output_count = 0;
    for (size_t i = 0; i < output_targs.size(); i++) {
        for (size_t j = 0; j < output_targs[i]->inputs.size(); j++) {
            targ_output_count++;
        }
    }

    // adjust the connector count
    {
        while (inputs.size() < targ_input_count) {
            inputs.push_back(Input_connector(this, inputs.size(), ""));
        }
        while (inputs.size() > targ_input_count) {
            inputs.pop_back();
        }
        assert(inputs.size() == targ_input_count);

        while (outputs.size() < targ_output_count) {
            outputs.push_back(Output_connector(this, outputs.size(), ""));
        }
        while (outputs.size() > targ_output_count) {
            outputs.pop_back();
        }
    }
    

    //label the connectors
    {
        size_t input_index = 0;
        for (Node* targ_in : input_targs) {
            for (Output_connector& _ : targ_in->outputs) {
                for (Input_connector& inconn : inputs) {
                    if (inconn.index == input_index) {
                        inconn.name = targ_in->label;
                        input_index++;
                        break;
                    }
                }
            }
        }

        size_t output_index = 0;
        for (Node* targ_out : output_targs) {
            for (Input_connector& _ : targ_out->inputs) {
                for (Output_connector& outconn : outputs) {
                    if (outconn.index == output_index) {
                        outconn.name = targ_out->label;
                        output_index++;
                        break;
                    }
                }
            }
        }
    }

    assert(outputs.size() == targ_output_count);
    recompute_size();
}

void FunctionNode::draw()
{
    Game& game = Game::getInstance();
    float roundness = 0.1f;
    int segments = 50;
    float lineThick = 10;
    Rectangle rec = { pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };


    DrawRectangleRec(rec, color);

    if (game.camera.zoom > 1 / 10.0f && !is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(color, -0.2f));
    if (is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(GREEN, -0.2f));

    //draw icon
    if (game.camera.zoom > 0.43f) {
        float texture_scale = 0.1f;

        float texture_pos_x = pos.x - get_texture().width / 2.0f * texture_scale;
        float texture_pos_y = pos.y - get_texture().height / 2.0f * texture_scale;
        DrawTextureEx(get_texture(), { texture_pos_x, texture_pos_y }, 0.0f, texture_scale, ColorBrightness(WHITE, -0.4f));
    }

    //draw name
    if (game.camera.zoom > 1 / 10.0f) {
        DrawTextEx(game.regular, label.c_str(), { pos.x - size.x / 2, pos.y + size.y / 2.0f + lineThick + 2 }, 30, 1.0f, WHITE); // Draw text using font and additional parameters
    }

    //draw inputs
    for (const Input_connector& inconn : inputs) inconn.draw();

    //draw outputs
    for (const Output_connector& outconn : outputs) outconn.draw();
}

void FunctionNode::pretick()
{
    Game& game = Game::getInstance();
    {
        size_t i = 0;
        for (size_t x = 0; x < input_targs.size(); x++) {
            for (size_t y = 0; y < input_targs[x]->outputs.size(); y++) {
                if (inputs[i].target) {
                    if (inputs[i].target->host->has_changed) {
                        has_changed = true;
                        input_targs[x]->outputs[y].state = inputs[i].target->state;
                        input_targs[x]->outputs[y].host->has_changed = true;
                    }
                }
                else
                    input_targs[x]->outputs[y].state = false;
                i++;
            }
        }
    }
    
    if (is_single_tick) {

        if (has_changed) {
            for (Node* node : nodes) {
                node->pretick();
                node->tick();
            }
        }
    }
    else {

        if (has_changed ||!game.get_efficient_simulation()) {
            for (Node* node : nodes) {
                node->pretick();
            }
        }
    }
}

void FunctionNode::tick()
{
    Game& game = Game::getInstance();
    if (is_single_tick) {
        if (has_changed || !game.get_efficient_simulation()) {
            has_changed = false;
            for (Node* node : nodes) {
                if (node->has_changed) has_changed = true;
            }
            {
                size_t i = 0;
                for (size_t x = 0; x < output_targs.size(); x++) {
                    for (size_t y = 0; y < output_targs[x]->inputs.size(); y++) {
                        if (output_targs[x]->inputs[y].target) {
                            if (outputs[i].state != output_targs[x]->inputs[y].target->state) {
                                outputs[i].state = output_targs[x]->inputs[y].target->state;
                                has_changed = true;
                            }
                        }
                        else outputs[i].new_state = false;
                        i++;
                    }
                }
            }
        }
        return;
    }
    if (has_changed || !game.get_efficient_simulation()) {
        has_changed = false;
        for (Node* node : nodes) {
            node->tick();
            if (node->has_changed) has_changed = true;
        }

        {
            size_t i = 0;
            for (size_t x = 0; x < output_targs.size(); x++) {
                for (size_t y = 0; y < output_targs[x]->inputs.size(); y++) {
                    if (output_targs[x]->inputs[y].target) {
                        if (outputs[i].state != output_targs[x]->inputs[y].target->state) {
                            outputs[i].state = output_targs[x]->inputs[y].target->state;
                            has_changed = true;
                        }
                    }
                    else outputs[i].new_state = false;
                    i++;
                }
            }
        }
    }
}

bool FunctionNode::is_cyclic() const
{
    if (is_cyclic_val.has_value()) {
        return is_cyclic_val.value();
    }

    enum NodeState {
        Unvisited,
        Visiting,  // Node is being visited (used for cycle detection)
        Visited    // Node has been fully visited
    };

    std::unordered_map<Output_connector*, NodeState> marked_outconns;
    bool hasCycle = false;

    std::function<void(Output_connector*)> DFS;


    DFS = [&](Output_connector* outconn) {
        if (hasCycle || !outconn) return;

        if (marked_outconns[outconn] == NodeState::Visiting) {
            hasCycle = true;
            return;
        }

        if (outconn->host->is_cyclic()) {
            hasCycle = true;
            return;
        }

        if (marked_outconns[outconn] == NodeState::Visited) {
            return;
        }

        marked_outconns[outconn] = NodeState::Visiting;
        
        for (Input_connector* inconn : outconn->host->connected_inputs(outconn->index)) {
            DFS(inconn->target);
            if (hasCycle) return;
        }

        marked_outconns[outconn] = NodeState::Visited;
    };

    for (Node* outnode : output_targs) {
        for (Input_connector& inconn : outnode->inputs) {
            DFS(inconn.target);
            if (hasCycle) return true;
            marked_outconns.clear();
        }
    }

    return false;
}

int FunctionNode::delay() const
{
    if (is_cyclic()) return -1;

    std::unordered_map<Output_connector*, int> marked_outconns;
    int max_delay = 1;
    std::function<void(Output_connector*)> DFS;


    DFS = [&](Output_connector* outconn) {
        if (!outconn || max_delay == -1) return;

        if (outconn->host->is_cyclic()) {
            max_delay = -1;
            return;
        }

        if (outconn->host->isInput()) return;

        int current_delay = marked_outconns[outconn] + outconn->host->delay();
        if (current_delay > max_delay) max_delay = current_delay;

        for (Input_connector* inconn : outconn->host->connected_inputs(outconn->index)) {
            if (marked_outconns[inconn->target] < current_delay) {
                marked_outconns[inconn->target] = current_delay;
                DFS(inconn->target);
                if (max_delay == -1) return;
            }
        }
    };

    for (Node* outnode : output_targs) {
        for (Input_connector& inconn : outnode->inputs) {
            DFS(inconn.target);
        }
    }

    return max_delay;
}

void FunctionNode::sort_linear()
{
    if (is_cyclic()) return;

    std::unordered_map<Output_connector*, int> marked_outconns;
    int max_delay = 1;
    std::function<void(Output_connector*)> DFS;


    DFS = [&](Output_connector* outconn) {
        if (!outconn || max_delay == -1) return;

        if (outconn->host->is_cyclic()) {
            max_delay = -1;
            return;
        }

        if (outconn->host->isInput()) return;

        int current_delay = marked_outconns[outconn] + outconn->host->delay();
        if (current_delay > max_delay) max_delay = current_delay;

        for (Input_connector* inconn : outconn->host->connected_inputs(outconn->index)) {
            if (marked_outconns[inconn->target] < current_delay) {
                marked_outconns[inconn->target] = current_delay;
                DFS(inconn->target);
                if (max_delay == -1) return;
            }
        }
    };

    for (Node* outnode : output_targs) {
        for (Input_connector& inconn : outnode->inputs) {
            DFS(inconn.target);
        }
    }

    std::sort(nodes.begin(), nodes.end(), [&](Node* a, Node* b) {
        size_t max_a = 0;
        for (Output_connector& out : a->outputs) {
            if (marked_outconns[&out] > max_a) max_a = marked_outconns[&out];
        }
        size_t max_b = 0;
        for (Output_connector& out : b->outputs) {
            if (marked_outconns[&out] > max_b) max_b = marked_outconns[&out];
        }
        return max_a > max_b; // Return true if 'a' should come before 'b'
        });
}

void FunctionNode::recompute_size()
{
    size = Vector2{ 150, std::max(100 + 30 * float(inputs.size()), 100 + 30 * float(outputs.size())) };
}

void LightBulb::draw()
{
    {
        Game& game = Game::getInstance();
        float roundness = 0.1f;
        int segments = 50;
        float lineThick = 10;
        Rectangle rec = { pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };
        if (inputs[0].target && inputs[0].target->state)
            DrawRectangleRounded(rec, roundness, segments, YELLOW);
        else
            DrawRectangleRounded(rec, roundness, segments, BLACK);

        if (game.camera.zoom > 1 / 10.0f && !is_selected)
            DrawRectangleRoundedLines(rec, roundness, segments, lineThick, Fade(GRAY, 0.4f));
        if (is_selected)
            DrawRectangleRoundedLines(rec, roundness, segments, lineThick, Fade(GREEN, 0.4f));

        //draw icon
        if (game.camera.zoom > 0.43f) {
            float texture_scale = 0.1f;

            float texture_pos_x = pos.x - get_texture().width / 2.0f * texture_scale;
            float texture_pos_y = pos.y - get_texture().height / 2.0f * texture_scale;
            DrawTextureEx(get_texture(), { texture_pos_x, texture_pos_y }, 0.0f, texture_scale, Fade(WHITE, 0.6f));
        }

        //draw name
        if (game.camera.zoom > 1 / 10.0f) {
            DrawTextEx(game.regular, label.c_str(), { pos.x - size.x / 2, pos.y + size.y / 2.0f + lineThick + 2 }, 30, 1.0f, WHITE); // Draw text using font and additional parameters
        }

        //draw inputs
        for (const Input_connector& conn : inputs)
            conn.draw();
    }
}

void SevenSegmentDisplay::draw()
{
    Game& game = Game::getInstance();
    float roundness = 0.1f;
    int segments = 50;
    float lineThick = 10;
    Rectangle rec = { pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };


    //draw inputs
    for (const Input_connector& conn : inputs)
        conn.draw();

    DrawRectangleRec(rec, color);

    if (game.camera.zoom > 1 / 10.0f && !is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(color, -0.2f));
    if (is_selected)
        DrawRectangleRoundedLines(rec, roundness, segments, lineThick, ColorBrightness(GREEN, -0.2f));

    //draw name
    if (game.camera.zoom > 1 / 10.0f) {
        DrawTextEx(game.regular, label.c_str(), { pos.x - size.x / 2, pos.y + size.y / 2.0f + lineThick + 2 }, 30, 1.0f, WHITE); // Draw text using font and additional parameters
    }


    //draw segments
    Color color;
    float linethickness = 12.0f;

    if (inputs[0].target && inputs[0].target->state) color = {255, 0, 0, 255};
    else color = ColorBrightness(GRAY, -0.8f);
    DrawLineBezier(pos + Vector2{ -70.0f, -150.0f }, pos + Vector2{ 70.0f, -150.0f }, linethickness, color);

    if (inputs[1].target && inputs[1].target->state) color = { 255, 0, 0, 255 };
    else color = ColorBrightness(GRAY, -0.8f);
    DrawLineBezier(pos + Vector2{ 75.0f, -145.0f }, pos + Vector2{ 75.0f, -5.0f }, linethickness, color);

    if (inputs[2].target && inputs[2].target->state) color = { 255, 0, 0, 255 };
    else color = ColorBrightness(GRAY, -0.8f);
    DrawLineBezier(pos + Vector2{ 75.0f, 5.0f }, pos + Vector2{ 75.0f, 145.0f }, linethickness, color);

    if (inputs[3].target && inputs[3].target->state) color = { 255, 0, 0, 255 };
    else color = ColorBrightness(GRAY, -0.8f);
    DrawLineBezier(pos + Vector2{ -70.0f, 150.0f }, pos + Vector2{ 70.0f, 150.0f }, linethickness, color);

    if (inputs[4].target && inputs[4].target->state) color = { 255, 0, 0, 255 };
    else color = ColorBrightness(GRAY, -0.8f);
    DrawLineBezier(pos + Vector2{ -75.0f, 5.0f }, pos + Vector2{ -75.0f, 145.0f }, linethickness, color);

    if (inputs[5].target && inputs[5].target->state) color = { 255, 0, 0, 255 };
    else color = ColorBrightness(GRAY, -0.8f);
    DrawLineBezier(pos + Vector2{ -75.0f, -145.0f }, pos + Vector2{ -75.0f, -5.0f }, linethickness, color);

    if (inputs[6].target && inputs[6].target->state) color = { 255, 0, 0, 255 };
    else color = ColorBrightness(GRAY, -0.8f);
    DrawLineBezier(pos + Vector2{ -70.0f, 0.0f }, pos + Vector2{ 70.0f, -0.0f }, linethickness, color);

}

void SevenSegmentDisplay::recompute_size()
{
    size = Vector2{ 200, 350 };
}

void PushButton::not_clicked()
{
    has_changed = false;
    for (size_t i = 0; i < outputs.size(); i++) {
        if (outputs[i].state) has_changed = true;
        set_output_state(i, false);
    }
}

void PushButton::clicked(Vector2 pos)
{
    has_changed = false;
    for (size_t i = 0; i < outputs.size(); i++) {
        if (!outputs[i].state) has_changed = true;
        if (CheckCollisionPointRec(pos, getButtonRect(i))) set_output_state(i, true);
        else set_output_state(i, false);

    }
}

void Bus::pretick()
{
    if (!*bus_values_has_updated) {
        for (size_t i = 0; i < (*bus_values).size(); i++) {
            (*bus_values)[i] = false;
        }
    }
    for (size_t i = 0; i < inputs.size(); i++) {
        if (inputs[i].target && inputs[i].target->state)
            (*bus_values)[i] = true;
    }
    *bus_values_has_updated = true;
}

void Bus::tick()
{
    has_changed = false;
    *bus_values_has_updated = false;
    for (size_t i = 0; i < outputs.size(); i++) {
        if ((*bus_values)[i] != outputs[i].state) {
            has_changed = true;
            outputs[i].state = (*bus_values)[i];
        }
    }
}

json Bus::to_JSON() const {

    json jOutputs = json::array();
    for (const auto& output : outputs) {
        jOutputs.push_back(output.to_JSON());
    }

    json jInputs = json::array();
    for (const auto& input : inputs) {
        jInputs.push_back(input.to_JSON());
    }

    json myJson =
    {

        {get_type_str(),
            {
                {"pos.x", pos.x},
                {"pos.y", pos.y},
                {"size.x", size.x},
                {"size.y", size.y},
                {"label", label},
                {"outputs", jOutputs},
                {"inputs", jInputs},
                {"bus_values", json::array()}
            }
        }
    };
    for (bool val : (*bus_values)) {
        myJson[get_type_str()]["bus_values"].push_back(val);
    }

    return myJson;
}

void Bus::load_extra_JSON(const json& nodeJson) {
    find_connections();

    if (nodeJson.contains(get_type_str()) && nodeJson[get_type_str()].contains("bus_values")) {
        std::vector<bool> loaded_bus_vals = nodeJson[get_type_str()]["bus_values"].get<std::vector<bool>>();

        if (loaded_bus_vals.size() >= bus_values->size()) {
            *bus_values = loaded_bus_vals;
        }
        else {
            for (size_t i = 0; i < loaded_bus_vals.size(); i++) {
                (*bus_values)[i] = loaded_bus_vals[i];
            }
        }

    }
}

void Bus::load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr)
{
    find_connections();
}
