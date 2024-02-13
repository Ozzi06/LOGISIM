#include "gui_ui.h"

#include "main_game.h"

#include "vector_tools.h"
#undef RAYGUI_IMPLEMENTATION
#include "raygui.h"
#include "file_dialogs.h"
#include <fstream>

void edit_mode_changed() {
    Game::getInstance().unselect_all();
}

bool EditModeButtons() {
    Game& game = Game::getInstance();

    float edit_mode_area_width = 300;
    float edit_mode_area_height = 50;
    Rectangle edit_mode_area{ 5, game.screenHeight - edit_mode_area_height - 5, edit_mode_area_width, edit_mode_area_height };
    GuiGroupBox(edit_mode_area, "Edit mode");

    static int edit_mode;
    GuiToggleGroup(
        { edit_mode_area.x + 5, edit_mode_area.y + 5, (edit_mode_area_width - 15) / 4.0f , edit_mode_area_height - 10 },
        "SELECT; EDIT; CONNECT; INTERACT",
        &edit_mode
    );


    if (game.edit_mode != (EditMode)edit_mode) {
        edit_mode_changed();
        game.edit_mode = (EditMode)edit_mode;
    }

    return CheckCollisionPointRec(GetMousePosition(), edit_mode_area);
}

bool SpeedControlButtons() {
    Game& game = Game::getInstance();

    float speed_control_area_width = 400, speed_control_area_height = 50;

    Rectangle speed_control_area{ 5 + speed_control_area_width + 5, game.screenHeight - speed_control_area_height - 5, speed_control_area_width, speed_control_area_height };
    GuiGroupBox(speed_control_area, "simulation control");
    Rectangle slider_bar_area{ speed_control_area.x + 10, speed_control_area.y + 10, speed_control_area.width - 50 - 20, speed_control_area.height - 20 };

    static float selected_speed = 3;
    GuiSliderBar(slider_bar_area, "", game.warp ? "warp" : num_toString(game.targ_sim_hz, 1).c_str(), &selected_speed, -1, 8);
    if (selected_speed == -1)
        game.targ_sim_hz = 0;
    else
        game.targ_sim_hz = powf(10, selected_speed);
    game.warp = selected_speed >= 8;

    return CheckCollisionPointRec(GetMousePosition(), speed_control_area);
}

bool MenuAreaButtons() {
    Game& game = Game::getInstance();

    float menu_area_w = 400, menu_area_h = 50;
    Rectangle menu_area{ 10, 10, menu_area_w, menu_area_h };
    GuiGroupBox(menu_area, "");

    Rectangle load_button_area{ menu_area.x + 10, menu_area.y + 10, menu_area.width / 2 - 20, menu_area.height - 20 };
    if (GuiButton(load_button_area, "#01#")) game.load(open_file_dialog_json());

    Rectangle save_button_area{ menu_area.x + menu_area.width / 2 + 10, menu_area.y + 10, menu_area.width / 2 - 20, menu_area.height - 20 };
    if (GuiButton(save_button_area, "#02#")) game.save(ShowSaveFileDialogJson());

    return CheckCollisionPointRec(GetMousePosition(), menu_area);
}

bool NodeSelectionMenu() {

    Game& game = Game::getInstance();

    float menu_area_w = 200.0f, menu_area_h = game.screenHeight - 200.0f;
    Vector2 menu_area_pos = { game.screenWidth - menu_area_w, 100 };

    Rectangle menu_area = { menu_area_pos.x, menu_area_pos.y, menu_area_w, menu_area_h };
    float content_w = menu_area_w - 15;
    static Rectangle panelContentRec = { 0, 0, content_w, menu_area_h};
    Rectangle panelView = { 0 };
    static Vector2 panelScroll = { 0, 0 };

    {
        GuiScrollPanel(menu_area, "Add Nodes", panelContentRec, &panelScroll, &panelView);

        BeginScissorMode((int)panelView.x, (int)panelView.y, (int)panelView.width, (int)panelView.height);
        //GuiGrid(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y, content_w, panelContentRec.height }, NULL, 16, 3, NULL);

        float curr_el_h;
        float current_depth = 30;

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Add function");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 50;
            const char* label = "#01#";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                
                {
                    std::ifstream saveFile;
                    saveFile.open(open_file_dialog_json());

                    if (!saveFile.is_open()) goto ouside_if;

                    json save;
                    saveFile >> save;
                    saveFile.close();
                    game.nodes.push_back(new FunctionNode(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
                    auto it = game.nodes.end();
                    it--;
                    (*it)->load_JSON(save);
                }
            }
        ouside_if:
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Add Subassembly");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 50;
            const char* label = "#01#";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {

                {
                    std::ifstream saveFile;
                    saveFile.open(open_file_dialog_json());

                    if (!saveFile.is_open()) goto ouside_if2;

                    json save;
                    saveFile >> save;
                    saveFile.close();

                    std::vector<Node*> subassembly; 
                    NodeNetworkFromJson(save.at("nodes"), subassembly);

                    NormalizeNodeNetworkPosTocLocation(subassembly, game.camera.target);

                    game.nodes.insert(game.nodes.end(), subassembly.begin(), subassembly.end());
                }
            }
        ouside_if2:
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 30;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "IO");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "PushButton";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new PushButton(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, NULL);
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "ToggleButton";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new ToggleButton(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, NULL);
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "StaticToggleButton";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new StaticToggleButton(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, NULL);
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "LightBulb";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new LightBulb(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 30;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Gates");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateAND";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateAND(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Gates");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateOR";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateOR(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Gates");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateNAND";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateNAND(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Gates");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateNOR";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateNOR(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Gates");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateXOR";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateXOR(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, "Gates");
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateXNOR";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateXNOR(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, NULL);
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateBUFFER";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateBUFFER(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        {   // Spacing line
            curr_el_h = 15;
            GuiLine(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, NULL);
            current_depth += curr_el_h;
        }

        {   // Button
            curr_el_h = 30;
            const char* label = "GateNOT";
            if (GuiButton(Rectangle{ menu_area.x + panelScroll.x, menu_area.y + panelScroll.y + current_depth, content_w, curr_el_h }, label)) {
                game.nodes.push_back(new GateNOT(GetScreenToWorld2D({ game.screenWidth / 2.0f, game.screenHeight / 2.0f }, game.camera)));
            }
            current_depth += curr_el_h;
        }

        panelContentRec.height = current_depth;
        EndScissorMode();

        //DrawRectangle(menu_area.x + panelScroll.x, menu_area.y + panelScroll.y, content_w, panelContentRec.height, Fade(RED, 0.1));

    }

    return CheckCollisionPointRec(GetMousePosition(), menu_area);
}

bool GuiUi()
{
    Game& game = Game::getInstance();
    bool does_hover = false;

    if (EditModeButtons()) does_hover = true;
    if (SpeedControlButtons()) does_hover = true;
    if (MenuAreaButtons()) does_hover = true;
    if (NodeSelectionMenu()) does_hover = true;
    return does_hover;
}
