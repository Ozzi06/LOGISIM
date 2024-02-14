#pragma once

#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "vector_tools.h"
#include "main_game.h"
//#define GUI_WINDOW_HELP_IMPLEMENTATION
//#include "gui_window_help.h"

#include <fstream>

enum class type {
    AND,
    NAND,
    OR,
    NOR,
    XOR,
    XNOR,
    NOT,
    BUFFER,
    CONST_0,
    CONST_1,
    OUTPUT,
    FUNCTION,
    SWITCH,
    BUTTON
};

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

int main()
{
    // Initialization
    //--------------------------------------------------------------------------------------
    Game& game = Game::getInstance();
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);    // Window configuration flags
    InitWindow(game.screenWidth, game.screenHeight, "raylib [core] example - 2d camera mouse zoom");

    game.regular = LoadFont("resources/fonts/OpenSans-Regular.ttf");

    LoadTextures();
    game.camera.zoom = 1.0f;
    SetTextureFilter(game.regular.texture, TEXTURE_FILTER_BILINEAR);

    rlEnableSmoothLines();

    game.camera.offset = { game.screenWidth / 2.0f , game.screenHeight / 2.0f };


    float width = 500.0f;
    float height = 500.0f;

    // Main game loop

    double draw_frequency = 60;
    double t = GetTime();
    double dt = 0;
    double prev_update_t = 0;
    double prev_draw_t = 0;
    double curr_time = GetTime();
    while (!WindowShouldClose())
    {
        {
            curr_time = GetTime();
            dt = t - curr_time;
            t = curr_time;
        }

        if (t - prev_update_t > 1 / game.targ_sim_hz) {
            size_t i = 0;
            for (; i < game.targ_sim_hz / draw_frequency; ++i) {
                game.pretick();
                game.tick();
            }
            game.real_sim_hz = i / (float)(t - prev_update_t);
            prev_update_t = t;
        }
        
        if (t - prev_draw_t > 1 / draw_frequency) {
            game.draw();
            prev_draw_t = t;
        }
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    return 0;
}