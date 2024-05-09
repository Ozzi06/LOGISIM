#include "simulation_nodes.h"

LogicNode* NodeContainer::addNode(
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

void NodeContainer::removeNode(int16_t idx)
{

}
