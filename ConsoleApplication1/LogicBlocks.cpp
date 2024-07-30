#include "LogicBlocks.h"
#include "vector_tools.h"
#include "main_game.h"

constexpr uint32_t NODE_ALIGNMENT = 4;

size_t LogicBlockBuilder::add_root(std::vector<Node*> nodes)
{
    assert(current_offset == 0);
    offset node_offset = current_offset;

    {
        // Reserve space for header
        RootNodeHeader* header = add<RootNodeHeader>(); // gets destroyed when adding

        // Fill in header
        header->type = NodeType::RootNode;
        header->total_size = 69420;
        header->child_count = nodes.size();
    }

    add_padding(NODE_ALIGNMENT);

    // Add child nodes recursively
    offset children_offset = current_offset;

    bus_map_t new_bus_map;
    for (Node* child : nodes) {
        assert(current_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        add_node(*child, new_bus_map);
    }
    assert(current_offset % NODE_ALIGNMENT == 0 && "misaligned node");


    {
        RootNodeHeader* header = get_at<RootNodeHeader>(node_offset);
        connect_children(children_offset, header->child_count);
        header->total_size = current_offset - node_offset;
        header->children_offset = children_offset;
        assert(current_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        assert(header->total_size % NODE_ALIGNMENT == 0 && "misaligned size");
    }
    return node_offset;
}

size_t LogicBlockBuilder::add_function_root(std::vector<Node*> nodes)
{
    assert(current_offset == 0);
    offset node_offset = current_offset;

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

    add_padding(sizeof(offset));

    // add intargs to nodedata
    offset intargs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isInput()) {
            assert(i >= 0);
            assert(i <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(i))); // is converted later
        }
    }
    assert(((current_offset - intargs_offset) / sizeof(offset) == get_at<FunctionNodeHeader>(node_offset)->input_targ_node_count));

    // add outtargs to nodedata
    offset outtargs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            assert(i >= 0);
            assert(i <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(i))); // is converted later
        }
    }
    assert(((current_offset - outtargs_offset) / sizeof(offset) == get_at<FunctionNodeHeader>(node_offset)->output_targ_node_count));

    add_padding(sizeof(input));

    // add inputs
    size_t inputs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isInput()) {
            for (size_t j = 0; j < nodes[i]->outputs.size(); j++) {
                add<input>(input(0)); // is not converted by parent so its 0
            }
        }
    }

    add_padding(sizeof(output));

    // add outputs
    size_t outputs_offset = current_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            for (size_t j = 0; j < nodes[i]->inputs.size(); j++) {
                add<output>(false);
            }
        }
    }

    // add new_outputs
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            for (size_t j = 0; j < nodes[i]->inputs.size(); j++) {
                add<output>(false);
            }
        }
    }

    add_padding(NODE_ALIGNMENT);

    // Add child nodes recursively
    offset children_offset = current_offset;

    bus_map_t new_bus_map;
    for (Node* child : nodes) {
        assert(current_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        add_node(*child, new_bus_map);
    }
    assert(current_offset% NODE_ALIGNMENT == 0 && "misaligned node");

    // Convert targets into offsets
    {
        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset);
        size_t intarg_count = header->input_targ_node_count;

        size_t converted_intargs = 0;
        // Update input target offsets
        for (size_t i = 0; i < intarg_count; i++) {
            offset curr_child_offset = children_offset;
            offset intarg_idx = *get_at<offset>(intargs_offset + i * sizeof(offset));
            for (size_t j = 0; j < header->child_count; j++) {
                if (intarg_idx == j) {
                    *get_at<offset>(intargs_offset + i * sizeof(offset)) = curr_child_offset;
                    ++converted_intargs;
                    break;
                }
                // Move to the next child
                curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
            }
        }
        assert(converted_intargs == intarg_count);

        size_t outtarg_count = header->output_targ_node_count;

        size_t converted_outtargs = 0;
        // Update input target offsets
        for (size_t i = 0; i < outtarg_count; i++) {
            offset curr_child_offset = children_offset;
            offset outtarg_idx = *get_at<offset>(outtargs_offset + i * sizeof(offset));
            for (size_t j = 0; j < header->child_count; j++) {
                if (outtarg_idx == j) {
                    *get_at<offset>(outtargs_offset + i * sizeof(offset)) = curr_child_offset;
                    ++converted_outtargs;
                    break;
                }
                // Move to the next child
                curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
            }
        }
        assert(converted_outtargs == outtarg_count);

        connect_children(children_offset, header->child_count);
    }

    {
        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset);
        header->total_size = current_offset - node_offset;
        header->intargs_offset = intargs_offset;
        header->outtargs_offset = outtargs_offset;
        header->inputs_offset = inputs_offset;
        header->outputs_offset = outputs_offset;
        header->children_offset = children_offset;
        assert(current_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        assert(header->total_size % NODE_ALIGNMENT == 0 && "misaligned size");
    }
    return node_offset;
}

offset LogicBlockBuilder::add_node(Node& node, bus_map_t& bus_map) {
    offset node_offset = current_offset;

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
            header->output = node.outputs[0].state;
            header->new_output = node.outputs[0].new_state;
            header->total_size = 69420;
        }

        add_padding(sizeof(input));

        // add inputs
        offset inputs_offset = current_offset;
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

        {
            BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
            header->inputs_offset = inputs_offset;
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

        add_padding(sizeof(input));

        // add inputs
        offset inputs_offset = current_offset;
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

        add_padding(sizeof(output));

        // add outputs
        offset outputs_offset = current_offset;
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        // add new_outputs
        offset new_outputs_offset = current_offset;
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.new_state });
        }
        {
            UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset);
            header->inputs_offset = inputs_offset;
            header->outputs_offset = outputs_offset;
            header->new_outputs_offset = new_outputs_offset;
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

        add_padding(sizeof(output));

        // add outputs
        offset outputs_offset = current_offset;
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }
        {
            InputNodeHeader* header = get_at<InputNodeHeader>(node_offset);
            header->outputs_offset = outputs_offset;
        }
        break;
    }

    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        {
            // Reserve space for header
            OutputNodeHeader* header = add<OutputNodeHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = node.get_type();
            header->input_count = node.inputs.size();
            header->total_size = 69420;
        }

        add_padding(sizeof(input));

        // add inputs
        offset inputs_offset = current_offset;
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
        {
            OutputNodeHeader* header = get_at<OutputNodeHeader>(node_offset);
            header->inputs_offset = inputs_offset;
        }
        break;
    }

    case NodeType::FunctionNode: {
        FunctionNode& funnode = static_cast<FunctionNode&>(node);

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

        add_padding(alignof(offset));

        // add input targets to nodedata
        offset intargs_offset = current_offset;
        for (Node* intarg : funnode.input_targs) {
            int pos = get_position<Node>(*funnode.get_children(), intarg);
            assert(pos >= 0);
            assert(pos <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(pos))); // is converted later
        }
        assert(((current_offset - intargs_offset) / sizeof(offset) == funnode.input_targs.size()));

        add_padding(alignof(offset));

        // add output targets to nodedata
        offset outtargs_offset = current_offset;
        for (Node* outtarg : funnode.output_targs) {
            int pos = get_position<Node>(*funnode.get_children(), outtarg);
            assert(pos >= 0);
            assert(pos <= UINT16_MAX);
            add<offset>(offset(static_cast<offset>(pos))); // is converted later
        }
        assert(((current_offset - outtargs_offset) / sizeof(offset) == funnode.output_targs.size()));

        add_padding(alignof(input));

        // add inputs
        offset inputs_offset = current_offset;
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

        add_padding(alignof(output));

        // add outputs
        offset outputs_offset = current_offset;
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        add_padding(NODE_ALIGNMENT);

        // Add child nodes recursively
        offset children_offset = current_offset;

        bus_map_t new_bus_map;
        for (Node* child : *funnode.get_children()) {
            assert(current_offset % NODE_ALIGNMENT == 0 && "misaligned node");
            add_node(*child, new_bus_map);
            assert(current_offset% NODE_ALIGNMENT == 0 && "misaligned node");
        }

        {
            FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset);
            header->intargs_offset = intargs_offset;
            header->outtargs_offset = outtargs_offset;
            header->inputs_offset = inputs_offset;
            header->outputs_offset = outputs_offset;
            header->children_offset = children_offset;
        }

        // Convert targets into offsets
        {
            FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset);
            size_t intarg_count = header->input_targ_node_count;

            size_t converted_intargs = 0;
            // Update input target offsets
            for (size_t i = 0; i < intarg_count; i++) {
                offset curr_child_offset = children_offset;
                offset intarg_idx = *get_at<offset>(intargs_offset + i * sizeof(offset));
                for (size_t j = 0; j < header->child_count; j++) {
                    if (intarg_idx == j) {
                        *get_at<offset>(intargs_offset + i * sizeof(offset)) = curr_child_offset;
                        ++converted_intargs;
                        break;
                    }
                    // Move to the next child
                    assert(curr_child_offset % NODE_ALIGNMENT == 0);
                    curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
                }
            }
            assert(converted_intargs == intarg_count);

            size_t outtarg_count = header->output_targ_node_count;

            size_t converted_outtargs = 0;
            // Update input target offsets
            for (size_t i = 0; i < outtarg_count; i++) {
                offset curr_child_offset = children_offset;
                offset outtarg_idx = *get_at<offset>(outtargs_offset + i * sizeof(offset));
                for (size_t j = 0; j < header->child_count; j++) {
                    if (outtarg_idx == j) {
                        *get_at<offset>(outtargs_offset + i * sizeof(offset)) = curr_child_offset;
                        ++converted_outtargs;
                        break;
                    }
                    // Move to the next child
                    curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
                }
            }
            assert(converted_outtargs == outtarg_count);

            connect_children(children_offset, header->child_count);
        }
        break;
    }

    case NodeType::BusNode: {

        const Bus& busnode = static_cast<const Bus&>(node);

        {
            // Reserve space for header
            BusNodeHeader* header = add<BusNodeHeader>(); // gets destroyed when adding

            // Fill in header
            header->type = busnode.get_type();
            header->total_size = 69420;
            header->input_output_count = busnode.inputs.size();
            header->shared_output_count = busnode.bus_values_size();
        }

        add_padding(alignof(input));

        // add inputs
        offset inputs_offset = current_offset;
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

        add_padding(alignof(output));

        // add shared outputs if first instance of the connected bus, must be the first for inference to work
        bool is_first_node_in_bus = !bus_map.count(busnode.label) > 0;
        if (is_first_node_in_bus) {
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
            BusNodeHeader* header = get_at<BusNodeHeader>(node_offset); // gets destroyed when adding
            header->is_first_node_in_bus = is_first_node_in_bus;
            header->inputs_offset = inputs_offset;
            header->shared_outputs_offset = bus_map[busnode.label].shared_outputs_offset;
            header->shared_new_outputs_offset = bus_map[busnode.label].shared_new_outputs_offset;
        }
        break;
    }
    default:
        assert(false && "unknown node_type");
        break;
    }

    add_padding(NODE_ALIGNMENT);
    {
        NodeHeader* header = get_at<NodeHeader>(node_offset);
        header->total_size = current_offset - node_offset;
        assert(current_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        assert(header->total_size % NODE_ALIGNMENT == 0 && "misaligned size");
        
        if (header->type == NodeType::FunctionNode) {
            FunctionNode& funnode = static_cast<FunctionNode&>(node);
            funnode.allocate_function_data(get_at<uint8_t>(node_offset));
        }
    }
    node.set_node_offset(node_offset);
    return node_offset;
    //TODO! store offsets in targeted node
}

void LogicBlockBuilder::connect_children(size_t children_offset, size_t child_count)
{
    offset curr_child_offset = children_offset;
    for (size_t i = 0; i < child_count; i++) {

        // Connect the inputs of the child nodes
        

        // ensure there are inputs to connect
        if (!input_count(curr_child_offset) > 0) { 
            // Move to the next child offset
            curr_child_offset += get_at<NodeHeader>(curr_child_offset)->total_size;
            continue;
        }


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
                        assert(curr_child_inputs[input_idx].target_offset < buffer.size());
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

size_t LogicBlock::parse(offset node_offset, std::string indent, size_t node_number) {
    assert(node_offset < size);
    assert(node_offset % 4 == 0);
    NodeHeader* header = get_at<NodeHeader>(node_offset);
    std::cout << std::boolalpha << std::hex;
    assert(header->total_size % 4 == 0);
    std::cout 
        << indent << "header:\n"
        << indent << "  type: " << toString(header->type) << "\n"
        << indent << "  offset: " << node_offset << "\n"
        //<< indent << "  number: " << node_number << "\n"
        << indent << "  total_size: " << header->total_size << "\n";
    switch (header->type) {
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        std::cout 
            << indent << "  output\n"
            //<< indent << "   value: " << header->output << "\n"
            << indent << "   offset: " << node_offset + (&header->output - get_at<bool>(node_offset)) * sizeof(bool) << "\n";
        for (size_t i = 0; i < header->input_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(header->inputs_offset + i * sizeof(input))->target_offset << "\n";
        }
        break;
    }
    case NodeType::GateBUFFER:
    case NodeType::GateNOT: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset);
        for (size_t i = 0; i < header->input_output_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(header->inputs_offset + i * sizeof(input))->target_offset << "\n";
        }
        for (size_t i = 0; i < header->input_output_count; i++) {
            std::cout
                << indent << "  output\n"
                //<< indent << "   value: " << *get_at<output>(header->outputs_offset + i * sizeof(output)) << "\n"
                << indent << "   offset: " << header->outputs_offset + i * sizeof(output) << "\n";
        }
        break;
    }
    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton: {
        InputNodeHeader* header = get_at<InputNodeHeader>(node_offset);
        for (size_t i = 0; i < header->output_count; i++) {
            std::cout
                << indent << "  output\n"
                //<< indent << "   value: " << *get_at<output>(header->outputs_offset + i * sizeof(output)) << "\n"
                << indent << "   offset: " << header->outputs_offset + i * sizeof(output) << "\n";
        }
        break;
    }
    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        OutputNodeHeader* header = get_at<OutputNodeHeader>(node_offset);
        for (size_t i = 0; i < header->input_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(header->inputs_offset + i * sizeof(input))->target_offset << "\n";
        }
        break;
    }
    case NodeType::FunctionNode: {
        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset);
        for (size_t i = 0; i < header->input_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(header->inputs_offset + i * sizeof(input))->target_offset << "\n";
        }
        for (size_t i = 0; i < header->output_count; i++) {
            std::cout
                << indent << "  output\n"
                //<< indent << "   value: " << *get_at<output>(header->outputs_offset + i * sizeof(output)) << "\n"
                << indent << "   offset: " << header->outputs_offset + i * sizeof(output) << "\n";
        }
        std::cout << indent << "children:\n";
        indent.append("  ");
        offset new_offset = header->children_offset;
        size_t new_node_number = node_number + 1;
        for (size_t i = 0; i < header->child_count; i++) {
            new_offset += parse(new_offset, indent, new_node_number);
            ++new_node_number;
        }
        break;
    }
    case NodeType::BusNode: {
        break;
    }
    case NodeType::RootNode: {
        RootNodeHeader* header = get_at<RootNodeHeader>(node_offset);
        std::cout << indent << "children:\n";
        indent.append("  ");
        offset new_offset = header->children_offset;
        size_t new_node_number = node_number + 1;
        for (size_t i = 0; i < header->child_count; i++) {
            new_offset += parse(new_offset, indent, new_node_number);
            ++new_node_number;
        }
        break;
    }
    }

    return header->total_size;
}

bool LogicBlock::pretick(bool update_all, offset node_offset){
    NodeHeader* header = get_at<NodeHeader>(node_offset);
    switch (header->type) {
    case NodeType::GateAND: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        header->new_output = true;
        for (size_t i = 0; header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset);
        }
        return header->new_output != header->output;
    }
    case NodeType::GateOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        header->new_output = false;
        for (size_t i = 0; !header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset);
        }
        return header->new_output != header->output;
    }
    case NodeType::GateNAND: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        header->new_output = true;
        for (size_t i = 0; header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset);
        }
        header->new_output = !header->new_output;
        return header->new_output != header->output;
    }
    case NodeType::GateNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        header->new_output = false;
        for (size_t i = 0; !header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset);
        }
        header->new_output = !header->new_output;
        return header->new_output != header->output;
    }
    case NodeType::GateXOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        header->new_output = false;
        for (size_t i = 0; i < header->input_count; i++) {
            if (get_input_val(i, header->inputs_offset)) {
                header->new_output = !header->new_output;
            }
        }
        return header->new_output != header->output;
    }
    case NodeType::GateXNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        header->new_output = true;
        for (size_t i = 0; i < header->input_count; i++) {
            if (get_input_val(i, header->inputs_offset)) {
                header->new_output = !header->new_output;
            }
        }
        return header->new_output != header->output;
    }
    case NodeType::GateBUFFER: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset);
        bool has_changed = false;
        for (size_t i = 0; i < header->input_output_count; i++) {
            output* new_output = get_at<output>(header->new_outputs_offset + i * sizeof(output));
            *new_output = get_input_val(i, header->inputs_offset);

            output* curr_output = get_at<output>(header->outputs_offset + i * sizeof(output));
            if (*new_output != *curr_output) has_changed = true;
        }
        return has_changed;
    }
    case NodeType::GateNOT: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset);
        bool has_changed = false;
        for (size_t i = 0; i < header->input_output_count; i++) {
            output* new_output = get_at<output>(header->new_outputs_offset + i * sizeof(output));
            *new_output = !get_input_val(i, header->inputs_offset);

            output* curr_output = get_at<output>(header->outputs_offset + i * sizeof(output));
            if (*new_output != *curr_output) has_changed = true;
        }
        return has_changed;
    }
    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton:
    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        return false;
    }
    case NodeType::FunctionNode: {
        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset);

        // load input values into the outputs of the input targs
        bool new_input = false;
        size_t input_idx = 0;
        for (size_t i = 0; i < header->input_targ_node_count; i++) {
            offset input_targ_offset = *get_at<offset>(header->intargs_offset + i * sizeof(offset));
            InputNodeHeader* input_targ_header = get_at<InputNodeHeader>(input_targ_offset);

            for (size_t j = 0; j < input_targ_header->output_count; j++) {
                assert(input_idx < header->input_count);
                output* outconn = get_at<output>(input_targ_header->outputs_offset + j * sizeof(output));
                bool input_val = get_input_val(input_idx, header->inputs_offset);
                if(*outconn != input_val) new_input = true;
                *outconn = input_val;
                ++input_idx;
            }
        }

        //if needed,pretick 
        //TODO fix this
        if (header->has_changed || new_input || update_all) {
            header->has_changed = false;
            offset curr_child_offset = header->children_offset;
            for (size_t i = 0; i < header->child_count; i++) {
                NodeHeader* child_header = get_at<NodeHeader>(curr_child_offset);
                if(pretick(update_all, curr_child_offset)) header->has_changed = true;
                curr_child_offset += child_header->total_size;
            }
            if (new_input) header->has_changed = true;
        }
        return header->has_changed;
    }
    // Bus nodes are not as efficient as i want but similar to buffers
    case NodeType::BusNode: {
        BusNodeHeader* header = get_at<BusNodeHeader>(node_offset);
        bool has_changed = false;

        //is it the first node in the bus
        if (header->total_size > (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))){
            uint16_t shared_output_count = (header->total_size - (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))) / 2;
            for (size_t i = 0; i < shared_output_count; i++) {
                output* new_output = get_at<output>(header->shared_new_outputs_offset + i * sizeof(output));
                *new_output = false;
            }
        }

        for (size_t i = 0; i < header->input_output_count; i++) {
            output* new_output = get_at<output>(header->shared_new_outputs_offset + i * sizeof(output));
            if (get_input_val(i, header->inputs_offset)) {
                if (!*new_output) {
                    *new_output = true;
                }
            }

            output* curr_output = get_at<output>(header->shared_outputs_offset + i * sizeof(output));
            if (*new_output != *curr_output) has_changed = true;
        }
        return has_changed;
        
    }
    
    case NodeType::RootNode: {
        RootNodeHeader* header = get_at<RootNodeHeader>(node_offset);

        offset curr_child_offset = header->children_offset;
        for (size_t i = 0; i < header->child_count; i++) {
            NodeHeader* child_header = get_at<NodeHeader>(curr_child_offset);
            pretick(update_all, curr_child_offset);
            curr_child_offset += child_header->total_size;
        }

        return true;
    }
    default: assert(false);
    }
}

void LogicBlock::tick(bool update_all, offset node_offset)
{
    NodeHeader* header = get_at<NodeHeader>(node_offset);
    switch (header->type) {
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset);
        header->output = header->new_output;
        return;
    }
    case NodeType::GateBUFFER:
    case NodeType::GateNOT: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset);
        for (size_t i = 0; i < header->input_output_count; i++) {
            *get_at<output>(header->outputs_offset + i * sizeof(output)) = *get_at<output>(header->new_outputs_offset + i * sizeof(output));
        }
        return;
    }
    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton:
    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        return;
    }
    case NodeType::FunctionNode: {
        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset);
        if (header->has_changed || update_all) {
            offset curr_child_offset = header->children_offset;
            for (size_t i = 0; i < header->child_count; i++) {
                NodeHeader* child_header = get_at<NodeHeader>(curr_child_offset);
                tick(update_all, curr_child_offset);
                curr_child_offset += child_header->total_size;
            }


            uint32_t output_idx = 0;
            for (size_t i = 0; i < header->output_targ_node_count; i++) {
                offset output_targ_offset = *get_at<offset>(header->outtargs_offset + i * sizeof(offset));
                OutputNodeHeader* output_targ_header = get_at<OutputNodeHeader>(output_targ_offset);

                for (size_t j = 0; j < output_targ_header->input_count; j++) {
                    assert(output_idx < header->output_count);
                    output* function_output = get_at<output>(header->outputs_offset + output_idx * sizeof(output));
                    bool output_targ_val = get_input_val(j, output_targ_header->inputs_offset);
                    *function_output = output_targ_val;
                    ++output_idx;
                }
            }

        }
        return;
    }
    case NodeType::BusNode: {
        BusNodeHeader* header = get_at<BusNodeHeader>(node_offset);

        //is it the first node in the bus
        if (header->total_size > (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))) {
            uint16_t shared_output_count = (header->total_size - (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))) / sizeof(output) / 2;
            for (size_t i = 0; i < shared_output_count; i++) {
                *get_at<output>(header->shared_outputs_offset + i * sizeof(output)) = *get_at<output>(header->shared_new_outputs_offset + i * sizeof(output));
            }
        }
        return;
    }
    case NodeType::RootNode: {
        RootNodeHeader* header = get_at<RootNodeHeader>(node_offset);

        offset curr_child_offset = header->children_offset;
        for (size_t i = 0; i < header->child_count; i++) {
            NodeHeader* child_header = get_at<NodeHeader>(curr_child_offset);
            tick(update_all, curr_child_offset);
            curr_child_offset += child_header->total_size;
        }
        return;
    }
    }
}


size_t LogicblockTools::input_count(offset header_offset, uint8_t* buffer) {
    NodeHeader* header = get_at<NodeHeader>(header_offset, buffer);
    switch (header->type)
    {
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR:
        return get_at<BinaryGateHeader>(header_offset, buffer)->input_count;
        break;

    case NodeType::GateBUFFER:
    case NodeType::GateNOT:
        return get_at<UnaryGateHeader>(header_offset, buffer)->input_output_count;
        break;

    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton: {
        return 0;
    }
    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        return get_at<OutputNodeHeader>(header_offset, buffer)->input_count;
    }

    case NodeType::FunctionNode: {
        return get_at<FunctionNodeHeader>(header_offset, buffer)->input_count;
        break;
    }

    case NodeType::BusNode:
        return get_at<BusNodeHeader>(header_offset, buffer)->input_output_count;
        break;
    default:
        assert(false && "unreachable");
    }
}

size_t LogicblockTools::output_count(offset header_offset, uint8_t* buffer) {
    NodeHeader* header = get_at<NodeHeader>(header_offset, buffer);
    switch (header->type) {
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR:
        return 1;

    case NodeType::GateBUFFER:
    case NodeType::GateNOT:
        return get_at<UnaryGateHeader>(header_offset, buffer)->input_output_count;

    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton:
        return get_at<InputNodeHeader>(header_offset, buffer)->output_count;

    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay:
        return 0;

    case NodeType::FunctionNode:
        return get_at<FunctionNodeHeader>(header_offset, buffer)->output_count;

    case NodeType::BusNode:
        return get_at<BusNodeHeader>(header_offset, buffer)->input_output_count;

    default:
        assert(false && "Invalid NodeType");
        return 0;
    }
}

offset LogicblockTools::outputs_offset(offset header_offset, uint8_t* buffer) {
    NodeHeader* header = get_at<NodeHeader>(header_offset, buffer);
    switch (header->type) {
        // Binary Gates
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        BinaryGateHeader* gate = get_at<BinaryGateHeader>(header_offset, buffer);
        offset byteDistance = (char*)&gate->output - (char*)get_at<size_t>(0, buffer);
        return byteDistance;
    }

                           // Unary Gates
    case NodeType::GateBUFFER:
    case NodeType::GateNOT: {
        return get_at<UnaryGateHeader>(header_offset, buffer)->outputs_offset;
    }

                          // Input Nodes (only have outputs)
    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton: {
        return get_at<InputNodeHeader>(header_offset, buffer)->outputs_offset;
    }

                                     // Output Nodes (only have inputs, no outputs)
    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        return 0;
    }

    case NodeType::FunctionNode: {
        return get_at<FunctionNodeHeader>(header_offset, buffer)->outputs_offset;
    }

    case NodeType::BusNode: {
        return get_at<BusNodeHeader>(header_offset, buffer)->shared_outputs_offset;
    }

    default:
        assert(false && "Invalid NodeType");
        return 0; // Return 0 for invalid types, though this line should never be reached due to the assert
    }
}
