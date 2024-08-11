#include "save_game.h"

void SaveBuilder::save_game(std::string filePath)
{
    Game& game = Game::getInstance();

    if (filePath.empty()) {
        std::cout << "No file path selected\n";
        return;
    }

    std::filesystem::path filepath(filePath);
    std::string filename = filepath.stem().string();

    std::ofstream outputFile(filePath);

    if (!outputFile.is_open()) {
        std::cerr << "Error opening file for writing: " << filePath << std::endl;
        outputFile.close();
    }

    size_t saveheader_offs = current_absolute_offset;
    assert(current_absolute_offset == 0);
    add<SaveHeader>(SaveHeader());


    size_t nodes_offs = current_absolute_offset;

    {
        SaveHeader* saveheader = get_at_abs<SaveHeader>(saveheader_offs);
        saveheader->version = 0;
        std::strncpy(saveheader->namestring, "a binary savefile for logisim, v0.0", sizeof(saveheader->namestring));
        saveheader->total_size = 0xffffffffffffffff;
        saveheader->node_count = game.nodes.size();
        saveheader->Nodes_offset = current_absolute_offset;
        saveheader->LogicBlock_offset = 0xffffffffffffffff;
    }

    for (Node* node : game.nodes) {
        size_t node_offs = current_absolute_offset;
        {
            NodeData* nodedata = add<NodeData>();
            nodedata->pos = node->pos;
            nodedata->size = node->size;
            nodedata->color = node->color;
            nodedata->abs_node_offset = node->get_abs_node_offset();
            nodedata->input_count = node->inputs.size();
            nodedata->output_count = node->outputs.size();
            nodedata->inputs_offset = current_absolute_offset;
            nodedata->outputs_offset = current_absolute_offset + nodedata->input_count * sizeof(InputData);
            nodedata->total_size = sizeof(NodeData) + nodedata->input_count * sizeof(InputData) + nodedata->output_count * sizeof(OutputData);
        }

        for (Input_connector& inconn : node->inputs) {
            InputData* indata = add<InputData>();
            indata->target_id = inconn.target_id;
            std::strncpy(indata->name, inconn.name.c_str(), 55);
            indata->name[55] = '\0';
        }
        for (Output_connector& outconn : node->outputs) {
            OutputData* outdata = add<OutputData>();
            outdata->id = outconn.id;
            std::strncpy(outdata->name, outconn.name.c_str(), 55);
            outdata->name[55] = '\0';
            outdata->state = outconn.state;
        }
        {
            NodeData* nodedata = get_at_abs<NodeData>(node_offs);
            assert(nodedata->outputs_offset == current_absolute_offset);
            assert(nodedata->total_size == current_absolute_offset - node_offs);
        }

    }

    size_t logicblock_offs = current_absolute_offset;
    {
        SaveHeader* saveheader = get_at_abs<SaveHeader>(saveheader_offs);
        saveheader->total_size = 0xffffffffffffffff;
        saveheader->LogicBlock_offset = logicblock_offs;
    }

    add_raw(game.get_logicblock<uint8_t>(0), game.logicblock_size());
    {
        SaveHeader* saveheader = get_at_abs<SaveHeader>(saveheader_offs);
        saveheader->total_size = current_absolute_offset;
    }


    outputFile.write(get_at_abs<const char>(0), current_absolute_offset);
    outputFile.close();
}
