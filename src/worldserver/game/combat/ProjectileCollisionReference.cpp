#include "ProjectileCollisionReference.h"

#include "game/behaviors/WorldObject.h"


ProjectileCollisionReference::ProjectileCollisionReference(AttackableObject* target, Projectile* proj, float precision) :
	m_precision(precision)
{
	this->link(target, proj);
}

ProjectileCollisionReference::~ProjectileCollisionReference()
{
	this->unlink();
}

void ProjectileCollisionReference::buildLink()
{
	this->getTarget()->getProjectileCollisionRefManager()->insertFirst(this);
	this->getTarget()->getProjectileCollisionRefManager()->incSize();
}

void ProjectileCollisionReference::destroyLink()
{
	this->getTarget()->getProjectileCollisionRefManager()->decSize();
}

