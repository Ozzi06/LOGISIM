#pragma once
#include "raylib.h" // Needed for Rectangle

// Returns true if state changed (ENTER pressed or clicked outside)
// Automatically sets Game::is_typing = true if editMode is on.
bool GuiTextBoxBlocking(Rectangle bounds, char *text, int textSize, bool editMode);

bool GuiUi();
