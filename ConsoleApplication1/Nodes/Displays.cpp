#include "Displays.h"
#include "game.h"
#include "vector_tools.h"

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

    if (inputs[0].target && inputs[0].target->state) color = { 255, 0, 0, 255 };
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