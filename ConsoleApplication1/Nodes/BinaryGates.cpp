#include "BinaryGates.h"
#include "game.h"

Texture GateAND::texture = Texture{ 0 };

Texture GateOR::texture = Texture{ 0 };

Texture GateNAND::texture = Texture{ 0 };

Texture GateNOR::texture = Texture{ 0 };

Texture GateXOR::texture = Texture{ 0 };

Texture GateXNOR::texture = Texture{ 0 };

void BinaryLogicGate::add_input() {
    inputs.push_back(Input_connector(this, inputs.size(), "")); recompute_size();
    Game& game = Game::getInstance();
    game.network_change();
}

void BinaryLogicGate::remove_input() {
    if (inputs.size() > 1) inputs.pop_back();  recompute_size();
    Game& game = Game::getInstance();
    game.network_change();
}