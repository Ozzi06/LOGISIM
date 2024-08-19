#include "Node.h"
#include "game.h"
#include "gui_ui.h"
#include "raygui.h"
#include "vector_tools.h"
#include "main_game.h"
#include "save_game.h"
#include "NodeFactory.h"

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

void Node::change_label(const char* newlabel) {
    label = newlabel;
    Game& game = Game::getInstance();
}

Node::Node(std::vector<Node*>* container, Vector2 pos, Vector2 size, Color color, std::vector<Input_connector> in, std::vector<Output_connector> out) : container(container), size(size), color(color), is_selected(false), inputs(in), outputs(out), pos(pos)
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

void NodeNetworkFromJson(const json& nodeNetworkJson, std::vector<Node*>* nodes) {
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

inline std::vector<Input_connector*> Node::connected_inputs(size_t output_idx) {
    std::vector<Input_connector*> input_nodes;
    for (size_t i = 0; i < inputs.size(); i++) {
        input_nodes.push_back(&inputs[i]);
    }
    return input_nodes;
}
