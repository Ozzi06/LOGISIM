#include "Buttons.h"
#include "game.h"
// #include "save_game.h"

void Button::draw()
{
    Game& game = Game::getInstance();
    float roundness = 0.1f;
    int segments = 50;
    float lineThick = 10; 
    Rectangle totalRec = { pos.x - size.x / 2, pos.y - size.y / 2, size.x, size.y };

    size_t button_count = outputs.size();
    float segmentHeight = size.y / button_count;
    float startY = pos.y - size.y / 2.0f;

    // 1. DRAW ALL FILLS
    for (size_t i = 0; i < button_count; i++) {
        Rectangle rec = getButtonRect(i);
        Color btnColor = outputs[i].get_state() ? Color{ 219, 42, 2, 255 } : Color{ 252, 57, 13, 255 };
        DrawRectangleRec(rec, btnColor);
    }

    // 2. DRAW BOUNDARY LINES
    // Instead of thinking "Bottom of button i", we think "Boundary between i and i+1"
    if (game.camera.zoom > 0.4f && button_count > 1) {
        for (size_t i = 1; i < button_count; i++) {
            // The boundary is exactly at Start + (index * height)
            float boundaryY = startY + (i * segmentHeight);
            
            DrawLineEx(
                { totalRec.x, boundaryY }, 
                { totalRec.x + totalRec.width, boundaryY }, 
                4.0f, // This line now sits 2px into the button above and 2px into the button below
                ColorBrightness(color, -0.4f)
            );
        }
    }

    // 3. DRAW NODE BORDER (Drawn last so it frames the buttons)
    if (game.camera.zoom > 1 / 10.0f) {
        Color borderColor = is_selected ? ColorBrightness(GREEN, -0.3f) : ColorBrightness(color, -0.2f);
        DrawRectangleRoundedLinesEx(totalRec, roundness, segments, lineThick, borderColor);
    }

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
    float segmentHeight = size.y / outputs.size();
    Rectangle rec{
        pos.x - size.x / 2.0f,
        // Start at the very top of the node and move down segment by segment
        (pos.y - size.y / 2.0f) + (id * segmentHeight),
        size.x, 
        segmentHeight
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
        assert(false);
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
            set_output_state(i, !outputs[i].get_state());
            has_changed = true;
        }
    }
}

void ToggleButton::load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr)
{
    // TODO: inspect what happens here
    // const NodeData* nodedata = reinterpret_cast<const NodeData*>(node_data_ptr);
}

void PushButton::not_clicked()
{
    has_changed = false;
    for (size_t i = 0; i < outputs.size(); i++) {
        if (outputs[i].get_state()) has_changed = true;
        set_output_state(i, false);
    }
}

void PushButton::clicked(Vector2 pos)
{
    has_changed = false;
    for (size_t i = 0; i < outputs.size(); i++) {
        if (!outputs[i].get_state()) has_changed = true;
        if (CheckCollisionPointRec(pos, getButtonRect(i))) set_output_state(i, true);
        else set_output_state(i, false);

    }
}
