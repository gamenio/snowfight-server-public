#ifndef __PROJECTILE_HOSTILE_REFMANAGER_H__
#define __PROJECTILE_HOSTILE_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"
#include "ProjectileHostileReference.h"

class Projectile;
class ProjectileThreatManager;

class ProjectileHostileRefManager: public RefManager<Projectile, ProjectileThreatManager>
{
public:
	ProjectileHostileReference* getFirst() { return ((ProjectileHostileReference*)RefManager<Projectile, ProjectileThreatManager>::getFirst()); }
};

#endif // __PROJECTILE_HOSTILE_REFMANAGER_H__
