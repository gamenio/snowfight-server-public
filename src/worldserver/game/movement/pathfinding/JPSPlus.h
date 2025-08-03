/*
 * JPSPlus.h
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
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * The name of the author may not be used to endorse or promote products
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

#ifndef __JPS_PLUS_H__
#define __JPS_PLUS_H__

#include "PathfindingNode.h"
#include "PrecomputeMap.h"
#include "FastStack.h"
#include "SimpleUnsortedPriorityQueue.h"
#include "PrecomputeMap.h"

// This version of JPS+ is intimately tied with Goal Bounding.
// Every effort has been made to maximize speed.
// If you develop a way to improve this code or make it faster, contact steve.rabin@gmail.com

enum PathStatus
{
	PATH_STATUS_WORKING,
	PATH_STATUS_PATH_FOUND,
	PATH_STATUS_NO_PATH_EXISTS
};

class JPSPlus
{
public:
	JPSPlus(PrecomputeMap* precomputeMap);
	~JPSPlus();

	bool getPath(TileCoord const& start, TileCoord const& goal, std::vector<TileCoord>& path);

protected:

	PathStatus searchLoop(PathfindingNode* startNode);
	void finalizePath(std::vector<TileCoord>& finalPath);

	// 48 function variations of exploring (used in 2048 entry look-up table)
	// D = Down, U = Up, R = Right, L = Left, DR = Down Right, DL = Down Left, UR = Up Right, UL = Up Left
	const void explore_Null(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_DR(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_UR(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_UL(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_DL(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_DR(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_DR_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_UR(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_UR_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U_UL(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_UL_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L_DL(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_DL_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_DR_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_DR_R_UR(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_UR_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_UR_U_UL(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U_UL_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_UL_L_DL(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L_DL_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_DL_D_DR(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_R_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_U_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U_L_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L_D_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_DR_D_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_D_DL_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U_UR_R_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U_R_DR_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L_UL_U_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L_U_UR_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_DL_L_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_L_UL_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_R_DR_D_DL_L(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_U_UR_R_DR_D(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_L_UL_U_UR_R(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_D_DL_L_UL_U(PathfindingNode* currentNode, DistantJumpPoints* map);
	const void explore_AllDirections(PathfindingNode* currentNode, DistantJumpPoints* map);

	void searchDown(PathfindingNode* currentNode, int32 jumpDistance);
	void searchDownRight(PathfindingNode* currentNode, int32 jumpDistance);
	void searchRight(PathfindingNode* currentNode, int32 jumpDistance);
	void searchUpRight(PathfindingNode* currentNode, int32 jumpDistance);
	void searchUp(PathfindingNode* currentNode, int32 jumpDistance);
	void searchUpLeft(PathfindingNode* currentNode, int32 jumpDistance);
	void searchLeft(PathfindingNode* currentNode, int32 jumpDistance);
	void searchDownLeft(PathfindingNode* currentNode, int32 jumpDistance);

	void pushNewNode(PathfindingNode* newSuccessor, PathfindingNode* currentNode, ArrayDirections parentDirection, uint32 givenCost);

	// 2D array initialization and destruction
	template <typename T> void initArray(T**& t, int32 width, int32 height);
	template <typename T> void destroyArray(T**& t);

	PrecomputeMap* m_precomputeMap;

	// Map properties
	int32 m_width, m_height;

	// Open list structures
	FastStack* m_fastStack;
	SimpleUnsortedPriorityQueue* m_simpleUnsortedPriorityQueue;

	// Preallocated nodes
	PathfindingNode** m_mapNodes;

	// Search specific info
	uint32 m_currentIteration;	// This allows us to know if a node has been touched this iteration (faster than clearing all the nodes before each search)
	PathfindingNode* m_goalNode;
	int32 m_goalRow, m_goalCol;
};

#endif // __JPS_PLUS_H__

