/*
 * FastStack.h
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

#ifndef __FAST_STACK_H__
#define __FAST_STACK_H__

#include "PathfindingNode.h"

// This class is designed to be used only for grid searches that use the Octile heuristic.
// In this specific use case, a stack can be used instead of a traditional open list to 
// hold neighboring all nodes that have a final cost equal to the parent node. When the
// cheapest node is needed, exhaust this stack first before pulling off of the open list.

class FastStack
{
public:
	FastStack(int32 size) : m_nextFreeNode(0) { m_nodeArray = new PathfindingNode*[size]; }
	~FastStack(void) { delete[] m_nodeArray; }

	inline void reset(void) { m_nextFreeNode = 0; }
	inline bool empty(void) { return m_nextFreeNode == 0; }
	inline void push(PathfindingNode* node) { m_nodeArray[m_nextFreeNode++] = node; }
	PathfindingNode* pop(void) { return m_nodeArray[--m_nextFreeNode]; }

private:
	int32 m_nextFreeNode;
	PathfindingNode** m_nodeArray;
};

#endif // __FAST_STACK_H__

