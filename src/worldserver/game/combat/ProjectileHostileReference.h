#ifndef __PROJECTILE_HOSTILE_REFERENCE_H__
#define __PROJECTILE_HOSTILE_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"
#include "game/entities/ObjectGuid.h"

class Projectile;
class ProjectileThreatManager;

class ProjectileHostileReference: public Reference<Projectile, ProjectileThreatManager>
{
public:
	ProjectileHostileReference(Projectile* proj, ProjectileThreatManager* threatManager);
	~ProjectileHostileReference();

	ProjectileHostileReference* next() { return ((ProjectileHostileReference*)Reference<Projectile, ProjectileThreatManager>::next()); }
	ObjectGuid const& getGuid() const { return m_guid; }

protected:
	void buildLink() override;
	void destroyLink() override;

private:
	ObjectGuid m_guid;
};

#endif // __PROJECTILE_HOSTILE_REFERENCE_H__