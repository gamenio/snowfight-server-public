/*
 * PrecomputeMap.h
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

#ifndef __PRECOMPUTE_MAP_H__
#define __PRECOMPUTE_MAP_H__

#include <fstream>

#include "Defines.h"
#include "game/maps/MapData.h"

enum ArrayDirections
{
	ARRAY_DIRECTIONS_DOWN				= 0,
	ARRAY_DIRECTIONS_DOWN_RIGHT			= 1,
	ARRAY_DIRECTIONS_RIGHT				= 2,
	ARRAY_DIRECTIONS_UP_RIGHT			= 3,
	ARRAY_DIRECTIONS_UP					= 4,
	ARRAY_DIRECTIONS_UP_LEFT			= 5,
	ARRAY_DIRECTIONS_LEFT				= 6,
	ARRAY_DIRECTIONS_DOWN_LEFT			= 7,
	ARRAY_DIRECTIONS_ALL				= 8
};

struct DistantJumpPoints
{
	uint8 blockedDirectionBitfield;	// highest bit [DownLeft, Left, UpLeft, Up, UpRight, Right, DownRight, Down] lowest bit
	short jumpDistance[8];
};

class BattleMap;

class PrecomputeMap
{
public:
	PrecomputeMap(BattleMap* map);
	~PrecomputeMap();

	void calculateMap();
	DistantJumpPoints** getPrecomputedMap() const { return m_distantJumpPointMap; }
	void invalidate() { m_isPrecomputed = false; }
	bool isPrecomputed() const { return m_isPrecomputed; }

	int32 getWidth() const { return m_width; }
	int32 getHeight() const { return m_height; }

protected:
	bool m_isPrecomputed;
	BattleMap* m_map;
	int32 m_width;
	int32 m_height;
	uint8** m_jumpPointMap;
	DistantJumpPoints** m_distantJumpPointMap;

	template <typename T> void initArray(T**& t, int32 width, int32 height);
	template <typename T> void destroyArray(T**& t);

	void calculateJumpPointMap();
	void calculateDistantJumpPointMap();
	bool isJumpPoint(int32 r, int32 c, int32 rowDir, int32 colDir);
	bool isEmpty(int32 r, int32 c);
	bool isWall(int32 r, int32 c);

	enum BitfieldDirections
	{
		MOVING_DOWN		= 1 << 0,
		MOVING_RIGHT	= 1 << 1,
		MOVING_UP		= 1 << 2,
		MOVING_LEFT		= 1 << 3,
	};
};

#endif // __PRECOMPUTE_MAP_H__
