#include "game.h"
#include "vector_tools.h"
#include "save_game.h"
#include "file_dialogs.h"
#include "FunctionNode.h"

Game Game::instance;

void Game::draw() {
    // Draw world
    screenWidth = GetScreenWidth();
    screenHeight = GetScreenHeight();

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(camera);

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
    else {
        if (logicblock) {
            logicblock->pretick(logicblock->get_data(0), has_updated, 0);
            return;
        }
        else {
            run_on_block = false;
            std::cout << "no serialized block, turning off run on block\n";
        }
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

void Game::add_function_node()
{
    std::filesystem::path filepath = open_file_dialog_json_bin();
    std::ifstream saveFile(filepath, std::ios::binary);

    if (saveFile.is_open() && filepath.extension() == ".bin") {
        std::vector<uint8_t> save;
        {
            saveFile.seekg(0, std::ios::end);
            std::streamsize size = saveFile.tellg();
            save.resize(size);
            saveFile.seekg(0, std::ios::beg);
            saveFile.read(reinterpret_cast<char*>(save.data()), size);
        }

        SaveHeader* saveheader = reinterpret_cast<SaveHeader*>(save.data());
        assert(saveheader->version.version_number == 0);

        FunctionNode* funnode = new FunctionNode(&nodes, GetScreenToWorld2D({ screenWidth / 2.0f, screenHeight / 2.0f }, camera));
        nodes.push_back(funnode);

        //load network
        size_t curr_node_offset = saveheader->Nodes_offset;

        FunctionNodeHeader* logic_block_root = reinterpret_cast<FunctionNodeHeader*>(save.data() + saveheader->LogicBlock_offset);
        assert(logic_block_root->type == NodeType::RootFunctionNode || logic_block_root->type == NodeType::FunctionNode);

        logic_block_root->type = NodeType::FunctionNode;

        funnode->label = filepath.stem().string();
        funnode->pos = { 0, 0 };
        funnode->size = { 100, 100 };

        //update connectors
        funnode->inputs.clear();
        funnode->outputs.clear();
        std::vector<NodeData*> input_targs;
        std::vector<NodeData*> output_targs;

        {
            size_t curr_nodedata_offset = saveheader->Nodes_offset;
            for (size_t _ = 0; _ < saveheader->node_count; ++_) {
                NodeData* nodedata = reinterpret_cast<NodeData*>(save.data() + curr_nodedata_offset);
                if (isInputType(nodedata->type)) {
                    input_targs.push_back(nodedata);
                }
                if (isOutputType(nodedata->type)) {
                    output_targs.push_back(nodedata);
                }
                curr_nodedata_offset += nodedata->total_size;
            }
        }

        std::sort(input_targs.begin(), input_targs.end(), [](NodeData* a, NodeData* b) {
            return a->pos.y > b->pos.y; // Return true if 'a' should come before 'b'
            });

        std::sort(output_targs.begin(), output_targs.end(), [](NodeData* a, NodeData* b) {
            return a->pos.y > b->pos.y; // Return true if 'a' should come before 'b'
            });

        for (NodeData* nodedata : input_targs) {
            for (size_t __ = 0; __ < nodedata->output_count; ++__) {
                funnode->inputs.push_back(Input_connector(funnode, funnode->inputs.size(), nodedata->label, nullptr, 0));
            }
        }
        for (NodeData* nodedata : output_targs) {
            for (size_t i = 0; i < nodedata->input_count; ++i) {
                const OutputData* outputdata = reinterpret_cast<const OutputData*>(save.data() + nodedata->outputs_offset + i * sizeof(OutputData));
                funnode->outputs.push_back(Output_connector(funnode, funnode->outputs.size(), nodedata->label, outputdata->id));
            }
        }



        const uint8_t* funheader_ptr = save.data() + saveheader->LogicBlock_offset;
        funnode->recompute_size();
        funnode->allocate_node_data_save(funheader_ptr);
        network_change();
    }
    else if (saveFile.is_open() && filepath.extension() == ".json") {
        json save;
        saveFile >> save;
        saveFile.close();
        FunctionNode* funnode = new FunctionNode(&nodes, GetScreenToWorld2D({ screenWidth / 2.0f, screenHeight / 2.0f }, camera));
        nodes.push_back(funnode);
        funnode->load_JSON(save);
        network_change();
    }
    else if (saveFile.is_open()) {
        std::cerr << "invalid path specified";
    }
    else {
        std::cerr << "could not open selected file";
    }
    saveFile.close();
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
        std::ifstream saveFile(filepath, std::ios::binary);
        if (saveFile.is_open()) {
            json save;
            saveFile >> save;
            saveFile.close();
            NodeNetworkFromJson(save.at("nodes"), &subassembly);
        }
    }

    NormalizeNodeNetworkPosToLocation(subassembly, camera.target);

    for (Node* node : subassembly) {
        node->set_container(&nodes);
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
                for (auto node : nodes) {

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

    for (Node* node : nodes)
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

void Game::save_bin(std::string filePath)
{
    SaveBuilder savebuilder = SaveBuilder();
    savebuilder.save_game(filePath);
}

void sort_nodes(std::vector<Node*>& nodes);

void Game::build_logic_block()
{
    // save current state before rebuild
    for (Node* node : nodes) {
        if (!node->has_offset()) continue;
        FunctionNode* funnode = dynamic_cast<FunctionNode*>(node);
        if (funnode && funnode->has_node_data_save()) {
            size_t abs_offset = funnode->get_abs_node_offset();
            FunctionNodeHeader* header = get_logicblock<FunctionNodeHeader>(abs_offset);
            assert(header->type == NodeType::FunctionNode);
            uint8_t* function_data = reinterpret_cast<uint8_t*>(header);
            funnode->allocate_node_data_save(function_data);
        }
    }


    // Record start time
    auto start = std::chrono::high_resolution_clock::now();

    //sort nodes
    std::vector<Node*> storted_nodes = nodes;
    sort_nodes(storted_nodes);

    LogicBlockBuilder builder;
    builder.add_function_root(storted_nodes);
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

static void sort_nodes(std::vector<Node*>& nodes)
{

    enum NodeState {
        Unvisited,
        Visiting,  // Node is being visited (used for cycle detection)
        Visited    // Node has been fully visited
    };

    std::unordered_map<Output_connector*, std::pair<int, NodeState>> marked_outconns;
    int max_delay = 1;
    std::function<void(Output_connector*)> DFS;

    

    DFS = [&](Output_connector* outconn) {
        if (!outconn || max_delay == -1) return;


        if (marked_outconns[outconn].second == NodeState::Visiting) {
            max_delay = -1;
            return;
        }
        if (marked_outconns[outconn].second == NodeState::Visited) {
            return;
        }

        if (outconn->host->is_cyclic()) {
            max_delay = -1;
            return;
        }

        marked_outconns[outconn].second = NodeState::Visiting;

        if (outconn->host->isInput()) return;

        int current_delay = marked_outconns[outconn].first + outconn->host->delay();
        if (current_delay > max_delay) max_delay = current_delay;

        for (Input_connector* inconn : outconn->host->connected_inputs(outconn->index)) {
            if (marked_outconns[inconn->target].first < current_delay) {
                marked_outconns[inconn->target].first = current_delay;
                DFS(inconn->target);
                if (max_delay == -1) return;
            }
        }

        marked_outconns[outconn].second = NodeState::Visited;

        };

    for (Node* node : nodes) {
        if (node->isOutput()) {
            for (Input_connector& inconn : node->inputs) {
                DFS(inconn.target);
            }
        }
    }
    if (max_delay != -1) {
        std::sort(nodes.begin(), nodes.end(), [&](Node* a, Node* b) {
            size_t max_a = 0;
            for (Output_connector& out : a->outputs) {
                if (marked_outconns[&out].first > max_a) max_a = marked_outconns[&out].first;
            }
            size_t max_b = 0;
            for (Output_connector& out : b->outputs) {
                if (marked_outconns[&out].first > max_b) max_b = marked_outconns[&out].first;
            }
            return max_a > max_b; // Return true if 'a' should come before 'b'
            });
    }


    //sort inputs
    std::sort(nodes.begin(), nodes.end(), [](const Node* a, const Node* b) {
        if (a->isInput() != b->isInput()) {
            return a->isInput(); // Inputs go to the top
        }
        if (a->isInput() && b->isInput()) {
            return a->pos.y < b->pos.y; // Sort inputs by posY
        }
        return false; // Keep non-inputs in their original order
        });
    //sort outputs
    std::sort(nodes.begin(), nodes.end(), [](const Node* a, const Node* b) {
        if (a->isOutput() != b->isOutput()) {
            return !a->isOutput(); // Outputs go to the bottom
        }
        if (a->isOutput() && b->isOutput()) {
            return a->pos.y > b->pos.y; // Sort outputs by posY
        }
        return false; // Keep non-inputs in their original order
        });
}