#pragma once

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "game.h"
#include "BinaryGates.h"
#include "UnaryGates.h"
//#define GUI_WINDOW_HELP_IMPLEMENTATION
//#include "gui_window_help.h"

#include <fstream>
#include "vector_tools.h"

void LoadTextures() {
    Image img = LoadImage("resources/logic_gates/AND.png");
    ImageColorInvert(&img);
    GateAND::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateAND::texture, TEXTURE_FILTER_BILINEAR);

    img = LoadImage("resources/logic_gates/OR.png");
    ImageColorInvert(&img);
    GateOR::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateOR::texture, TEXTURE_FILTER_BILINEAR);

    img = LoadImage("resources/logic_gates/NAND.png");
    ImageColorInvert(&img);
    GateNAND::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateNAND::texture, TEXTURE_FILTER_BILINEAR);

    img = LoadImage("resources/logic_gates/NOR.png");
    ImageColorInvert(&img);
    GateNOR::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateNOR::texture, TEXTURE_FILTER_BILINEAR);

    img = LoadImage("resources/logic_gates/XOR.png");
    ImageColorInvert(&img);
    GateXOR::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateXOR::texture, TEXTURE_FILTER_BILINEAR);

    img = LoadImage("resources/logic_gates/XNOR.png");
    ImageColorInvert(&img);
    GateXNOR::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateXNOR::texture, TEXTURE_FILTER_BILINEAR);

    img = LoadImage("resources/logic_gates/BUFFER.png");
    ImageColorInvert(&img);
    GateBUFFER::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateBUFFER::texture, TEXTURE_FILTER_BILINEAR);

    img = LoadImage("resources/logic_gates/NOT.png");
    ImageColorInvert(&img);
    GateNOT::texture = LoadTextureFromImage(img);
    SetTextureFilter(GateNOT::texture, TEXTURE_FILTER_BILINEAR);
}

int run_game()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    Game& game = Game::getInstance();
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);    // Window configuration flags
    InitWindow(game.screenWidth, game.screenHeight, "raylib [core] example - 2d camera mouse zoom");
    LoadFontEx("resources/fonts/OpenSans-Regular.ttf", 128, 0, 0);
    game.regular = LoadFont("resources/fonts/OpenSans-Regular.ttf");

    LoadTextures();
    game.camera.zoom = 1.0f;
    SetTextureFilter(game.regular.texture, TEXTURE_FILTER_POINT);

    rlEnableSmoothLines();

    game.camera.offset = { game.screenWidth / 2.0f , game.screenHeight / 2.0f };


    float width = 500.0f;
    float height = 500.0f;

    // Main game loop

    double draw_frequency = 60;
    double t = GetTime();
    double prev_draw_t = 0;


    double expected_updates = 0.0;
    double pre_update_t = 0;
    RollingAverage roller(60);

    SetTargetFPS(draw_frequency);
    while (!WindowShouldClose())
    {
        {
            pre_update_t = t;
        }
        size_t i = 0;
        double duration;
        double elapsed_time = 0;
        if (expected_updates >= 1.0) {
            t = GetTime();
            game.pretick();
            game.tick();
            i++;
            expected_updates--;
            duration = GetTime() - t;
            elapsed_time += duration;
        }
        while (expected_updates >= 1.0 && elapsed_time < 1.0f / draw_frequency) {
            game.pretick();
            game.tick();
            expected_updates--;
            i++;
            elapsed_time += duration;
        }

        game.draw();

        t = GetTime();
        expected_updates += (t - pre_update_t) * game.targ_sim_hz;
        if (expected_updates > game.targ_sim_hz * 5)
            expected_updates = game.targ_sim_hz * 5;

        roller.add(i / (t - pre_update_t));

        game.real_sim_hz = roller.getAverage();
        i = 0;

    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    return 0;
}

int main()
{
    return run_game();
}