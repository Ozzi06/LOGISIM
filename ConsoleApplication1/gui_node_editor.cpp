#include "gui_node_editor.h"

#include "main_game.h"

#include "vector_tools.h"
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include <cassert>

GuiNodeEditorState::GuiNodeEditorState() : node(nullptr)
{}

GuiNodeEditorState::GuiNodeEditorState(Node* node) : node(node) {
    strcpy(TextBoxNodeLabel, node->label.c_str());
}

void GuiNodeEditorState::labelUpdated()
{
    node->label = TextBoxNodeLabel;
}

void GuiNodeEditorState::ButtonDeleteNode()
{
    Game& game = Game::getInstance();
    game.remove_node(node);
    delete node;
    return;
}

void GuiNodeEditorState::ButtonCopyNode()
{
}

void GuiNodeEditorState::ButtonRotateCW()
{
}

void GuiNodeEditorState::ButtonRotateCCW()
{
}

void GuiNodeEditorState::ButtonDecreaseInputCount()
{
    node->add_input();
}

void GuiNodeEditorState::ButtonIncreaseInputCount()
{
    node->remove_input();
}

bool GuiNodeEditor(GuiNodeEditorState* state)
{
    assert(state->node);
    Game game = Game::getInstance();
    Vector2 Pos = GetWorldToScreen2D(state->node->pos + Vector2{ state->node->size.x / 2 , 0 }, game.camera);
    Rectangle area = { Pos.x + 0, Pos.y + 0, 176, 232 };

    GuiPanel(area, "Node Settings");
    if (GuiButton(Rectangle{ Pos.x + 32, Pos.y + 32, 56, 40 }, "#143#")) {
        state->ButtonDeleteNode();
        return true;
    }
    if (GuiButton(Rectangle{ Pos.x + 104, Pos.y + 32, 56, 40 }, "#016#")) state->ButtonCopyNode();
    GuiLine(Rectangle{ Pos.x + 0, Pos.y + 120, 176, 12 }, NULL);

    GuiLabel(Rectangle{ Pos.x + 8, Pos.y + 88, 48, 32 }, "Rotate:");
    if (GuiButton(Rectangle{ Pos.x + 56, Pos.y + 88, 32, 32 }, "#073#")) state->ButtonRotateCW();
    if (GuiButton(Rectangle{ Pos.x + 104, Pos.y + 88, 32, 32 }, "#072#")) state->ButtonRotateCCW();
    GuiLine(Rectangle{ Pos.x + 0, Pos.y + 72, 176, 16 }, NULL);

    GuiLabel(Rectangle{ Pos.x + 8, Pos.y + 136, 48, 32 }, "Inputs:");
    if (GuiButton(Rectangle{ Pos.x + 56, Pos.y + 136, 32, 32 }, "#121#")) state->ButtonDecreaseInputCount();
    if (GuiButton(Rectangle{ Pos.x + 104, Pos.y + 136, 32, 32 }, "#120#")) state->ButtonIncreaseInputCount();
    GuiLine(Rectangle{ Pos.x + 0, Pos.y + 168, 176, 12 }, NULL);

    GuiLabel(Rectangle{ Pos.x + 8, Pos.y + 176, 48, 16 }, "Label:");
    if (GuiTextBox(Rectangle{ Pos.x + 8, Pos.y + 192, 160, 32 }, state->TextBoxNodeLabel, 32, state->TextBoxNodeLabelEditMode)) state->TextBoxNodeLabelEditMode = !state->TextBoxNodeLabelEditMode;
    state->labelUpdated();

    return CheckCollisionPointRec(GetMousePosition(), area);
}
