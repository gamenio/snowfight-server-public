#include "ObserverReference.h"

#include "game/behaviors/Robot.h"
#include "TargetSelector.h"
#include "logging/Log.h"

ObserverReference::ObserverReference(WorldObject* object, TargetAction action, TargetSelector* targetManager) :
	m_action(action),
	m_weight(0)
{
	this->link(object, targetManager);
}

ObserverReference::~ObserverReference()
{
}

void ObserverReference::update()
{
	m_weight = 0.f;

	Robot* self = this->getSource()->getOwner();
	WorldObject* target = this->getTarget();

	// The distance between oneself and the target object
	float distance = self->getData()->getPosition().getDistance(target->getData()->getPosition());
	m_weight += std::max(0.f, 1.0f - distance / self->getSightDistance());
}

float ObserverReference::getOrder() const
{
	return m_weight;
}

void ObserverReference::buildLink()
{
	m_targetGuid = this->getTarget()->getData()->getGuid();
	this->getTarget()->getObserverRefManager()->insertFirst(this);
	this->getTarget()->getObserverRefManager()->incSize();
}

void ObserverReference::destroyLink()
{
	this->getTarget()->getObserverRefManager()->decSize();
}
