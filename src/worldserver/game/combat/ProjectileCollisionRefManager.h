#ifndef __PROJECTILE_COLLISION_REFMANAGER_H__
#define __PROJECTILE_COLLISION_REFMANAGER_H__

#include "game/dynamic/reference/RefManager.h"
#include "ProjectileCollisionReference.h"

class ProjectileCollisionRefManager : public RefManager<AttackableObject, Projectile>
{
public:
	typedef LinkedListHead::Iterator<ProjectileCollisionReference> iterator;

	ProjectileCollisionReference* getFirst() { return (ProjectileCollisionReference*)RefManager::getFirst(); }
	ProjectileCollisionReference* getLast() { return (ProjectileCollisionReference*)RefManager::getLast(); }

	iterator begin() { return iterator(getFirst()); }
	iterator end() { return iterator(NULL); }
	iterator rbegin() { return iterator(getLast()); }
	iterator rend() { return iterator(NULL); }
};

#endif // __PROJECTILE_COLLISION_REFMANAGER_H__

