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

json Bus::to_JSON() const {

    json jOutputs = json::array();
    for (const auto& output : outputs) {
        jOutputs.push_back(output.to_JSON());
    }

    json jInputs = json::array();
    for (const auto& input : inputs) {
        jInputs.push_back(input.to_JSON());
    }

    json myJson =
    {

        {get_type_str(),
            {
                {"pos.x", pos.x},
                {"pos.y", pos.y},
                {"size.x", size.x},
                {"size.y", size.y},
                {"label", label},
                {"outputs", jOutputs},
                {"inputs", jInputs},
                {"bus_values", json::array()}
            }
        }
    };
    for (bool val : (*bus_values)) {
        myJson[get_type_str()]["bus_values"].push_back(val);
    }

    return myJson;
}

void Bus::load_extra_JSON(const json& nodeJson) {
    find_connections();

    if (nodeJson.contains(get_type_str()) && nodeJson[get_type_str()].contains("bus_values")) {
        std::vector<bool> loaded_bus_vals = nodeJson[get_type_str()]["bus_values"].get<std::vector<bool>>();

        if (loaded_bus_vals.size() >= bus_values->size()) {
            *bus_values = loaded_bus_vals;
        }
        else {
            for (size_t i = 0; i < loaded_bus_vals.size(); i++) {
                (*bus_values)[i] = loaded_bus_vals[i];
            }
        }

    }
}

void Bus::load_extra_bin(const uint8_t* node_data_ptr, const uint8_t* save_ptr)
{
    find_connections();
}
