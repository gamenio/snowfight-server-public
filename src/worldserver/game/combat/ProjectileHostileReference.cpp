#include "ProjectileHostileReference.h"

#include "game/behaviors/Projectile.h"
#include "ProjectileThreatManager.h"

ProjectileHostileReference::ProjectileHostileReference(Projectile* proj, ProjectileThreatManager* threatManager) :
	m_guid(proj->getData()->getGuid())
{
	this->link(proj, threatManager);
}

ProjectileHostileReference::~ProjectileHostileReference()
{
}

void ProjectileHostileReference::buildLink()
{
	this->getTarget()->getProjectileHostileRefManager()->insertFirst(this);
	this->getTarget()->getProjectileHostileRefManager()->incSize();
}

void ProjectileHostileReference::destroyLink()
{
	this->getTarget()->getProjectileHostileRefManager()->decSize();
	this->getSource()->setDirty(true);
}