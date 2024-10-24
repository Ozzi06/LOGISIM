#include "FunctionNode.h"
#include "game.h"
#include "raygui.h"
#include "save_game.h"
#include "vector_tools.h"

FunctionNode::FunctionNode(const FunctionNode* base) : Node(base), is_single_tick(base->is_single_tick), is_cyclic_val(base->is_cyclic_val), node_data_save(base->node_data_save)
{
    nodes.clear();
    size_t* idxs = new size_t[base->nodes.size()];

    size_t idx = 0;
    for (size_t i = 0; i < base->nodes.size(); ++i) {
        nodes.push_back(base->nodes[i]->copy());
        nodes[i]->set_container(&nodes);
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

        if (is_cyclic_val.has_value() && is_cyclic_val.value()) {
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
        if (delay_str.empty())
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

        if (is_cyclic()) {}
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
    //TODO! move data saves to base class
    const SaveHeader* saveheader = reinterpret_cast<const SaveHeader*>(save_ptr);
    const NodeData* nodedata = reinterpret_cast<const NodeData*>(node_data_ptr);


    const FunctionNodeHeader* logic_block_root = reinterpret_cast<const FunctionNodeHeader*>(save_ptr + saveheader->LogicBlock_offset);
    assert(logic_block_root->type == NodeType::RootFunctionNode);
    const uint8_t* funheader_ptr = save_ptr + saveheader->LogicBlock_offset + nodedata->abs_node_offset;
    allocate_node_data_save(funheader_ptr);
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

void FunctionNode::allocate_node_data_save(const uint8_t* node) {
    using namespace LogicblockTools;
    const FunctionNodeHeader* header = get_at<const FunctionNodeHeader>(0, node);
    assert(header->type == NodeType::FunctionNode);

    node_data_save.allocate(header->total_size);
    std::memcpy(node_data_save.get_data(0), get_at<const uint8_t>(0, node), header->total_size);

    input_targs.clear();
    output_targs.clear();
    for (Node* node : nodes) {
        delete node;
    }
    nodes.clear();
}