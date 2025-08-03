/*
 * PrecomputeMap.cpp
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

#include "PrecomputeMap.h"

#include "logging/Log.h"
#include "utilities/TimeUtil.h"
#include "utilities/Random.h"
#include "game/maps/BattleMap.h"
#include "JPSPlus.h"

PrecomputeMap::PrecomputeMap(BattleMap* map):
	m_isPrecomputed(false),
	m_map(map),
	m_width(0),
	m_height(0),
	m_jumpPointMap(nullptr),
	m_distantJumpPointMap(nullptr)
{
	m_width = (int32)m_map->getMapData()->getMapSize().width;
	m_height = (int32)m_map->getMapData()->getMapSize().height;

	initArray(m_jumpPointMap, m_width, m_height);
	initArray(m_distantJumpPointMap, m_width, m_height);
}

PrecomputeMap::~PrecomputeMap()
{
	destroyArray(m_jumpPointMap);
	destroyArray(m_distantJumpPointMap);
	
	m_map = nullptr;
}

void PrecomputeMap::calculateMap()
{
	if (m_isPrecomputed)
		return;

	NS_LOG_DEBUG("movement.pathfinding", "Calculating distant jump points");

	calculateJumpPointMap();
	calculateDistantJumpPointMap();

	m_isPrecomputed = true;
}

template <typename T>
void PrecomputeMap::initArray(T**& t, int32 width, int32 height)
{
	t = new T*[height];
	for (int32 i = 0; i < height; ++i)
	{
		t[i] = new T[width];
		memset(t[i], 0, sizeof(T)*width);
	}
}

template <typename T>
void PrecomputeMap::destroyArray(T**& t)
{
	for (int32 i = 0; i < m_height; ++i)
		delete[] t[i];
	delete[] t;

	t = 0;
}

void PrecomputeMap::calculateJumpPointMap()
{
	auto currTime = getHighResolutionTimeMillis();

	for (int32 r = 0; r < m_height; ++r)
	{
		for (int32 c = 0; c < m_width; ++c)
		{
			m_jumpPointMap[r][c] = 0;

			if (this->isEmpty(r, c))
			{
				if (isJumpPoint(r, c, 1, 0))
				{
					m_jumpPointMap[r][c] |= MOVING_DOWN;
				}
				if (isJumpPoint(r, c, -1, 0))
				{
					m_jumpPointMap[r][c] |= MOVING_UP;
				}
				if (isJumpPoint(r, c, 0, 1))
				{
					m_jumpPointMap[r][c] |= MOVING_RIGHT;
				}
				if (isJumpPoint(r, c, 0, -1))
				{
					m_jumpPointMap[r][c] |= MOVING_LEFT;
				}
			}
		}
	}

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("movement.pathfinding", "calculateJumpPointMap() time in %f ms", elapsed);
}

bool PrecomputeMap::isJumpPoint(int32 r, int32 c, int32 rowDir, int32 colDir)
{
	return
		isEmpty(r - rowDir, c - colDir) &&						// Parent not a wall (not necessary)
		((isEmpty(r + colDir, c + rowDir) &&					// 1st forced neighbor
		isWall(r - rowDir + colDir, c - colDir + rowDir)) ||	// 1st forced neighbor (continued)
		((isEmpty(r - colDir, c - rowDir) &&					// 2nd forced neighbor
		isWall(r - rowDir - colDir, c - colDir - rowDir))));	// 2nd forced neighbor (continued)
}

inline bool PrecomputeMap::isEmpty(int32 r, int32 c)
{
	if (m_map->getMapData()->isValidTileCoord(c, r))
		return !m_map->getMapData()->isWall(c, r) && !m_map->isTileClosed(c, r);
	else
		return false;
}

inline bool PrecomputeMap::isWall(int32 r, int32 c)
{
	if (m_map->getMapData()->isValidTileCoord(c, r))
		return m_map->getMapData()->isWall(c, r) || m_map->isTileClosed(c, r);
	else
		return true;
}

void PrecomputeMap::calculateDistantJumpPointMap()
{
	auto currTime = getHighResolutionTimeMillis();
	// Calculate distant jump points (Left and Right)
	for (int32 r = 0; r < m_height; ++r)
	{
		{
			int32 countMovingLeft = -1;
			bool jumpPointLastSeen = false;
			for (int32 c = 0; c < m_width; ++c)
			{
				if (isWall(r, c))
				{
					countMovingLeft = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_LEFT] = 0;
					continue;
				}

				countMovingLeft++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_LEFT] = countMovingLeft;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_LEFT] = -countMovingLeft;
				}

				if ((m_jumpPointMap[r][c] & MOVING_LEFT) > 0)
				{
					countMovingLeft = 0;
					jumpPointLastSeen = true;
				}
			}
		}

		{
			int32 countMovingRight = -1;
			bool jumpPointLastSeen = false;
			for (int32 c = m_width - 1; c >= 0; --c)
			{
				if (isWall(r, c))
				{
					countMovingRight = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_RIGHT] = 0;
					continue;
				}

				countMovingRight++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_RIGHT] = countMovingRight;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_RIGHT] = -countMovingRight;
				}

				if ((m_jumpPointMap[r][c] & MOVING_RIGHT) > 0)
				{
					countMovingRight = 0;
					jumpPointLastSeen = true;
				}
			}
		}
	}

	// Calculate distant jump points (Up and Down)
	for (int32 c = 0; c < m_width; ++c)
	{
		{
			int32 countMovingUp = -1;
			bool jumpPointLastSeen = false;
			for (int32 r = 0; r < m_height; ++r)
			{
				if (isWall(r, c))
				{
					countMovingUp = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP] = 0;
					continue;
				}

				countMovingUp++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP] = countMovingUp;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP] = -countMovingUp;
				}

				if ((m_jumpPointMap[r][c] & MOVING_UP) > 0)
				{
					countMovingUp = 0;
					jumpPointLastSeen = true;
				}
			}
		}

		{
			int32 countMovingDown = -1;
			bool jumpPointLastSeen = false;
			for (int32 r = m_height - 1; r >= 0; --r)
			{
				if (isWall(r, c))
				{
					countMovingDown = -1;
					jumpPointLastSeen = false;
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN] = 0;
					continue;
				}

				countMovingDown++;

				if (jumpPointLastSeen)
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN] = countMovingDown;
				}
				else // Wall last seen
				{
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN] = -countMovingDown;
				}

				if ((m_jumpPointMap[r][c] & MOVING_DOWN) > 0)
				{
					countMovingDown = 0;
					jumpPointLastSeen = true;
				}
			}
		}
	}

	// Calculate distant jump points (Diagonally UpLeft and UpRight)
	for (int32 r = 0; r < m_height; ++r)
	{
		for (int32 c = 0; c < m_width; ++c)
		{
			if (isEmpty(r, c))
			{
				if (r == 0 || c == 0 || (isWall(r - 1, c) || isWall(r, c - 1) || isWall(r - 1, c - 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_LEFT] = 0;
				}
				else if (isEmpty(r - 1, c) && isEmpty(r, c - 1) && 
					(m_distantJumpPointMap[r - 1][c - 1].jumpDistance[ARRAY_DIRECTIONS_UP] > 0 ||
					 m_distantJumpPointMap[r - 1][c - 1].jumpDistance[ARRAY_DIRECTIONS_LEFT] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_LEFT] = 1;
				}
				else
				{
					// Increment from last
					int32 jumpDistance = m_distantJumpPointMap[r - 1][c - 1].jumpDistance[ARRAY_DIRECTIONS_UP_LEFT];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_LEFT] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_LEFT] = -1 + jumpDistance;
					}
				}


				if (r == 0 || c == m_width - 1 || (isWall(r - 1, c) || isWall(r, c + 1) || isWall(r - 1, c + 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT] = 0;
				}
				else if (isEmpty(r - 1, c) && isEmpty(r, c + 1) &&
					(m_distantJumpPointMap[r - 1][c + 1].jumpDistance[ARRAY_DIRECTIONS_UP] > 0 ||
					 m_distantJumpPointMap[r - 1][c + 1].jumpDistance[ARRAY_DIRECTIONS_RIGHT] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT] = 1;
				}
				else
				{
					// Increment from last
					int32 jumpDistance = m_distantJumpPointMap[r - 1][c + 1].jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT] = -1 + jumpDistance;
					}
				}
			}
			else
			{
				m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_LEFT] = 0;
				m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_UP_RIGHT] = 0;
			}
		}
	}

	// Calculate distant jump points (Diagonally DownLeft and DownRight)
	for (int32 r = m_height - 1; r >= 0; --r)
	{
		for (int32 c = 0; c < m_width; ++c)
		{
			if (isEmpty(r, c))
			{
				if (r == m_height - 1 || c == 0 ||
					(isWall(r + 1, c) || isWall(r, c - 1) || isWall(r + 1, c - 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT] = 0;
				}
				else if (isEmpty(r + 1, c) && isEmpty(r, c - 1) &&
					(m_distantJumpPointMap[r + 1][c - 1].jumpDistance[ARRAY_DIRECTIONS_DOWN] > 0 ||
					 m_distantJumpPointMap[r + 1][c - 1].jumpDistance[ARRAY_DIRECTIONS_LEFT] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT] = 1;
				}
				else
				{
					// Increment from last
					int32 jumpDistance = m_distantJumpPointMap[r + 1][c - 1].jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT] = -1 + jumpDistance;
					}
				}


				if (r == m_height - 1 || c == m_width - 1 || (isWall(r + 1, c) || isWall(r, c + 1) || isWall(r + 1, c + 1)))
				{
					// Wall one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT] = 0;
				}
				else if (isEmpty(r + 1, c) && isEmpty(r, c + 1) &&
					(m_distantJumpPointMap[r + 1][c + 1].jumpDistance[ARRAY_DIRECTIONS_DOWN] > 0 ||
					 m_distantJumpPointMap[r + 1][c + 1].jumpDistance[ARRAY_DIRECTIONS_RIGHT] > 0))
				{
					// Diagonal one away
					m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT] = 1;
				}
				else
				{
					// Increment from last
					int32 jumpDistance = m_distantJumpPointMap[r + 1][c + 1].jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT];

					if (jumpDistance > 0)
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT] = 1 + jumpDistance;
					}
					else //if( jumpDistance <= 0 )
					{
						m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT] = -1 + jumpDistance;
					}
				}
			}
			else
			{
				m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_LEFT] = 0;
				m_distantJumpPointMap[r][c].jumpDistance[ARRAY_DIRECTIONS_DOWN_RIGHT] = 0;
			}
		}
	}

	// Fabricate wall bitfield for each node
	for (int32 r = 0; r < m_height; r++)
	{
		for (int32 c = 0; c < m_width; c++)
		{
			DistantJumpPoints* map = &m_distantJumpPointMap[r][c];
			map->blockedDirectionBitfield = 0;

			if (isWall(r, c))
			{
				continue;
			}

			for (int32 i = 0; i < 8; i++)
			{
				// Jump distance of zero is invalid movement and means a wall
				if (map->jumpDistance[i] == 0)
				{
					map->blockedDirectionBitfield |= (1 << i);
				}
			}
		}
	}

	auto elapsed = getHighResolutionTimeMillis() - currTime;
	NS_LOG_DEBUG("movement.pathfinding", "calculateDistantJumpPointMap() time in %f ms", elapsed);
}