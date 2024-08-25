#include "UnaryGates.h"
#include "game.h"

Texture GateBUFFER::texture = Texture{ 0 };

Texture GateNOT::texture = Texture{ 0 };

void UnaryLogicGate::add_input() {
    if (outputs.size() == outputs.capacity()) return;
    inputs.push_back(Input_connector(this, inputs.size(), ""));
    outputs.push_back(Output_connector(this, outputs.size(), "", false));
    recompute_size();
    Game& game = Game::getInstance();
    game.network_change();
}

void UnaryLogicGate::remove_input() {
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
    game.network_change();
}