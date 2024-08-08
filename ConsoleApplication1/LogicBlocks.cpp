#include "LogicBlocks.h"
#include "vector_tools.h"
#include "main_game.h"

constexpr uint32_t NODE_ALIGNMENT = 4;

size_t LogicBlockBuilder::add_root(std::vector<Node*> nodes)
{
    assert(current_absolute_offset == 0);
    size_t abs_node_offset = current_absolute_offset;

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
    size_t abs_children_offset = current_absolute_offset;
    offset children_offset = abs_children_offset - abs_node_offset;

    bus_map_t new_bus_map;
    for (Node* child : nodes) {
        assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        add_node(*child, new_bus_map, abs_children_offset);
    }
    assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");


    {
        RootNodeHeader* header = get_at_abs<RootNodeHeader>(abs_node_offset);
        if (header->child_count) {
            uint8_t* children_container = get_at_abs<uint8_t>(abs_children_offset);
            connect_children(children_container, header->child_count);
        }
        header->total_size = current_absolute_offset - abs_node_offset;
        header->children_offset = children_offset;
        assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        assert(header->total_size % NODE_ALIGNMENT == 0 && "misaligned size");
    }
    return abs_node_offset;
}

size_t LogicBlockBuilder::add_function_root(std::vector<Node*> nodes)
{
    assert(current_absolute_offset == 0);
    size_t abs_node_offset = current_absolute_offset;

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
    size_t abs_intargs_offset = current_absolute_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isInput()) {
            add<offset>(offset(static_cast<offset>(i))); // is converted later
        }
    }
    assert(((current_absolute_offset - abs_intargs_offset) / sizeof(offset) == get_at_abs<FunctionNodeHeader>(abs_node_offset)->input_targ_node_count));

    // add outtargs to nodedata
    size_t abs_outtargs_offset = current_absolute_offset;
    for (uint32_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            add<offset>(offset(static_cast<offset>(i))); // is converted later
        }
    }
    assert(((current_absolute_offset - abs_outtargs_offset) / sizeof(offset) == get_at_abs<FunctionNodeHeader>(abs_node_offset)->output_targ_node_count));

    add_padding(sizeof(input));

    // add inputs
    size_t abs_inputs_offset = current_absolute_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isInput()) {
            for (size_t j = 0; j < nodes[i]->outputs.size(); j++) {
                add<input>(input(0)); // is not converted by parent so its 0
            }
        }
    }

    add_padding(sizeof(output));

    // add outputs
    size_t abs_outputs_offset = current_absolute_offset;
    for (size_t i = 0; i < nodes.size(); i++) {
        if (nodes[i]->isOutput()) {
            for (size_t j = 0; j < nodes[i]->inputs.size(); j++) {
                add<output>(false);
            }
        }
    }

    add_padding(NODE_ALIGNMENT);

    // Add child nodes recursively
    size_t abs_children_offset = current_absolute_offset;

    bus_map_t new_bus_map;
    for (Node* child : nodes) {
        assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        add_node(*child, new_bus_map, abs_children_offset);
    }
    assert(current_absolute_offset% NODE_ALIGNMENT == 0 && "misaligned node");

    // Convert targets into offsets
    {
        FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);
        size_t intarg_count = header->input_targ_node_count;

        size_t converted_intargs = 0;
        // Update input target offsets
        for (size_t i = 0; i < intarg_count; i++) {
            size_t abs_curr_child_offset = abs_children_offset;
            offset intarg_idx = *get_at_abs<offset>(abs_intargs_offset + i * sizeof(offset));
            for (size_t j = 0; j < header->child_count; j++) {
                if (intarg_idx == j) {
                    offset* curr_intarg = get_at_abs<offset>(abs_intargs_offset + i * sizeof(offset));
                    *curr_intarg = abs_curr_child_offset - abs_node_offset;
                    ++converted_intargs;
                    break;
                }
                // Move to the next child
                abs_curr_child_offset += get_at_abs<NodeHeader>(abs_curr_child_offset)->total_size;
            }
        }
        assert(converted_intargs == intarg_count);

        size_t outtarg_count = header->output_targ_node_count;

        size_t converted_outtargs = 0;
        // Update input target offsets
        for (size_t i = 0; i < outtarg_count; i++) {
            size_t abs_curr_child_offset = abs_children_offset;
            offset outtarg_idx = *get_at_abs<offset>(abs_outtargs_offset + i * sizeof(offset));
            for (size_t j = 0; j < header->child_count; j++) {
                if (outtarg_idx == j) {
                    offset* curr_outtarg = get_at_abs<offset>(abs_outtargs_offset + i * sizeof(offset));
                    *curr_outtarg = abs_curr_child_offset - abs_node_offset;
                    ++converted_outtargs;
                    break;
                }
                // Move to the next child
                abs_curr_child_offset += get_at_abs<NodeHeader>(abs_curr_child_offset)->total_size;
            }
        }
        assert(converted_outtargs == outtarg_count);

        uint8_t* children_container = get_at_abs<uint8_t>(abs_children_offset);

        connect_children(children_container, header->child_count);
    }

    {
        offset intargs_offset  = abs_intargs_offset  - abs_node_offset;
        offset outtargs_offset = abs_outtargs_offset - abs_node_offset;
        offset inputs_offset   = abs_inputs_offset   - abs_node_offset;
        offset outputs_offset  = abs_outputs_offset  - abs_node_offset;
        offset children_offset = abs_children_offset - abs_node_offset;

        FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);
        header->total_size = current_absolute_offset - abs_node_offset;
        header->intargs_offset = intargs_offset;
        header->outtargs_offset = outtargs_offset;
        header->inputs_offset = inputs_offset;
        header->outputs_offset = outputs_offset;
        header->children_offset = children_offset;
        assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        assert(header->total_size % NODE_ALIGNMENT == 0 && "misaligned size");
    }
    return abs_node_offset;
}

void LogicBlockBuilder::add_node(Node& node, bus_map_t& bus_map, size_t abs_container_offset) {
    size_t abs_node_offset = current_absolute_offset;

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
        size_t abs_inputs_offset = current_absolute_offset;

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
            offset inputs_offset = abs_inputs_offset - abs_node_offset;
            BinaryGateHeader* header = get_at_abs<BinaryGateHeader>(abs_node_offset);
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
        size_t abs_inputs_offset = current_absolute_offset;
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
        size_t abs_outputs_offset = current_absolute_offset;
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }

        // add new_outputs
        size_t abs_new_outputs_offset = current_absolute_offset;
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.new_state });
        }

        {
            offset inputs_offset = abs_inputs_offset - abs_node_offset;
            offset outputs_offset = abs_outputs_offset - abs_node_offset;
            offset new_outputs_offset = abs_new_outputs_offset - abs_node_offset;

            UnaryGateHeader* header = get_at_abs<UnaryGateHeader>(abs_node_offset);
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
        size_t abs_outputs_offset = current_absolute_offset;
        for (const Output_connector& outconn : node.outputs) {
            add<output>({ outconn.state });
        }
        {
            offset outputs_offset = abs_outputs_offset - abs_node_offset;
            InputNodeHeader* header = get_at_abs<InputNodeHeader>(abs_node_offset);
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
        size_t abs_inputs_offset = current_absolute_offset;
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
            offset inputs_offset = abs_inputs_offset - abs_node_offset;
            OutputNodeHeader* header = get_at_abs<OutputNodeHeader>(abs_node_offset);
            header->inputs_offset = inputs_offset;
        }
        break;
    }

    case NodeType::FunctionNode: {

        FunctionNode& funnode = static_cast<FunctionNode&>(node);
        if (funnode.has_function_data()) {
            //insert data
            {
                LogicBlock* function_data = funnode.get_function_data();
                add_raw(function_data->get_data(0), function_data->get_size());
            }

            {//check header
                FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);
                assert(header->type == funnode.get_type());
                assert(header->input_count == funnode.inputs.size());
                assert(header->output_count == funnode.outputs.size());
                header->has_changed = true;
                assert(header->total_size % NODE_ALIGNMENT == 0);
                assert(current_absolute_offset % NODE_ALIGNMENT == 0);
            }
            {
                FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);

                // edit inputs
                size_t abs_inputs_offset = abs_node_offset + header->inputs_offset;

                for (size_t i = 0; i < funnode.inputs.size(); i++) {
                    Input_connector& inconn = funnode.inputs[i];
                    input* curr_input = get_at_abs<input>(abs_inputs_offset + i * sizeof(input));
                    if (inconn.target) {
                        int pos = get_position<Node>(*node.get_container(), inconn.target->host);
                        assert(pos >= 0);
                        assert(pos <= UINT16_MAX);
                        *curr_input = input(static_cast<uint16_t>(pos), inconn.target->index); // is converted by parent
                    }
                    else {
                        *curr_input = input(UINT32_MAX); // is converted by parent
                    }
                }
            }
            {
                FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);

                // edit outputs
                size_t abs_outputs_offset = abs_node_offset + header->outputs_offset;

                for (size_t i = 0; i < funnode.outputs.size(); i++) {
                    Output_connector& outconn = funnode.outputs[i];
                    output* curr_output = get_at_abs<output>(abs_outputs_offset + i * sizeof(output));
                    *curr_output = outconn.state;
                }
            }
        }
        else {
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
            size_t abs_intargs_offset = current_absolute_offset;
            for (Node* intarg : funnode.input_targs) {
                int pos = get_position<Node>(*funnode.get_children(), intarg);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<offset>(offset(static_cast<offset>(pos))); // is converted later
            }
            assert(((current_absolute_offset - abs_intargs_offset) / sizeof(offset) == funnode.input_targs.size()));

            add_padding(alignof(offset));

            // add output targets to nodedata
            size_t abs_outtargs_offset = current_absolute_offset;
            for (Node* outtarg : funnode.output_targs) {
                int pos = get_position<Node>(*funnode.get_children(), outtarg);
                assert(pos >= 0);
                assert(pos <= UINT16_MAX);
                add<offset>(offset(static_cast<offset>(pos))); // is converted later
            }
            assert(((current_absolute_offset - abs_outtargs_offset) / sizeof(offset) == funnode.output_targs.size()));

            add_padding(alignof(input));

            // add inputs
            size_t abs_inputs_offset = current_absolute_offset;
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
            size_t abs_outputs_offset = current_absolute_offset;
            for (const Output_connector& outconn : node.outputs) {
                add<output>({ outconn.state });
            }

            add_padding(NODE_ALIGNMENT);

            // Add child nodes recursively
            size_t abs_children_offset = current_absolute_offset;

            bus_map_t new_bus_map;
            for (Node* child : *funnode.get_children()) {
                assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");
                add_node(*child, new_bus_map, abs_children_offset);
                assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");
            }

            // Convert targets into offsets
            {
                FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);
                size_t intarg_count = header->input_targ_node_count;

                size_t converted_intargs = 0;
                // Update input target offsets
                for (size_t i = 0; i < intarg_count; i++) {
                    size_t abs_curr_child_offset = abs_children_offset;
                    offset intarg_idx = *get_at_abs<offset>(abs_intargs_offset + i * sizeof(offset));
                    for (size_t j = 0; j < header->child_count; j++) {
                        if (intarg_idx == j) {
                            offset* curr_intarg = get_at_abs<offset>(abs_intargs_offset + i * sizeof(offset));
                            *curr_intarg = abs_curr_child_offset - abs_node_offset;
                            ++converted_intargs;
                            break;
                        }
                        // Move to the next child
                        assert(abs_curr_child_offset % NODE_ALIGNMENT == 0);
                        abs_curr_child_offset += get_at_abs<NodeHeader>(abs_curr_child_offset)->total_size;
                    }
                }
                assert(converted_intargs == intarg_count);

                size_t outtarg_count = header->output_targ_node_count;

                size_t converted_outtargs = 0;
                // Update output target offsets
                for (size_t i = 0; i < outtarg_count; i++) {
                    size_t abs_curr_child_offset = abs_children_offset;
                    offset outtarg_idx = *get_at_abs<offset>(abs_outtargs_offset + i * sizeof(offset));
                    for (size_t j = 0; j < header->child_count; j++) {
                        if (outtarg_idx == j) {
                            offset* curr_uttarg = get_at_abs<offset>(abs_outtargs_offset + i * sizeof(offset));
                            *curr_uttarg = abs_curr_child_offset - abs_node_offset;
                            ++converted_outtargs;
                            break;
                        }
                        // Move to the next child
                        abs_curr_child_offset += get_at_abs<NodeHeader>(abs_curr_child_offset)->total_size;
                    }
                }
                assert(converted_outtargs == outtarg_count);

                {
                    FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);
                    if (header->child_count) {
                        uint8_t* children_container = get_at_abs<uint8_t>(abs_children_offset);
                        connect_children(children_container, header->child_count);
                    }
                }

            }

            {
                offset intargs_offset   = abs_intargs_offset  - abs_node_offset;
                offset outtargs_offset  = abs_outtargs_offset - abs_node_offset;
                offset inputs_offset    = abs_inputs_offset   - abs_node_offset;
                offset outputs_offset   = abs_outputs_offset  - abs_node_offset;
                offset children_offset  = abs_children_offset - abs_node_offset;

                FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);
                header->intargs_offset = intargs_offset;
                header->outtargs_offset = outtargs_offset;
                header->inputs_offset = inputs_offset;
                header->outputs_offset = outputs_offset;
                header->children_offset = children_offset;
            }
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
        size_t abs_inputs_offset = current_absolute_offset;
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
        bool is_first_node_in_bus = bus_map.count(busnode.label) == 0;
        if (is_first_node_in_bus) {
            size_t abs_shared_outputs_offset = current_absolute_offset;
            offset shared_outputs_offset = abs_shared_outputs_offset - abs_container_offset;
            for (size_t i = 0; i < busnode.bus_values_size(); i++) {
                add<output>(false); //TODO! Make these copy the state over
            }
            size_t abs_shared_new_outputs_offset = current_absolute_offset;
            offset shared_new_outputs_offset = abs_shared_new_outputs_offset - abs_container_offset;
            for (size_t i = 0; i < busnode.bus_values_size(); i++) {
                add<output>(false);
            }
            bus_map.insert({ busnode.label, { shared_outputs_offset, shared_new_outputs_offset } });
        }
        {
            offset inputs_offset = abs_inputs_offset - abs_node_offset;

            BusNodeHeader* header = get_at_abs<BusNodeHeader>(abs_node_offset); // gets destroyed when adding
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
        NodeHeader* header = get_at_abs<NodeHeader>(abs_node_offset);
        header->total_size = current_absolute_offset - abs_node_offset;
        assert(current_absolute_offset % NODE_ALIGNMENT == 0 && "misaligned node");
        assert(header->total_size % NODE_ALIGNMENT == 0 && "misaligned size");
        
        if (header->type == NodeType::FunctionNode) {
            FunctionNode& funnode = static_cast<FunctionNode&>(node);
            funnode.allocate_function_data(get_at_abs<uint8_t>(abs_node_offset));


            {//check header
                FunctionNodeHeader* header = get_at_abs<FunctionNodeHeader>(abs_node_offset);
                assert(header->type == funnode.get_type());
                assert(header->input_count == funnode.inputs.size());
                assert(header->output_count == funnode.outputs.size());
                assert(header->total_size % NODE_ALIGNMENT == 0);
                assert(current_absolute_offset % NODE_ALIGNMENT == 0);
            }

            {//check header
                FunctionNodeHeader* header = reinterpret_cast<FunctionNodeHeader*>(funnode.get_function_data()->get_data(0));
                assert(header->type == funnode.get_type());
                assert(header->input_count == funnode.inputs.size());
                assert(header->output_count == funnode.outputs.size());
                assert(header->total_size % NODE_ALIGNMENT == 0);
                assert(current_absolute_offset % NODE_ALIGNMENT == 0);
            }
        }
    }
    node.set_abs_node_offset(abs_node_offset);
    return;
    //TODO! store offsets in targeted node
}

void LogicBlockBuilder::connect_children(uint8_t* container, size_t child_count)
{
    using namespace LogicblockTools;
    offset curr_child_offset = 0;

    for (size_t i = 0; i < child_count; i++) {
        NodeHeader* curr_child_header = get_at<NodeHeader>(curr_child_offset, container);
        // Connect the inputs of the child nodes
        

        // ensure there are inputs to connect
        if (!(input_count(curr_child_offset, container) > 0)) {
            // Move to the next child offset
            curr_child_offset += get_at<NodeHeader>(curr_child_offset, container)->total_size;
            continue;
        }

        input* curr_child_inputs = get_at<input>(curr_child_offset + inputs_offset(curr_child_offset, container), container);

        std::vector<bool> converted_inputs(input_count(curr_child_offset, container), false);

        // Convert all unconnected inputs(indicated by being UINT32_MAX) to 0
        for (size_t input_idx = 0; input_idx < input_count(curr_child_offset, container); input_idx++) {
            if (curr_child_inputs[input_idx].target_offset == UINT32_MAX) {
                curr_child_inputs[input_idx].target_offset = 0;
                converted_inputs[input_idx] = true;
            }
        }
        offset curr_inner_child_offset = 0;

        for (size_t j = 0; j < child_count; j++) {

            for (size_t input_idx = 0; input_idx < input_count(curr_child_offset, container); input_idx++) {
                NodeHeader* curr_inner_child_header = get_at<NodeHeader>(curr_inner_child_offset, container);
                if (!converted_inputs[input_idx]) {
                    if (curr_child_inputs[input_idx].location.node_pos == j) {
                        // This input should be connected to an output of the current inner child

                        offset curr_outputs_offset;
                        if(curr_inner_child_header->type == NodeType::BusNode)
                            curr_outputs_offset = outputs_offset(curr_inner_child_offset, container);
                        else
                            curr_outputs_offset = curr_inner_child_offset + outputs_offset(curr_inner_child_offset, container);


                        curr_child_inputs[input_idx].target_offset = curr_outputs_offset + curr_child_inputs[input_idx].location.connector_index * sizeof(output);
                        converted_inputs[input_idx] = true;
                        assert(curr_child_inputs[input_idx].target_offset < buffer.size());
                    }
                }
            }

            curr_inner_child_offset += get_at<NodeHeader>(curr_inner_child_offset, container)->total_size;
        }

        // Check if all inputs were converted
        assert(std::all_of(converted_inputs.begin(), converted_inputs.end(), [](bool v) { return v; }) &&
            "Not all inputs were converted");

        // Move to the next child offset
        curr_child_offset += get_at<NodeHeader>(curr_child_offset, container)->total_size;
    }
}

size_t LogicBlock::parse(uint8_t* container, size_t abs_node_offset, std::string indent, size_t node_number) {
    using namespace LogicblockTools;
    assert(abs_node_offset < size);
    assert(abs_node_offset % 4 == 0);
    NodeHeader* header = LogicblockTools::get_at<NodeHeader>(abs_node_offset, container);
    std::cout << std::boolalpha << std::dec;
    assert(header->total_size % 4 == 0);
    std::cout 
        << indent << "header:\n"
        << indent << "  type: " << toString(header->type) << "\n"
        << indent << "  offset: " << abs_node_offset << "\n"
        //<< indent << "  number: " << node_number << "\n"
        << indent << "  total_size: " << header->total_size << "\n";
    switch (header->type) {
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        BinaryGateHeader* header = LogicblockTools::get_at<BinaryGateHeader>(abs_node_offset, container);
        std::cout
            << indent << "  output\n"
            //<< indent << "   value: " << header->output << "\n"
            << indent << "   offset: " << abs_node_offset + (&header->output - get_at<bool>(abs_node_offset, container)) * sizeof(bool) << "\n";
        for (size_t i = 0; i < header->input_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(abs_node_offset + header->inputs_offset + i * sizeof(input), container)->target_offset << "\n";
        }
        break;
    }
    case NodeType::GateBUFFER:
    case NodeType::GateNOT: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(abs_node_offset, container);
        for (size_t i = 0; i < header->input_output_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(abs_node_offset + header->inputs_offset + i * sizeof(input), container)->target_offset << "\n";
        }
        for (size_t i = 0; i < header->input_output_count; i++) {
            std::cout
                << indent << "  output\n"
                //<< indent << "   value: " << *get_at<output>(header->outputs_offset + i * sizeof(output)) << "\n"
                << indent << "   offset: " << abs_node_offset + header->outputs_offset + i * sizeof(output) << "\n";
        }
        break;
    }
    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton: {
        InputNodeHeader* header = get_at<InputNodeHeader>(abs_node_offset, container);
        for (size_t i = 0; i < header->output_count; i++) {
            std::cout
                << indent << "  output\n"
                //<< indent << "   value: " << *get_at<output>(header->outputs_offset + i * sizeof(output)) << "\n"
                << indent << "   offset: " << abs_node_offset + header->outputs_offset + i * sizeof(output) << "\n";
        }
        break;
    }
    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        OutputNodeHeader* header = get_at<OutputNodeHeader>(abs_node_offset, container);
        for (size_t i = 0; i < header->input_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(abs_node_offset + header->inputs_offset + i * sizeof(input), container)->target_offset << "\n";
        }
        break;
    }

    case NodeType::FunctionNode: {
        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(abs_node_offset, container);

        for (size_t i = 0; i < header->input_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(abs_node_offset + header->inputs_offset + i * sizeof(input), container)->target_offset << "\n";
        }
        for (size_t i = 0; i < header->output_count; i++) {
            std::cout
                << indent << "  output\n"
                //<< indent << "   value: " << *get_at<output>(header->outputs_offset + i * sizeof(output)) << "\n"
                << indent << "   offset: " << abs_node_offset + header->outputs_offset + i * sizeof(output) << "\n";
        }
        std::cout << indent << "children:\n";
        indent.append("  ");
        offset new_offset = 0;
        size_t new_node_number = node_number + 1;
        uint8_t* child_container = container + abs_node_offset + header->children_offset;

        for (size_t i = 0; i < header->child_count; i++) {
            new_offset += parse(child_container, new_offset, indent, new_node_number);
            ++new_node_number;
        }
        break;
    }

    case NodeType::BusNode: {
        BusNodeHeader* header = get_at<BusNodeHeader>(abs_node_offset, container);
        for (size_t i = 0; i < header->input_output_count; i++) {
            std::cout
                << indent << "  input\n"
                << indent << "   target offset: " << get_at<input>(abs_node_offset + header->inputs_offset + i * sizeof(input), container)->target_offset << "\n";
        }
        for (size_t i = 0; i < header->input_output_count; i++) {
            std::cout
                << indent << "  output\n"
                //<< indent << "   value: " << *get_at<output>(header->outputs_offset + i * sizeof(output)) << "\n"
                << indent << "   offset: " << header->shared_outputs_offset + i * sizeof(output) << "\n";
        }
        break;
    }

    case NodeType::RootNode: {
        RootNodeHeader* header = get_at<RootNodeHeader>(abs_node_offset, container);
        std::cout << indent << "children:\n";
        indent.append("  ");
        offset new_offset = 0;
        size_t new_node_number = node_number + 1;
        uint8_t* child_container = container + abs_node_offset + header->children_offset;
        for (size_t i = 0; i < header->child_count; i++) {
            new_offset += parse(child_container, new_offset, indent, new_node_number);
            ++new_node_number;
        }
        break;
    }
    default: {
        assert(false && "bogus amogus node type");
    }
    }
    return header->total_size;
}

bool LogicBlock::pretick(uint8_t* container, bool update_all, offset node_offset){
    using namespace LogicblockTools;
    NodeHeader* header = get_at<NodeHeader>(node_offset, container);
    switch (header->type) {
    case NodeType::GateAND: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset, container);
        header->new_output = true;
        for (size_t i = 0; header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset, node_offset, container);
        }
        return header->new_output != header->output;
    }
    case NodeType::GateOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset, container);
        header->new_output = false;
        for (size_t i = 0; !header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset, node_offset, container);
        }
        return header->new_output != header->output;
    }
    case NodeType::GateNAND: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset, container);
        header->new_output = true;
        for (size_t i = 0; header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset, node_offset, container);
        }
        header->new_output = !header->new_output;
        return header->new_output != header->output;
    }
    case NodeType::GateNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset, container);
        header->new_output = false;
        for (size_t i = 0; !header->new_output && i < header->input_count; i++) {
            header->new_output = get_input_val(i, header->inputs_offset, node_offset, container);
        }
        header->new_output = !header->new_output;
        return header->new_output != header->output;
    }
    case NodeType::GateXOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset, container);
        header->new_output = false;
        for (size_t i = 0; i < header->input_count; i++) {
            if (get_input_val(i, header->inputs_offset, node_offset, container)) {
                header->new_output = !header->new_output;
            }
        }
        return header->new_output != header->output;
    }
    case NodeType::GateXNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset, container);
        header->new_output = true;
        for (size_t i = 0; i < header->input_count; i++) {
            if (get_input_val(i, header->inputs_offset, node_offset, container)) {
                header->new_output = !header->new_output;
            }
        }
        return header->new_output != header->output;
    }
    case NodeType::GateBUFFER: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset, container);
        bool has_changed = false;
        for (size_t i = 0; i < header->input_output_count; i++) {
            output* new_output = get_at<output>(node_offset + header->new_outputs_offset + i * sizeof(output), container);
            *new_output = get_input_val(i, header->inputs_offset, node_offset, container);

            output* curr_output = get_at<output>(node_offset + header->outputs_offset + i * sizeof(output), container);
            if (*new_output != *curr_output) {
                has_changed = true;
            }
        }
        return has_changed;
    }
    case NodeType::GateNOT: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset, container);
        bool has_changed = false;
        for (size_t i = 0; i < header->input_output_count; i++) {
            output* new_output = get_at<output>(node_offset + header->new_outputs_offset + i * sizeof(output), container);
            *new_output = !get_input_val(i, header->inputs_offset, node_offset, container);

            output* curr_output = get_at<output>(node_offset + header->outputs_offset + i * sizeof(output), container);
            if (*new_output != *curr_output) {
                has_changed = true;
            }
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

        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset, container);

        // load input values into the outputs of the input targs
        bool new_input = false;
        size_t input_idx = 0;
        for (uint32_t i = 0; i < header->input_targ_node_count; i++) {
            offset input_targ_offset = node_offset + *get_at<offset>(node_offset + header->intargs_offset + i * sizeof(offset), container);
            InputNodeHeader* input_targ_header = get_at<InputNodeHeader>(input_targ_offset, container);

            for (uint32_t j = 0; j < input_targ_header->output_count; j++) {
                assert(input_idx < header->input_count);
                //__debugbreak(); // check that this is the correct offset
                output* outconn = get_at<output>(input_targ_offset + input_targ_header->outputs_offset + j * sizeof(output), container);
                bool input_val = get_input_val(input_idx, header->inputs_offset, node_offset, container);
                if(*outconn != input_val) new_input = true;
                *outconn = input_val;
                ++input_idx;
            }
        }

        //if needed,pretick 
        if (header->has_changed || new_input || update_all) {
            header->has_changed = false;

            offset curr_child_offset = node_offset + header->children_offset;
            uint8_t* children_container = get_at<uint8_t>(curr_child_offset, container);
            for (size_t i = 0; i < header->child_count; i++) {
                offset child_node_offset = curr_child_offset - node_offset - header->children_offset;
                NodeHeader* child_header = get_at<NodeHeader>(child_node_offset, children_container);
                if(pretick(children_container, update_all, child_node_offset)) header->has_changed = true;
                curr_child_offset += child_header->total_size;
            }

            if (new_input) header->has_changed = true;
        }
        return header->has_changed;
    }
    // Bus nodes are not as efficient as i want but similar to buffers
    case NodeType::BusNode: {
        BusNodeHeader* header = get_at<BusNodeHeader>(node_offset, container);
        bool has_changed = false;

        //is it the first node in the bus
        if (header->total_size > (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))){
            uint16_t shared_output_count = (header->total_size - (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))) / 2;
            for (size_t i = 0; i < shared_output_count; i++) {
                output* new_output = get_at<output>(header->shared_new_outputs_offset + i * sizeof(output), container);
                *new_output = false;
            }
        }

        for (size_t i = 0; i < header->input_output_count; i++) {
            output* new_output = get_at<output>(header->shared_new_outputs_offset + i * sizeof(output), container);
            if (get_input_val(i, header->inputs_offset, node_offset, container)) {
                if (!*new_output) {
                    *new_output = true;
                }
            }

            output* curr_output = get_at<output>(header->shared_outputs_offset + i * sizeof(output), container);
            if (*new_output != *curr_output) has_changed = true;
        }
        return has_changed;
        
    }
    
    case NodeType::RootNode: {
        RootNodeHeader* header = get_at<RootNodeHeader>(node_offset, container);

        offset curr_child_offset = header->children_offset;
        uint8_t* children_container = get_at<uint8_t>(header->children_offset, container);
        for (size_t i = 0; i < header->child_count; i++) {
            offset child_node_offset = curr_child_offset - header->children_offset;
            NodeHeader* child_header = get_at<NodeHeader>(child_node_offset, children_container);
            pretick(children_container, update_all, child_node_offset);
            curr_child_offset += child_header->total_size;
        }

        return true;
    }
    default: assert(false);
    return false;
    }
}

void LogicBlock::tick(uint8_t* container, bool update_all, offset node_offset)
{
    using namespace LogicblockTools;
    NodeHeader* header = get_at<NodeHeader>(node_offset, container);
    switch (header->type) {
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        BinaryGateHeader* header = get_at<BinaryGateHeader>(node_offset, container);
        header->output = header->new_output;
        return;
    }
    case NodeType::GateBUFFER:
    case NodeType::GateNOT: {
        UnaryGateHeader* header = get_at<UnaryGateHeader>(node_offset, container);
        for (size_t i = 0; i < header->input_output_count; i++) {
            *get_at<output>(node_offset + header->outputs_offset + i * sizeof(output), container) = *get_at<output>(node_offset + header->new_outputs_offset + i * sizeof(output), container);
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
        FunctionNodeHeader* header = get_at<FunctionNodeHeader>(node_offset, container);
        if (header->has_changed || update_all) {
            {
                offset curr_child_offset = node_offset + header->children_offset;
                uint8_t* children_container = get_at<uint8_t>(node_offset + header->children_offset, container);
                for (size_t i = 0; i < header->child_count; i++) {
                    NodeHeader* child_header = get_at<NodeHeader>(curr_child_offset, container);
                    offset rel_child_node_offset = curr_child_offset - node_offset - header->children_offset;
                    tick(children_container, update_all, rel_child_node_offset);
                    curr_child_offset += child_header->total_size;
                }
            }
            {
                uint32_t output_idx = 0;
                uint8_t* children_container = get_at<uint8_t>(node_offset + header->children_offset, container);
                for (size_t i = 0; i < header->output_targ_node_count; i++) {

                    // get location of output_target node reltive to the current conatiner
                    offset child_output_targ_offset = *get_at<offset>(node_offset + header->outtargs_offset + i * sizeof(offset), container) - header->children_offset;
                    OutputNodeHeader* output_targ_header = get_at<OutputNodeHeader>(child_output_targ_offset, children_container);

                    for (size_t j = 0; j < output_targ_header->input_count; j++) {
                        assert(output_idx < header->output_count);
                        output* function_output = get_at<output>(node_offset + header->outputs_offset + output_idx * sizeof(output), container);
                        bool output_targ_val = get_input_val(j, output_targ_header->inputs_offset, child_output_targ_offset, children_container); //get_input_val_needs a bufer in the right scope
                        *function_output = output_targ_val;
                        ++output_idx;
                    }
                }
            }

        }
        return;
    }
    case NodeType::BusNode: {
        BusNodeHeader* header = get_at<BusNodeHeader>(node_offset, container);

        //is it the first node in the bus
        if (header->total_size > (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))) {
            uint16_t shared_output_count = (header->total_size - (sizeof(BusNodeHeader) + header->input_output_count * sizeof(input))) / sizeof(output) / 2;
            for (size_t i = 0; i < shared_output_count; i++) {
                *get_at<output>(header->shared_outputs_offset + i * sizeof(output), container) = *get_at<output>(header->shared_new_outputs_offset + i * sizeof(output), container);
            }
        }
        return;
    }
    case NodeType::RootNode: {
        RootNodeHeader* header = get_at<RootNodeHeader>(node_offset, container);

        {
            offset curr_child_offset = node_offset + header->children_offset;
            uint8_t* children_container = get_at<uint8_t>(node_offset + header->children_offset, container);
            for (size_t i = 0; i < header->child_count; i++) {
                NodeHeader* child_header = get_at<NodeHeader>(curr_child_offset, container);
                offset rel_child_node_offset = curr_child_offset - node_offset - header->children_offset;
                tick(children_container, update_all, rel_child_node_offset);
                curr_child_offset += child_header->total_size;
            }
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
        return 0;
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

// header offset iis relative to buffer
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
        offset byteDistance = (char*)&gate->output - (char*)gate;
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

offset LogicblockTools::inputs_offset(offset header_offset, uint8_t* buffer) {
    NodeHeader* header = get_at<NodeHeader>(header_offset, buffer);
    switch (header->type) {
        // Binary Gates
    case NodeType::GateAND:
    case NodeType::GateOR:
    case NodeType::GateNAND:
    case NodeType::GateNOR:
    case NodeType::GateXOR:
    case NodeType::GateXNOR: {
        return get_at<BinaryGateHeader>(header_offset, buffer)->inputs_offset;
    }

                           // Unary Gates
    case NodeType::GateBUFFER:
    case NodeType::GateNOT: {
        return get_at<UnaryGateHeader>(header_offset, buffer)->inputs_offset;
    }

                          // Input Nodes (only have outputs)
    case NodeType::PushButton:
    case NodeType::ToggleButton:
    case NodeType::StaticToggleButton: {
        return 0;
    }

                                     // Output Nodes (only have inputs, no outputs)
    case NodeType::LightBulb:
    case NodeType::SevenSegmentDisplay: {
        return get_at<OutputNodeHeader>(header_offset, buffer)->inputs_offset;
    }

    case NodeType::FunctionNode: {
        return get_at<FunctionNodeHeader>(header_offset, buffer)->inputs_offset;
    }

    case NodeType::BusNode: {
        return get_at<BusNodeHeader>(header_offset, buffer)->inputs_offset;
    }

    default:
        assert(false && "Invalid NodeType");
        return 0; // Return 0 for invalid types, though this line should never be reached due to the assert
    }
}

// inputs_offset is relative to node start, node offset is relative to buffer, buffer must be the container of the nodes
output LogicblockTools::get_input_val(size_t index, offset inputs_offset, offset node_offset, uint8_t* buffer) {
    input* inconn = get_at<input>(node_offset + inputs_offset + index * sizeof(input), buffer);
    if (!inconn->target_offset) return false;
    output* targ_output = get_at<output>(inconn->target_offset, buffer);
    return *targ_output;
}
