#include "LogicBlocks.h"
#include "vector_tools.h"
#include "main_game.h"


size_t LogicBlockBuilder::add_root(const std::vector<Node*> nodes) // remember that the vector may move when adding more stuff, so pointers don't live very long
{
    size_t node_offset = current_offset;
    size_t header_offset = current_offset;

    {
        // Reserve space for header
        RootNodeHeader* header = add<RootNodeHeader>(); // gets destroyed when adding

        // Fill in header
        header->type = NodeType::RootNode;
        header->child_count = nodes.size();
        header->total_size = 69420;
    }

    // Add child nodes recursively
    size_t children_offset = current_offset;
    for (const Node* child : nodes) {
        add_node(*child);
    }

    size_t converted_inputs = 0;
    size_t converted_outputs = 0;

    connect_children(children_offset, nodes.size());

    {
        NodeHeader* header = get_at<NodeHeader>(header_offset);
        header->total_size = current_offset - node_offset;
    }
    return node_offset;
}

size_t LogicBlockBuilder::add_function_root(const std::vector<Node*> nodes)
{

    size_t header_offset = current_offset;

    struct OffsetStruct {
        offset shared_outputs_offset;
        offset shared_new_outputs_offset;
    };
    std::map<std::string, OffsetStruct> bus_map;

    {
        // Reserve space for header
        FunctionNodeHeader* header = add<FunctionNodeHeader>(); // gets destroyed when adding

        // Fill in header
        header->type = NodeType::FunctionNode;
        header->total_size = 69420;
        header->input_targ_node_count = 0;
        for (Node* node : nodes) {
            if (node->isInput()) header->input_targ_node_count++;
        }
        header->output_targ_node_count = 0;
        for (Node* node : nodes) {
            if (node->isOutput()) header->output_targ_node_count++;
        }
        header->input_count = 0;
        for (Node* node : nodes) {
            if (node->isInput()) {
                header->input_count += node->outputs.size();
            }
        }
        header->output_count = 0;
        for (Node* node : nodes) {
            if (node->isOutput()) {
                header->output_count += node->inputs.size();
            }
        }
        header->child_count = nodes.size();
        header->has_changed = true;
    }


    // add input targets to nodedata
    offset intargs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isInput()) {
            assert(i >= 0);
            assert(i <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(i))); // is converted later
        }
    }
    assert(((current_offset - intargs_offset) / sizeof(offset) == get_at<FunctionNodeHeader>(header_offset)->input_targ_node_count));

    // add output targets to nodedata
    offset outtargs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            assert(i >= 0);
            assert(i <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(i))); // is converted later
        }
    }
    assert(((current_offset - outtargs_offset) / sizeof(offset) == get_at<FunctionNodeHeader>(header_offset)->output_targ_node_count));


    // add inputs
    size_t inputs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isInput()) {
            for (size_t j = 0; j < nodes[i]->outputs.size(); j++) {
                add<input>(input(UINT32_MAX)); // is converted by parent
            }
        }
    }
    assert(LogicBlockBuilder::inputs_offset(header_offset) == inputs_offset);


    // add outputs
    size_t outputs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            for (size_t j = 0; j < nodes[i]->inputs.size(); j++) {
                add<output>(false);
            }
        }
    }
    assert(LogicBlockBuilder::outputs_offset(header_offset) == outputs_offset);

    // add new_outputs
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            for (size_t j = 0; j < nodes[i]->inputs.size(); j++) {
                add<output>(false);
            }
        }
    }

    // Add child nodes recursively
    offset children_offset = current_offset;

    for (const Node* child : nodes) {
        add_node(*child);
    }

    // Convert targets into offsets
    size_t intarg_count = get_at<FunctionNodeHeader>(header_offset)->input_targ_node_count;

    size_t converted_intargs = 0;
    // Update input target offsets
    for (size_t i = 0; i < intarg_count; i++) {
        offset curr_child_offset = children_offset;
        for (size_t j = 0; j < (nodes.size()); j++) {
            if (intargs_offset + i * sizeof(offset) == j) {
                *get_at<offset>(intargs_offset + i * sizeof(offset)) = curr_child_offset;
                ++converted_intargs;
                break;
            }
            // Move to the next child
            curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
        }
    }
    assert(converted_intargs == intarg_count);

    connect_children(children_offset, nodes.size());

    {
        NodeHeader* header = get_at<NodeHeader>(header_offset);
        header->total_size = current_offset - header_offset;
    }
    return header_offset;
}

inline size_t LogicBlockBuilder::add_node(const Node& node) {
    size_t node_offset = current_offset;
    size_t header_offset = current_offset;

    struct OffsetStruct {
        offset shared_outputs_offset;
        offset shared_new_outputs_offset;
    };
    std::map<std::string, OffsetStruct> bus_map;

    // Add node data
    switch (node.get_type())
    {
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        {
            // Reserve space for header
            BinaryGateHeader* header = add<BinaryGateHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = node.get_type();
            header->input_count = node.inputs.size();
            header->total_size = 69420;
        }


        size_t inputs_offset = current_offset;

        // add inputs
        for (const Input_connector& inconn : node.inputs) {
            if (inconn.target) {
                int pos = get_position<Node>(*node.get_container(), inconn.target->host);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<input>(input(static_cast<uint16_t>(pos), inconn.target->index)); // is converted by parent
            }
            else {
                add<input>(input(UINT32_MAX)); // is converted by parent
            }
        }

        size_t outputs_offset = current_offset;

        // add outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        // add new_outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.new_state });
        }
        break;
    }

    case NodeType::GateBUFFER:
    case NodeType::GateNOT: {
        {
            // Reserve space for header
            UnaryGateHeader* header = add<UnaryGateHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = node.get_type();
            header->input_output_count = node.inputs.size();
            assert(node.inputs.size() == node.outputs.size());
            header->total_size = 69420;
        }


        size_t inputs_offset = current_offset;

        // add inputs
        for (const Input_connector& inconn : node.inputs) {
            if (inconn.target) {
                int pos = get_position<Node>(*node.get_container(), inconn.target->host);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<input>(input(static_cast<uint16_t>(pos), inconn.target->index)); // is converted by parent
            }
            else {
                add<input>(input(UINT32_MAX)); // is converted by parent
            }
        }

        size_t outputs_offset = current_offset;

        // add outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        // add new_outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.new_state });
        }
        break;
    }

    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton: {
        {
            // Reserve space for header
            InputNodeHeader* header = add<InputNodeHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = node.get_type();
            header->output_count = node.outputs.size();
            header->total_size = 69420;
        }

        size_t outputs_offset = current_offset;

        // add outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }
        break;
    }

    case NodeType::LightBulb: {
        {
            // Reserve space for header
            OutputNodeHeader* header = add<OutputNodeHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = node.get_type();
            header->input_count = node.inputs.size();
            header->total_size = 69420;
        }


        size_t inputs_offset = current_offset;

        // add inputs
        for (const Input_connector& inconn : node.inputs) {
            if (inconn.target) {
                int pos = get_position<Node>(*node.get_container(), inconn.target->host);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<input>(input(static_cast<uint16_t>(pos), inconn.target->index)); // is converted by parent
            }
            else {
                add<input>(input(UINT32_MAX)); // is converted by parent
            }
        }
        break;
    }

    case NodeType::SevenSegmentDisplay: {
        {
            // Reserve space for header
            GenericNodeHeader* header = add<GenericNodeHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = node.get_type();
            header->input_count = node.inputs.size();
            header->output_count = node.outputs.size();
            header->total_size = 69420;
        }


        size_t inputs_offset = current_offset;

        // add inputs
        for (const Input_connector& inconn : node.inputs) {
            if (inconn.target) {
                int pos = get_position<Node>(*node.get_container(), inconn.target->host);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<input>(input(static_cast<uint16_t>(pos), inconn.target->index)); // is converted by parent
            }
            else {
                add<input>(input(UINT32_MAX)); // is converted by parent
            }
        }

        size_t outputs_offset = current_offset;

        // add outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        // add new_outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.new_state });
        }
        break;
    }

    case NodeType::FunctionNode: {
        const FunctionNode& funnode = static_cast<const FunctionNode&>(node);

        {
            // Reserve space for header
            FunctionNodeHeader* header = add<FunctionNodeHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = funnode.get_type();
            header->total_size = 69420;
            header->input_targ_node_count = funnode.input_targs.size();
            header->output_targ_node_count = funnode.output_targs.size();
            header->input_count = funnode.inputs.size();
            header->output_count = funnode.outputs.size();
            header->child_count = funnode.nodes.size();
            header->has_changed = true;
        }


        // add input targets to nodedata
        offset intargs_offset = current_offset;
        for (Node* intarg : funnode.input_targs) {
            int pos = get_position<Node>(*funnode.get_children(), intarg);
            assert(pos >= 0);
            assert(pos <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(pos))); // is converted later
        }
        assert(((current_offset - intargs_offset) / sizeof(offset) == funnode.input_targs.size()));

        // add output targets to nodedata
        offset outtargs_offset = current_offset;
        for (Node* outtarg : funnode.output_targs) {
            int pos = get_position<Node>(*funnode.get_children(), outtarg);
            assert(pos >= 0);
            assert(pos <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(pos))); // is converted later
        }
        assert(((current_offset - outtargs_offset) / sizeof(offset) == funnode.output_targs.size()));


        // add inputs
        for (const Input_connector& inconn : node.inputs) {
            if (inconn.target) {
                int pos = get_position<Node>(*node.get_container(), inconn.target->host);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<input>(input(static_cast<uint16_t>(pos), inconn.target->index)); // is converted by parent
            }
            else {
                add<input>(input(UINT32_MAX)); // is converted by parent
            }
        }

        size_t outputs_offset = current_offset;

        // add outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        // add new_outputs
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.new_state });
        }

        // Add child nodes recursively
        offset children_offset = current_offset;

        for (const Node* child : *funnode.get_children()) {
            add_node(*child);
        }

        // Convert targets into offsets
        size_t intarg_count = get_at<FunctionNodeHeader>(header_offset)->input_targ_node_count;

        size_t converted_intargs = 0;
        // Update input target offsets
        for (size_t i = 0; i < intarg_count; i++) {
            offset curr_child_offset = children_offset;
            for (size_t j = 0; j < (*funnode.get_children()).size(); j++) {
                if (intargs_offset + i * sizeof(offset) == j) {
                    *get_at<offset>(intargs_offset + i * sizeof(offset)) = curr_child_offset;
                    ++converted_intargs;
                    break;
                }
                // Move to the next child
                curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
            }
        }
        assert(converted_intargs == intarg_count);

        connect_children(children_offset, funnode.get_children()->size());

        break;
    }

    case NodeType::Bus: {

        const Bus& busnode = static_cast<const Bus&>(node);

        {
            // Reserve space for header
            BusNodeHeader* header = add<BusNodeHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = busnode.get_type();
            header->total_size = 69420;
            header->input_count = busnode.inputs.size();
            
        }

        offset inputs_offset = current_offset;

        // add inputs
        for (const Input_connector& inconn : node.inputs) {
            if (inconn.target) {
                int pos = get_position<Node>(*node.get_container(), inconn.target->host);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<input>(input(static_cast<uint16_t>(pos), inconn.target->index)); // is converted by parent
            }
            else {
                add<input>(input(UINT32_MAX)); // is converted by parent
            }
        }

        if (!bus_map.count(busnode.label) > 0) {
            offset shared_outputs_offset = current_offset;
            for (size_t i = 0; i < busnode.bus_values_size(); i++) {
                add<output>(false); //TODO! Make these copy the state over
            }
            offset shared_new_outputs_offset = current_offset;
            for (size_t i = 0; i < busnode.bus_values_size(); i++) {
                add<output>(false);
            }
            bus_map.insert({ busnode.label, { shared_outputs_offset, shared_new_outputs_offset } });
        }
        {
            BusNodeHeader* header = get_at<BusNodeHeader>(header_offset); // gets destroyed when adding
            header->shared_outputs_offset = bus_map[busnode.label].shared_outputs_offset;
            header->shared_new_outputs_offset = bus_map[busnode.label].shared_new_outputs_offset;
        }
        break;
    }
    case NodeType::RootNode: {
        assert(false && "roots have their own function");
    }
    default:
        assert(false && "unknown node_type");
        break;
    }

    {
        NodeHeader* header = get_at<NodeHeader>(header_offset);
        header->total_size = current_offset - node_offset;
    }
    return node_offset;
    //TODO! store offsets in targeted node
}

void LogicBlockBuilder::connect_children(size_t children_offset, size_t child_count)
{
    offset curr_child_offset = children_offset;
    for (size_t i = 0; i < child_count; i++) {

        // Connect the inputs of the child nodes

        if (!input_count(curr_child_offset) > 0) continue; // ensure there are inputs to connect
        input* curr_child_inputs = get_at<input>(inputs_offset(curr_child_offset));

        std::vector<bool> converted_inputs(input_count(curr_child_offset), false);

        // Convert all unconnected inputs(indicated by being UINT32_MAX) to 0
        for (size_t input_idx = 0; input_idx < input_count(curr_child_offset); input_idx++) {
            if (curr_child_inputs[input_idx].target_offset == UINT32_MAX) {
                curr_child_inputs[input_idx].target_offset = 0;
                converted_inputs[input_idx] = true;
            }
        }
        offset curr_inner_child_offset = children_offset;

        for (size_t j = 0; j < child_count; j++) {

            for (size_t input_idx = 0; input_idx < input_count(curr_child_offset); input_idx++) {
                if (!converted_inputs[input_idx]) {
                    if (curr_child_inputs[input_idx].location.node_pos == j) {
                        // This input should be connected to an output of the current inner child
                        offset output_offset = outputs_offset(curr_inner_child_offset);

                        curr_child_inputs[input_idx].target_offset = output_offset + curr_child_inputs[input_idx].location.connector_index * sizeof(output);
                        converted_inputs[input_idx] = true;
                    }
                }
            }

            curr_inner_child_offset += get_at<NodeHeader>(curr_inner_child_offset)->total_size;
        }

        // Check if all inputs were converted
        assert(std::all_of(converted_inputs.begin(), converted_inputs.end(), [](bool v) { return v; }) &&
            "Not all inputs were converted");

        // Move to the next child offset
        curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
    }
}
