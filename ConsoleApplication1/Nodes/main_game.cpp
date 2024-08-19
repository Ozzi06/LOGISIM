#include "main_game.h"
#include "game.h"
#include "vector_tools.h"
#include <fstream>
#include <cassert>

#include <iostream>

#include <algorithm>
#include "vector_tools.h"
#include "raygui.h"
#include "save_game.h"

Texture GateAND::texture = Texture{ 0 };

Texture GateOR::texture = Texture{ 0 };

Texture GateNAND::texture = Texture{ 0 };

Texture GateNOR::texture = Texture{ 0 };

Texture GateXOR::texture = Texture{ 0 };

Texture GateXNOR::texture = Texture{ 0 };

Texture GateBUFFER::texture = Texture{ 0 };

Texture GateNOT::texture = Texture{ 0 };

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

void Button::add_input() {
    if (outputs.size() == outputs.capacity()) return;
    outputs.push_back(Output_connector(this, outputs.size(), "")); recompute_size();
    Game& game = Game::getInstance();
    game.network_change();
}

void Button::remove_input() {
    if (outputs.size() > 1) {
        Game& game = Game::getInstance();
        for (Node* node : game.nodes) {
            for (auto& conn : node->inputs) {
                if (conn.target == &outputs.back()) conn.target = nullptr;
            }
        }
        outputs.pop_back();  recompute_size();
        game.network_change();
    }
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

void Button::set_output_state(size_t index, bool new_state) {
    Game& game = Game::getInstance();
    if (game.run_on_block && has_offset_val) {
        InputNodeHeader* header = game.get_logicblock<InputNodeHeader>(abs_node_offset);
        assert(header->outputs_offset == sizeof(InputNodeHeader));
        //TODO this is jank but should work, creates absolute offset as long as it's the child of a root node
        output* outconn = game.get_logicblock<output>(abs_node_offset + header->outputs_offset + index * sizeof(output));
        *outconn = new_state;
    }
    else {
        outputs[index].state = new_state;
        game.has_updated = true;
    }
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

FunctionNode::FunctionNode(const FunctionNode* base): Node(base), is_single_tick(base->is_single_tick), is_cyclic_val(base->is_cyclic_val), node_data_save(base->node_data_save)
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

SevenSegmentDisplay::SevenSegmentDisplay(std::vector<Node*>* container, Vector2 pos) : Node(container, pos, { 0, 0 }, ColorBrightness(GRAY, -0.7f)) {
    label = "7-Segment Disp";

    outputs.clear();
    inputs.clear();
    for (size_t i = 0; i < 7; i++)
        inputs.push_back(Input_connector(this, i, ""));
    recompute_size();
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

void Bus::add_input() {
    if (outputs.size() == outputs.capacity()) return;
    inputs.push_back(Input_connector(this, inputs.size(), ""));
    outputs.push_back(Output_connector(this, outputs.size(), "", false));
    recompute_size();
    find_connections();
    Game& game = Game::getInstance();
    game.network_change();
}

void Bus::remove_input() {
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
    game.network_change();
}

void Bus::change_label(const char* newlabel) {
    label = newlabel;
    find_connections();
    Game& game = Game::getInstance();
    game.network_change();
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

void BinaryLogicGate::add_input() {
    inputs.push_back(Input_connector(this, inputs.size(), "")); recompute_size();
    Game& game = Game::getInstance();
    game.network_change();
}

void BinaryLogicGate::remove_input() {
    if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    Game& game = Game::getInstance();
    game.network_change();
}

void UnaryLogicGate::add_input() {
    if (outputs.size() == outputs.capacity()) return;
    inputs.push_back(Input_connector(this, inputs.size(), ""));
    outputs.push_back(Output_connector(this, outputs.size(), "", false));
    recompute_size();
    Game& game = Game::getInstance();
    game.network_change();
}

void UnaryLogicGate::remove_input() {
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
    game.network_change();
}
