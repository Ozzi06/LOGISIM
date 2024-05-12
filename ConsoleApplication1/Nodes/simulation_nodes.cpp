#include "simulation_nodes.h"

LogicNode* LogicNodeContainer::add_node(
	void (*pretickFunc)(LogicNode&),
	bool (*tickFunc)(LogicNode&),
	void (*destructor)(LogicNode&)
)
{
	nodes.push_back(
		LogicNode(this, pretickFunc, tickFunc, destructor)
	);
	return &nodes.back();
}

bool LogicNodeContainer::connect_node(LogicNode* from_ptr, uint8_t from_output_idx, LogicNode* to_ptr, uint8_t to_input_idx)
{
	int from_node_idx = -1;
	for (int i = 0; i < nodes.size(); i++) {
		if (&nodes[i] == from_ptr) { 
			from_node_idx = i;
			break; 
		}
	}
	if(from_node_idx < 0 || from_output_idx > 0b11111) return false;

	to_ptr->input_idxs[to_input_idx] = NodeConnectionIndex(from_node_idx, from_output_idx);
	return true;
}
