/*******************************************************************************************
*
*   TestMenu v1.0.0 - just a simple test
*
*   MODULE USAGE:
*       #define GUI_TEST_MENU_IMPLEMENTATION
*       #include "gui_Test_menu.h"
*
*       INIT: GuiTestMenuState state = InitGuiTestMenu();
*       DRAW: GuiTestMenu(&state);
*
*   LICENSE: Propietary License
*
*   Copyright (c) 2022 ozzi. All Rights Reserved.
*
*   Unauthorized copying of this file, via any medium is strictly prohibited
*   This project is proprietary and confidential unless the owner allows
*   usage in any other form by expresely written permission.
*
**********************************************************************************************/

#include "raylib.h"

// WARNING: raygui implementation is expected to be defined before including this header
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <string.h>     // Required for: strcpy()

#ifndef GUI_TEST_MENU_H
#define GUI_TEST_MENU_H

typedef struct {
    int ListView000ScrollIndex;
    int ListView000Active;
    bool Toggle002Active;
    bool TextBox002EditMode;
    char TextBox002Text[128];
    Rectangle ScrollPanel004ScrollView;
    Vector2 ScrollPanel004ScrollOffset;
    Vector2 ScrollPanel004BoundsOffset;
    char ScrollPanel004Text[128];

    // Custom state variables (depend on development software)
    // NOTE: This variables should be added manually if required

} GuiTestMenuState;

#ifdef __cplusplus
extern "C" {            // Prevents name mangling of functions
#endif

//----------------------------------------------------------------------------------
// Module Functions Declaration
//----------------------------------------------------------------------------------
GuiTestMenuState InitGuiTestMenu(void);
void GuiTestMenu(GuiTestMenuState *state);

#ifdef __cplusplus
}
#endif

#endif // GUI_TEST_MENU_H

/***********************************************************************************
*
*   GUI_TEST_MENU IMPLEMENTATION
*
************************************************************************************/
#if defined(GUI_TEST_MENU_IMPLEMENTATION)

#include "raygui.h"

//----------------------------------------------------------------------------------
// Global Variables Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Internal Module Functions Definition
//----------------------------------------------------------------------------------
//...

//----------------------------------------------------------------------------------
// Module Functions Definition
//----------------------------------------------------------------------------------
GuiTestMenuState InitGuiTestMenu(void)
{
    GuiTestMenuState state = { 0 };

    state.ListView000ScrollIndex = 0;
    state.ListView000Active = 0;
    state.Toggle002Active = true;
    state.TextBox002EditMode = false;
    strcpy(state.TextBox002Text, "Hover Info");
    state.ScrollPanel004ScrollView = Rectangle{ 0, 0, 0, 0 };
    state.ScrollPanel004ScrollOffset = Vector2{ 0, 0 };
    state.ScrollPanel004BoundsOffset = Vector2{ 0, 0 };
    strcpy(state.ScrollPanel004Text, "Scroll_panel");

    // Custom variables initialization

    return state;
}

void GuiTestMenu(GuiTestMenuState *state)
{
    const char *ListView000Text = "ONE;TWO;THREE;FOUR";
    const char *Toggle002Text = "#002#";
    const char *GroupBox003Text = "SAMPLE TEXT";
    
    GuiListView(Rectangle{ 264, 240, 120, 88 }, ListView000Text, &state->ListView000ScrollIndex, &state->ListView000Active);
    GuiToggle(Rectangle{ 264, 168, 24, 24 }, Toggle002Text, &state->Toggle002Active);
    if (GuiTextBox(Rectangle{ 264, 192, 72, 16 }, state->TextBox002Text, 128, state->TextBox002EditMode)) state->TextBox002EditMode = !state->TextBox002EditMode;
    GuiGroupBox(Rectangle{ 240, 144, 168, 296 }, GroupBox003Text);
    GuiScrollPanel(
        Rectangle{ 264, 360, 120 - state->ScrollPanel004BoundsOffset.x, 72 - state->ScrollPanel004BoundsOffset.y }, //Bounds
        state->ScrollPanel004Text,                                                                                  //text
        Rectangle{264, 360, 120, 72},                                                                               //Content
        &state->ScrollPanel004ScrollOffset,                                                                         //scroll
        &state->ScrollPanel004ScrollView                                                                            //view
    );
}

#endif // GUI_TEST_MENU_IMPLEMENTATION
