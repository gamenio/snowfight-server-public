#include "ProjectileThreatManager.h"

#include "game/behaviors/Robot.h"
#include "game/behaviors/Projectile.h"

ProjectileThreatManager::ProjectileThreatManager(Robot* owner):
	m_owner(owner),
	m_isDirty(false)
{
}


ProjectileThreatManager::~ProjectileThreatManager()
{
	m_owner = nullptr;
	this->removeAllThreats();
}


Projectile* ProjectileThreatManager::getHostileTarget()
{
	if (ProjectileHostileReference* ref = this->selectNextHostileRef())
		return ref->getTarget();

	return nullptr;
}


void ProjectileThreatManager::addThreat(Projectile* proj)
{
	ProjectileHostileReference* hostileRef = this->getReferenceByTarget(proj);
	if (!hostileRef)
	{
		hostileRef = new ProjectileHostileReference(proj, this);
		m_threatList.push_back(hostileRef);
		auto it = m_threats.emplace(proj->getData()->getGuid(), hostileRef);
		NS_ASSERT(it.second);
	}
}

bool ProjectileThreatManager::isHostileTarget(Projectile* proj) const
{
	return this->getReferenceByTarget(proj) != nullptr;
}

ProjectileHostileReference* ProjectileThreatManager::getReferenceByTarget(Projectile* target) const
{
	auto it = m_threats.find(target->getData()->getGuid());
	if (it != m_threats.end())
		return (*it).second;

	return nullptr;
}

void ProjectileThreatManager::removeAllThreats()
{
	for (auto it = m_threatList.begin(); it != m_threatList.end();)
	{
		ProjectileHostileReference* ref = *it;
		ref->unlink();
		m_threats.erase(ref->getGuid());
		it = m_threatList.erase(it);

		delete ref;
	}
}

void ProjectileThreatManager::update()
{
	if (m_isDirty)
	{
		for (auto it = m_threatList.begin(); it != m_threatList.end();)
		{
			ProjectileHostileReference* ref = *it;
			if (!ref->isValid())
			{
				it = m_threatList.erase(it);
				m_threats.erase(ref->getGuid());
				delete ref;
			}
			else
				++it;
		}
	}

	m_isDirty = false;
}

ProjectileHostileReference* ProjectileThreatManager::selectNextHostileRef()
{
	ProjectileHostileReference* currRef = nullptr;
	for (auto it = m_threatList.begin(); it != m_threatList.end(); ++it)
	{
		ProjectileHostileReference* ref = *it;
		NS_ASSERT(ref->isValid());
		Projectile* proj = ref->getTarget();
		if (m_owner->isPotentialThreat(proj))
		{
			currRef = ref;
			break;
		}

	}
	return currRef;
}

