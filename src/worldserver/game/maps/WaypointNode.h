#ifndef __WAYPOINT_NODE_H__
#define __WAYPOINT_NODE_H__

#include "Common.h"
#include "game/tiles/TileCoord.h"

class WaypointNode;

typedef std::unordered_map<int32 /* TileIndex */, WaypointNode*> WaypointNodeMap;

class WaypointNode
{
public:
	WaypointNode(TileCoord const& position);
	~WaypointNode();

	bool isIsolated() const { return m_nextNodes.empty() && m_prevNodes.empty(); }
	std::unordered_set<WaypointNode*> const& getNextNodes() const { return m_nextNodes; };
	std::unordered_set<WaypointNode*> const& getPrevNodes() const { return m_prevNodes; };

	void link(WaypointNode* node);
	void unlink();

	TileCoord const& getPosition() const { return m_position; }

private:
	TileCoord m_position;
	std::unordered_set<WaypointNode*> m_nextNodes;
	std::unordered_set<WaypointNode*> m_prevNodes;
};

#endif // __WAYPOINT_NODE_H__