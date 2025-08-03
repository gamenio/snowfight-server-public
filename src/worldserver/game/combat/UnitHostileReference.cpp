#include "UnitHostileReference.h"

#include "game/behaviors/Robot.h"

UnitHostileReference::UnitHostileReference(Unit* unit, UnitThreatManager* threatManager):
	m_totalDamage(0),
	m_threat(0)
{
	this->link(unit, threatManager);

	for (int32 i = 0; i < MAX_UNITTHREAT_TYPES; ++i)
		m_threatParts[i] = 0;
}

UnitHostileReference::~UnitHostileReference()
{
}


void UnitHostileReference::updateThreat(UnitThreatType type, float variant)
{
	Unit* victim = this->getTarget();
	Robot* robot = this->getSource()->getOwner();

	float threatPart = 0.f;
	switch (type)
	{
	case UNITTHREAT_DISTANCE:
	case UNITTHREAT_ENEMY_HEALTH:
	case UNITTHREAT_ENEMY_CHARGED_POWER:
		threatPart = robot->calcThreat(victim, type);
		break;
	case UNITTHREAT_RECEIVED_DAMAGE:
		m_totalDamage += variant;
		threatPart = robot->calcThreat(victim, type, m_totalDamage);
		break;
	default:
		break;
	}

	m_threatParts[type] = threatPart;

	float threat = this->calcThreatSum();
	if (threat != m_threat)
	{
		m_threat = threat;
		this->getSource()->setDirty(true);
	}
}

void UnitHostileReference::buildLink()
{
	m_targetGuid = this->getTarget()->getData()->getGuid();
	this->getTarget()->getUnitHostileRefManager()->insertFirst(this);
	this->getTarget()->getUnitHostileRefManager()->incSize();
}

void UnitHostileReference::destroyLink()
{
	this->getTarget()->getUnitHostileRefManager()->decSize();
	this->getSource()->setDirty(true);
}


float UnitHostileReference::calcThreatSum()
{
	float threat = 0.f;
	for (int32 i = 0; i < MAX_UNITTHREAT_TYPES; ++i)
	{
		Robot* robot = this->getSource()->getOwner();
		threat += robot->applyThreatModifier(static_cast<UnitThreatType>(i), m_threatParts[i]);
	}
	return threat;
}
