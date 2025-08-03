#ifndef __PROJECTILE_COLLISION_REFERENCE_H__
#define __PROJECTILE_COLLISION_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"

class AttackableObject;
class Projectile;

class ProjectileCollisionReference: public Reference<AttackableObject, Projectile>
{
public:
	ProjectileCollisionReference(AttackableObject* target, Projectile* proj, float precision);
	~ProjectileCollisionReference();

	ProjectileCollisionReference* next() { return ((ProjectileCollisionReference*)Reference<AttackableObject, Projectile>::next()); }

	float getPrecision() const { return m_precision; }

protected:
	void buildLink() override;
	void destroyLink() override;

private:
	float m_precision;
};

#endif // __PROJECTILE_COLLISION_REFERENCE_H__

