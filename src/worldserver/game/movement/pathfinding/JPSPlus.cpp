/*
* JPSPlus.cpp
*
* Copyright (c) 2014-2015, Steve Rabin
* All rights reserved.
*
* An explanation of the JPS+ algorithm is contained in Chapter 14
* of the book Game AI Pro 2, edited by Steve Rabin, CRC Press, 2015.
* A presentation on Goal Bounding titled "JPS+: Over 100x Faster than A*"
* can be found at www.gdcvault.com from the 2015 GDC AI Summit.
* A copy of this code is on the website http://www.gameaipro.com.
*
* If you develop a way to improve this code or make it faster, please
* contact steve.rabin@gmail.com and share your insights. I would
* be equally eager to hear from anyone integrating this code or using
* the Goal Bounding concept in a commercial application or game.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*    * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*    * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*    * The name of the author may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STEVE RABIN ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <copyright holder> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ 

#include "JPSPlus.h"

#include "game/maps/BattleMap.h"

// Ideal choice of fixed-point equivalent to 1.0 that can almost perfectly represent sqrt(2) and (sqrt(2) - 1) in whole numbers
// 1.000000000 = 2378
// 0.414213624 = 985 / 2378
// 1.414213625 = 3363 / 2378
// 1.414213562 = Actual sqrt(2)
// 0.00000006252 = Difference between actual sqrt(2) and fixed-point sqrt(2)
#define FIXED_POINT_MULTIPLIER 2378
#define FIXED_POINT_SHIFT(x) ((x)* FIXED_POINT_MULTIPLIER)
#define SQRT_2 3363
#define SQRT_2_MINUS_ONE 985

typedef const void (JPSPlus::*FunctionPointer)(PathfindingNode* currentNode, DistantJumpPoints* map);

JPSPlus::JPSPlus(PrecomputeMap* precomputeMap) :
	m_precomputeMap(precomputeMap),
	m_width(0),
	m_height(0),
	m_fastStack(nullptr),
	m_simpleUnsortedPriorityQueue(nullptr),
	m_mapNodes(nullptr),
	m_currentIteration(0),
	m_goalNode(nullptr),
	m_goalRow(0),
	m_goalCol(0)
{
	// Map properties
	m_width = precomputeMap->getWidth();
	m_height = precomputeMap->getHeight();

	// Adjust preallocation for worst-case
	m_simpleUnsortedPriorityQueue = new SimpleUnsortedPriorityQueue(10000);
	m_fastStack = new FastStack(1000);

	m_currentIteration = 1;	// This gets incremented on each search

	// Initialize nodes
	initArray(m_mapNodes, m_width, m_height);
	for (int32 r = 0; r<m_height; r++)
	{
		for (int32 c = 0; c<m_width; c++)
		{
			PathfindingNode& node = m_mapNodes[r][c];
			node.m_row = r;
			node.m_col = c;
			node.m_listStatus = PATHFINDING_NODE_STATUS_ON_NONE;
			node.m_iteration = 0;
		}
	}
}

JPSPlus::~JPSPlus()
{
	delete m_fastStack;
	delete m_simpleUnsortedPriorityQueue;
	destroyArray(m_mapNodes);

	m_goalNode = nullptr;
	m_precomputeMap = nullptr;
}

template <typename T>
void JPSPlus::initArray(T**& t, int32 width, int32 height)
{
	t = new T*[height];
	for (int32 i = 0; i < height; ++i)
	{
		t[i] = new T[width];
		memset(t[i], 0, sizeof(T)*width);
	}
}

template <typename T>
void JPSPlus::destroyArray(T**& t)
{
	for (int32 i = 0; i < m_height; ++i)
		delete[] t[i];
	delete[] t;

	t = 0;
}

bool JPSPlus::getPath(TileCoord const& start, TileCoord const& goal, std::vector<TileCoord>& path)
{
	int32 startRow = start.y;
	int32 startCol = start.x;
	m_goalRow = goal.y;
	m_goalCol = goal.x;

	{
		// Initialize map
		path.clear();
		m_precomputeMap->calculateMap();

		m_goalNode = &m_mapNodes[m_goalRow][m_goalCol];
		m_currentIteration++;

		m_fastStack->reset();
		m_simpleUnsortedPriorityQueue->reset();
	}

	// Create starting node
	PathfindingNode* startNode = &m_mapNodes[startRow][startCol];
	startNode->m_parent = NULL;
	startNode->m_givenCost = 0;
	startNode->m_finalCost = 0;
	startNode->m_listStatus = PATHFINDING_NODE_STATUS_ON_OPEN;
	startNode->m_iteration = m_currentIteration;

	// Actual search
	PathStatus status = searchLoop(startNode);

	if (status == PATH_STATUS_PATH_FOUND)
	{
		finalizePath(path);
		if (path.size() > 0)
		{
			return true;
		}
		return false;
	}
	else
	{
		// No path
		return false;
	}
}

PathStatus JPSPlus::searchLoop(PathfindingNode* startNode)
{
	// Create 2048 entry function pointer lookup table
	#define CASE(x) &JPSPlus::explore_##x,
	static const FunctionPointer exploreDirections[2048] = 
	{ 
		#include "ExploreDirections.h"
	};
	#undef CASE

	{
		// Special case for the starting node

		if (startNode == m_goalNode)
		{
			return PATH_STATUS_PATH_FOUND;
		}

		DistantJumpPoints* distantJumpPoints = &m_precomputeMap->getPrecomputedMap()[startNode->m_row][startNode->m_col];
		explore_AllDirections(startNode, distantJumpPoints);
		startNode->m_listStatus = PATHFINDING_NODE_STATUS_ON_CLOSED;
	}

	while (!m_simpleUnsortedPriorityQueue->empty() || !m_fastStack->empty())
	{
		PathfindingNode* currentNode;

		if(!m_fastStack->empty())
		{
			currentNode = m_fastStack->pop();
		}
		else
		{
			currentNode = m_simpleUnsortedPriorityQueue->pop();
		}

		if (currentNode == m_goalNode)
		{
			return PATH_STATUS_PATH_FOUND;
		}
		
		// Explore nodes based on parent
		DistantJumpPoints* distantJumpPoints = &m_precomputeMap->getPrecomputedMap()[currentNode->m_row][currentNode->m_col];

		(this->*exploreDirections[(distantJumpPoints->blockedDirectionBitfield* 8) +
			currentNode->m_directionFromParent])(currentNode, distantJumpPoints);

		currentNode->m_listStatus = PATHFINDING_NODE_STATUS_ON_CLOSED;
	}
	return PATH_STATUS_NO_PATH_EXISTS;
}

void JPSPlus::finalizePath(std::vector<TileCoord>& finalPath)
{
	//PathfindingNode* prevNode = NULL;
	PathfindingNode* curNode = m_goalNode;

	while (curNode != NULL)
	{
		TileCoord coord;
		coord.x = curNode->m_col;
		coord.y = curNode->m_row;

		curNode = curNode->m_parent;
		if(curNode)
			finalPath.push_back(coord);
	}
	std::reverse(finalPath.begin(), finalPath.end());
}

inline const void JPSPlus::explore_Null(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	// Purposely does nothing
}

inline const void JPSPlus::explore_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

inline const void JPSPlus::explore_DR(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
}

inline const void JPSPlus::explore_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

inline const void JPSPlus::explore_UR(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
}

inline const void JPSPlus::explore_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_UL(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
}

inline const void JPSPlus::explore_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_DL(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
}

// Adjacent Doubles

inline const void JPSPlus::explore_D_DR(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
}

inline const void JPSPlus::explore_DR_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

inline const void JPSPlus::explore_R_UR(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
}

inline const void JPSPlus::explore_UR_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_U_UL(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
}

inline const void JPSPlus::explore_UL_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_L_DL(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
}

inline const void JPSPlus::explore_DL_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

// Non-Adjacent Cardinal Doubles

inline const void JPSPlus::explore_D_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

inline const void JPSPlus::explore_R_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_U_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_L_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

inline const void JPSPlus::explore_D_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_R_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

// Adjacent Triples

inline const void JPSPlus::explore_D_DR_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

inline const void JPSPlus::explore_DR_R_UR(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
}

inline const void JPSPlus::explore_R_UR_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_UR_U_UL(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
}

inline const void JPSPlus::explore_U_UL_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_UL_L_DL(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
}

inline const void JPSPlus::explore_L_DL_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

inline const void JPSPlus::explore_DL_D_DR(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
}

// Non-Adjacent Cardinal Triples

inline const void JPSPlus::explore_D_R_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_R_U_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_U_L_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

inline const void JPSPlus::explore_L_D_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

// Quads

inline const void JPSPlus::explore_R_DR_D_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_R_D_DL_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_U_UR_R_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

inline const void JPSPlus::explore_U_R_DR_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

inline const void JPSPlus::explore_L_UL_U_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

inline const void JPSPlus::explore_L_U_UR_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

inline const void JPSPlus::explore_D_DL_L_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_D_L_UL_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

// Quints

inline const void JPSPlus::explore_R_DR_D_DL_L(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
}

inline const void JPSPlus::explore_U_UR_R_DR_D(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
}

inline const void JPSPlus::explore_L_UL_U_UR_R(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
}

inline const void JPSPlus::explore_D_DL_L_UL_U(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
}

inline const void JPSPlus::explore_AllDirections(PathfindingNode* currentNode, DistantJumpPoints* map)
{
	searchDown(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN]);
	searchDownLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT]);
	searchLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_LEFT]);
	searchUpLeft(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_LEFT]);
	searchUp(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP]);
	searchUpRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT]);
	searchRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_RIGHT]);
	searchDownRight(currentNode, map->jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT]);
}

void JPSPlus::searchDown(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Consider straight line to Goal
	if (col == m_goalCol && row < m_goalRow)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((row + absJumpDistance) >= m_goalRow)
		{
			uint32 diff = m_goalRow - row;
			uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode* newSuccessor = m_goalNode;
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_DOWN, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newRow = row + jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[newRow][col];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_DOWN, givenCost);
	}
}

void JPSPlus::searchDownRight(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row < m_goalRow && col < m_goalCol)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int32 diffRow = m_goalRow - row;
		int32 diffCol = m_goalCol - col;
		int32 smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int32 newRow = row + smallerDiff;
			int32 newCol = col + smallerDiff;
			uint32 givenCost = currentNode->m_givenCost + (SQRT_2* smallerDiff);
			PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_DOWN_RIGHT, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newRow = row + jumpDistance;
		int32 newCol = col + jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + (SQRT_2* jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_DOWN_RIGHT, givenCost);
	}
}

void JPSPlus::searchRight(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Consider straight line to Goal
	if (row == m_goalRow && col < m_goalCol)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((col + absJumpDistance) >= m_goalCol)
		{
			uint32 diff = m_goalCol - col;
			uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode* newSuccessor = m_goalNode;
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_RIGHT, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newCol = col + jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[row][newCol];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_RIGHT, givenCost);
	}
}

void JPSPlus::searchUpRight(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row > m_goalRow && col < m_goalCol)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int32 diffRow = row - m_goalRow;
		int32 diffCol = m_goalCol - col;
		int32 smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int32 newRow = row - smallerDiff;
			int32 newCol = col + smallerDiff;
			uint32 givenCost = currentNode->m_givenCost + (SQRT_2* smallerDiff);
			PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_UP_RIGHT, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newRow = row - jumpDistance;
		int32 newCol = col + jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + (SQRT_2* jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_UP_RIGHT, givenCost);
	}
}

void JPSPlus::searchUp(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Consider straight line to Goal
	if (col == m_goalCol && row > m_goalRow)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((row - absJumpDistance) <= m_goalRow)
		{
			uint32 diff = row - m_goalRow;
			uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode* newSuccessor = m_goalNode;
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_UP, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newRow = row - jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[newRow][col];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_UP, givenCost);
	}
}

void JPSPlus::searchUpLeft(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row > m_goalRow && col > m_goalCol)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int32 diffRow = row - m_goalRow;
		int32 diffCol = col - m_goalCol;
		int32 smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int32 newRow = row - smallerDiff;
			int32 newCol = col - smallerDiff;
			uint32 givenCost = currentNode->m_givenCost + (SQRT_2* smallerDiff);
			PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_UP_LEFT, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newRow = row - jumpDistance;
		int32 newCol = col - jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + (SQRT_2* jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_UP_LEFT, givenCost);
	}
}

void JPSPlus::searchLeft(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Consider straight line to Goal
	if (row == m_goalRow && col > m_goalCol)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		if ((col - absJumpDistance) <= m_goalCol)
		{
			uint32 diff = col - m_goalCol;
			uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(diff);
			PathfindingNode* newSuccessor = m_goalNode;
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_LEFT, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newCol = col - jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + FIXED_POINT_SHIFT(jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[row][newCol];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_LEFT, givenCost);
	}
}

void JPSPlus::searchDownLeft(PathfindingNode* currentNode, int32 jumpDistance)
{
	int32 row = currentNode->m_row;
	int32 col = currentNode->m_col;

	// Check for goal in general direction (straight line to Goal or Target Jump Point)
	if (row < m_goalRow && col > m_goalCol)
	{
		int32 absJumpDistance = jumpDistance;
		if (absJumpDistance < 0) { absJumpDistance = -absJumpDistance; }

		int32 diffRow = m_goalRow - row;
		int32 diffCol = col - m_goalCol;
		int32 smallerDiff = diffRow;
		if (diffCol < smallerDiff) { smallerDiff = diffCol; }

		if (smallerDiff <= absJumpDistance)
		{
			int32 newRow = row + smallerDiff;
			int32 newCol = col - smallerDiff;
			uint32 givenCost = currentNode->m_givenCost + (SQRT_2* smallerDiff);
			PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
			pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_DOWN_LEFT, givenCost);
			return;
		}
	}

	if (jumpDistance > 0)
	{
		// Directly jump
		int32 newRow = row + jumpDistance;
		int32 newCol = col - jumpDistance;
		uint32 givenCost = currentNode->m_givenCost + (SQRT_2* jumpDistance);
		PathfindingNode* newSuccessor = &m_mapNodes[newRow][newCol];
		pushNewNode(newSuccessor, currentNode, ARRAY_DIRECTIONS_DOWN_LEFT, givenCost);
	}
}

void JPSPlus::pushNewNode(
	PathfindingNode* newSuccessor, 
	PathfindingNode* currentNode, 
	ArrayDirections parentDirection, 
	uint32 givenCost)
{
	if (newSuccessor->m_iteration != m_currentIteration)
	{
		// Place node on the Open list (we've never seen it before)

		// Compute heuristic using octile calculation (optimized: minDiff* SQRT_2_MINUS_ONE + maxDiff)
		uint32 diffrow = abs(m_goalRow - newSuccessor->m_row);
		uint32 diffcolumn = abs(m_goalCol - newSuccessor->m_col);
		uint32 heuristicCost;
		if (diffrow <= diffcolumn)
		{
			heuristicCost = (diffrow* SQRT_2_MINUS_ONE) + FIXED_POINT_SHIFT(diffcolumn);
		}
		else
		{
			heuristicCost = (diffcolumn* SQRT_2_MINUS_ONE) + FIXED_POINT_SHIFT(diffrow);
		}

		newSuccessor->m_parent = currentNode;
		newSuccessor->m_directionFromParent = parentDirection;
		newSuccessor->m_givenCost = givenCost;
		newSuccessor->m_finalCost = givenCost + heuristicCost;
		newSuccessor->m_listStatus = PATHFINDING_NODE_STATUS_ON_OPEN;
		newSuccessor->m_iteration = m_currentIteration;

		if(newSuccessor->m_finalCost <= currentNode->m_finalCost)
		{
			m_fastStack->push(newSuccessor);
		}
		else
		{
			m_simpleUnsortedPriorityQueue->add(newSuccessor);
		}
	}
	else if (givenCost < newSuccessor->m_givenCost &&
		newSuccessor->m_listStatus == PATHFINDING_NODE_STATUS_ON_OPEN)	// Might be valid to remove this 2nd condition for extra speed (a node on the closed list wouldn't be cheaper)
	{
		// We found a cheaper way to this node - update node

		// Extract heuristic cost (was previously calculated)
		uint32 heuristicCost = newSuccessor->m_finalCost - newSuccessor->m_givenCost;

		newSuccessor->m_parent = currentNode;
		newSuccessor->m_directionFromParent = parentDirection;
		newSuccessor->m_givenCost = givenCost;
		newSuccessor->m_finalCost = givenCost + heuristicCost;

		// No decrease key operation necessary (already in unsorted open list)
	}
}