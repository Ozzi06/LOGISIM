#pragma once
#include "raylib.h"
#include <string>

struct Node;

struct GuiNodeEditorState {
    GuiNodeEditorState();
    GuiNodeEditorState(Node* node);
    Node* node;
    char TextBoxNodeLabel[256] = "";
    bool TextBoxNodeLabelEditMode = false;

    void labelUpdated();

    void ButtonDeleteNode();

    void ButtonCopyNode();
    void ButtonRotateCW();
    void ButtonRotateCCW();
    void ButtonDecreaseInputCount();
    void ButtonIncreaseInputCount();
};


bool GuiNodeEditor(GuiNodeEditorState* state);
