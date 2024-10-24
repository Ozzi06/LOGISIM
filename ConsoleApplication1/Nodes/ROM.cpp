#include "ROM.h"
#include "game.h"
#include "raygui.h"
#include "save_game.h"
#include "vector_tools.h"
#include <file_dialogs.h>

bool ROMNode::show_node_editor()
{
    Game& game = Game::getInstance();
    Vector2 Pos = GetWorldToScreen2D(pos + Vector2{ size.x / 2 , 0 }, game.camera);

    const static float area_width = 176;
    const static float margin = 4;
    const static float content_w = area_width - 2 * margin;

    static float area_height = 232;

    Rectangle area = { Pos.x + 0, Pos.y + 0, area_width, area_height };

    static bool TextBoxNodeLabelEditMode = false;
    const static size_t buffersize = 256;
    char TextBoxNodeLabel[256] = "";
    strcpy_s(TextBoxNodeLabel, buffersize, label.c_str());

    GuiPanel(area, "Node Settings");

    float curr_el_h;
    float current_depth = 30;

    {   // label
        curr_el_h = 32;
        float current_x = Pos.x + margin;
        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 32, 16 }, "Label:");
        current_x += 32 + margin;

        if (GuiTextBox(Rectangle{ current_x, Pos.y + current_depth, Pos.x + margin + content_w - current_x, curr_el_h }, TextBoxNodeLabel, buffersize, TextBoxNodeLabelEditMode))
            TextBoxNodeLabelEditMode = !TextBoxNodeLabelEditMode;
        label = TextBoxNodeLabel;
        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, "Change data");
        current_depth += curr_el_h;
    }

    {   // Button
        curr_el_h = 50;
        float current_x = Pos.x + margin;
        const char* label = "#01#";
        if (GuiButton(Rectangle{ current_x, Pos.y + current_depth, Pos.x + margin + content_w - current_x, curr_el_h }, label)) {
            grab_new_data();
        }
        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    {   // Connectors
        curr_el_h = 32;
        float current_x = Pos.x + margin;

        GuiLabel(Rectangle{ current_x, Pos.y + current_depth, 64, 32 }, "Connectors:");
        current_x += 64 + margin;

        if (GuiButton(Rectangle{ current_x, Pos.y + current_depth, 32, 32 }, "#121#")) add_input();
        current_x += 32 + margin;

        if (GuiButton(Rectangle{ current_x, Pos.y + current_depth, 32, 32 }, "#120#")) remove_input();
        current_x += 32 + margin;

        current_depth += curr_el_h;
    }

    {   // Spacing line
        curr_el_h = 15;
        GuiLine(Rectangle{ Pos.x, Pos.y + current_depth, area_width, curr_el_h }, NULL);
        current_depth += curr_el_h;
    }

    area_height = current_depth;
    return CheckCollisionPointRec(GetMousePosition(), area);
}

void ROMNode::grab_new_data()
{
    Game& game = Game::getInstance();

    std::filesystem::path filepath = open_file_dialog_hex();
    std::ifstream dataFile(filepath, std::ios::binary);

    if (dataFile.is_open() && filepath.extension() == ".hex") {
        std::vector<uint8_t> data;
        {
            dataFile.seekg(0, std::ios::end);
            std::streamsize size = dataFile.tellg();
            data.resize(size);
            dataFile.seekg(0, std::ios::beg);
            dataFile.read(reinterpret_cast<char*>(data.data()), size);
        }

        data_save = data;
        recompute_input_connector_count();
    }
    else if (dataFile.is_open()) {
        std::cerr << "invalid path specified";
    }
    else {
        std::cerr << "could not open selected file";
    }
    dataFile.close();
}

inline void ROMNode::set_data_save(const std::vector<uint8_t>& data) {
    data_save = data;
    Game& game = Game::getInstance();
    game.network_change();
}
