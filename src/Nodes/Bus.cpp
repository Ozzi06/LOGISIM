#include "Bus.h"
#include "game.h"

void Bus::add_input() {
    if (outputs.size() == outputs.capacity()) return;
    inputs.push_back(Input_connector(this, inputs.size(), ""));
    outputs.push_back(Output_connector(this, outputs.size(), "", false));
    recompute_size();
    find_connections();
    Game& game = Game::getInstance();
    game.network_change();
}

void Bus::remove_input() {
    Game& game = Game::getInstance();
    if (inputs.size() > 1) {
        inputs.pop_back();
        for (Node* node : game.nodes) {
            for (Input_connector& input : node->inputs) {
                if (input.target == &outputs.back()) input.target = nullptr;
            }
        }
        outputs.pop_back();
    }
    recompute_size();
    find_connections();
    game.network_change();
}

void Bus::change_label(const char* newlabel) {
    label = newlabel;
    find_connections();
    Game& game = Game::getInstance();
    game.network_change();
}

void Bus::load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr)
{
    find_connections();
}
