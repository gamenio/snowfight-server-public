#ifndef __PROJECTILE_THREAT_MANAGER_H__
#define __PROJECTILE_THREAT_MANAGER_H__

#include "ProjectileHostileReference.h"

class Robot;
class Projectile;

class ProjectileThreatManager
{
public:
	ProjectileThreatManager(Robot* owner);
	~ProjectileThreatManager();

	Robot* getOwner() const { return m_owner; }

	Projectile* getHostileTarget();

	void addThreat(Projectile* proj);
	bool isHostileTarget(Projectile* proj) const;
	std::list<ProjectileHostileReference*> const& getThreatList() const { return m_threatList; }
	void removeAllThreats();

	void setDirty(bool isDirty) { m_isDirty = isDirty; }
	void update();

private:
	ProjectileHostileReference* selectNextHostileRef();
	ProjectileHostileReference* getReferenceByTarget(Projectile* target) const;

	Robot* m_owner;
	bool m_isDirty;
	std::unordered_map<ObjectGuid, ProjectileHostileReference*> m_threats;
	std::list<ProjectileHostileReference*> m_threatList;
};


#endif // __PROJECTILE_THREAT_MANAGER_H__

