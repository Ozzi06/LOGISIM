#include "Buttons.h"
#include "game.h"
#include "save_game.h"

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
        if (outputs[i].get_state())
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
    Rectangle rec{
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
    const NodeData* nodedata = reinterpret_cast<const NodeData*>(node_data_ptr);
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