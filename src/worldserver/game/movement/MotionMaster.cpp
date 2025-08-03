#include "MotionMaster.h"

#include "generators/SmartChaseMovementGenerator.h"
#include "generators/SimpleChaseMovementGenerator.h"
#include "generators/IdleMovementGenerator.h"
#include "generators/EscapeMovementGenerator.h"
#include "generators/PointMovementGenerator.h"
#include "generators/SeekMovementGenerator.h"
#include "generators/ExploreMovementGenerator.h"

MotionMaster::MotionMaster(Robot* owner) :
	m_top(-1),
	m_owner(owner)
{
	for (int32 i = 0; i < MAX_MOTION_SLOT; ++i)
		m_motions[i] = nullptr;
}

MotionMaster::~MotionMaster()
{
	this->clear();
	m_owner = nullptr;
}

MovementGenerator* MotionMaster::pop()
{
	if (empty())
		return nullptr;

	MovementGenerator* curr = m_motions[m_top];

	m_motions[m_top] = nullptr;
	while (!empty() && !top())
		--m_top;

	return curr;
}

MovementGenerator* MotionMaster::top()
{
	if (!empty())
		return m_motions[m_top];
	return nullptr;
}

void MotionMaster::clear()
{
	while (MovementGenerator* curr = pop())
	{
		directDelete(curr);
	}
}

void MotionMaster::remove(MotionSlot slot)
{
	if (empty())
		return;

	if (MovementGenerator* curr = m_motions[slot])
	{
		m_motions[slot] = nullptr;
		this->directDelete(curr);
		while (!top())
			--m_top;
	}
}

void MotionMaster::reset(MotionSlot slot)
{
	MovementGenerator* m = m_motions[slot];
	if (m)
		m->reset();
}

void MotionMaster::updateMotion(NSTime diff)
{
	if (MovementGenerator* curr = this->top())
	{
		if (!curr->update(diff))
		{
			this->pop();
			this->directDelete(curr);
		}
	}
}

void MotionMaster::moveIdle()
{
	MovementGenerator* m = m_motions[MOTION_SLOT_IDLE];
	if (!m)
		this->mutate(new IdleMovementGenerator(m_owner), MOTION_SLOT_IDLE);

	while (size() > MOTION_SLOT_IDLE + 1)
	{
		if (MovementGenerator* curr = pop())
			directDelete(curr);
	}
}

void MotionMaster::moveExplore()
{
	this->mutate(new ExploreMovementGenerator(m_owner), MOTION_SLOT_ACTIVE);
}

void MotionMaster::moveChase(Unit* target, bool isSmart)
{
	if(isSmart)
		this->mutate(new SmartChaseMovementGenerator<Unit>(m_owner, target), MOTION_SLOT_ACTIVE);
	else
		this->mutate(new SimpleChaseMovementGenerator<Unit>(m_owner, target), MOTION_SLOT_ACTIVE);
}

void MotionMaster::moveChase(ItemBox* target, bool isSmart)
{
	if (isSmart)
		this->mutate(new SmartChaseMovementGenerator<ItemBox>(m_owner, target), MOTION_SLOT_ACTIVE);
	else
		this->mutate(new SimpleChaseMovementGenerator<ItemBox>(m_owner, target), MOTION_SLOT_ACTIVE);
}

void MotionMaster::moveEscape(Unit* target)
{
	this->mutate(new EscapeMovementGenerator(m_owner, target), MOTION_SLOT_ACTIVE);
}

void MotionMaster::movePoint(TileCoord const& point, bool isBypassEnemy)
{
	this->mutate(new PointMovementGenerator(m_owner, point, isBypassEnemy), MOTION_SLOT_ACTIVE);
}

void MotionMaster::moveSeek(TileCoord const& hidingSpot)
{
	this->mutate(new SeekMovementGenerator(m_owner, hidingSpot), MOTION_SLOT_ACTIVE);
}

void MotionMaster::mutate(MovementGenerator* m, MotionSlot slot)
{
	if (MovementGenerator* curr = m_motions[slot])
	{
		m_motions[slot] = nullptr;
		this->directDelete(curr);
	}
	else if (m_top < slot)
	{
		if (!this->empty())
		{
			if (m_motions[m_top])
				m_motions[m_top]->finish();
		}

		m_top = slot;
	}

	m_motions[slot] = m;
}

void MotionMaster::directDelete(MovementGenerator* m)
{
	m->finish();
	delete m;
}

