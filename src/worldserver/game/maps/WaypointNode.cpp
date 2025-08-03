#include "WaypointNode.h"

WaypointNode::WaypointNode(TileCoord const& position) :
	m_position(position)
{}

WaypointNode::~WaypointNode()
{
	this->unlink();
}


void WaypointNode::link(WaypointNode* node)
{
	auto ret = node->m_prevNodes.insert(this);
	NS_ASSERT(ret.second);
	ret = m_nextNodes.insert(node);
	NS_ASSERT(ret.second);
}

void WaypointNode::unlink()
{
	if (this->isIsolated())
		return;

	for (auto it = m_nextNodes.begin(); it != m_nextNodes.end(); )
	{
		WaypointNode* node = *it;
		node->m_prevNodes.erase(this);
		it = m_nextNodes.erase(it);
	}

	for (auto it = m_prevNodes.begin(); it != m_prevNodes.end(); )
	{
		WaypointNode* node = *it;
		node->m_nextNodes.erase(this);
		it = m_prevNodes.erase(it);
	}
}