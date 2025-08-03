#ifndef __PROJECTILE_H__
#define __PROJECTILE_H__

#include "WorldObject.h"
#include "game/entities/Point.h"
#include "game/entities/Size.h"
#include "game/entities/DataProjectile.h"
#include "game/movement/spline/ProjectileMoveSpline.h"
#include "game/combat/LaunchReference.h"
#include "game/combat/ProjectileCollisionReference.h"
#include "game/combat/ProjectileHostileRefManager.h"

struct SimpleProjectile
{
	SimpleProjectile() :
		launcher(ObjectGuid::EMPTY),
		launchRadiusInMap(0),
		orientation(0),
		projectileType(PROJECTILE_TYPE_NORMAL),
		damageBonusRatio(0),
		attackCounter(0),
		consumedStamina(0),
		chargedStamina(0),
		scale(0),
		attackInfoCounter(0)
	{}

	ObjectGuid launcher;
	Point launcherOrigin;
	float attackRange;
	Point launchCenter;
	float launchRadiusInMap;
	float orientation;
	ProjectileType projectileType;
	float damageBonusRatio;
	uint32 attackCounter;
	int32 consumedStamina;
	int32 chargedStamina;
	float scale;
	uint32 attackInfoCounter;
};

enum ProjectileState
{
	PROJECTILE_STATE_ACTIVE,
	PROJECTILE_STATE_COLLIDED,
	PROJECTILE_STATE_INACTIVATE,
	PROJECTILE_STATE_INACTIVATING,
	PROJECTILE_STATE_INACTIVE,
};

class Projectile : public WorldObject, public GridObject<Projectile>
{
public:
	Projectile();
    virtual ~Projectile();

	void update(NSTime diff) override;
	bool isActive() const { return m_state == PROJECTILE_STATE_ACTIVE; }
	bool isCollided() const { return m_state == PROJECTILE_STATE_COLLIDED; }

	void updatePosition(Point const& newPosition);
	ProjectileMoveSpline* getMoveSpline() const { return m_moveSpline; }
	LaunchReference& getLauncher() { return m_launcher; }
	ProjectileHostileRefManager* getProjectileHostileRefManager() { return &m_projHostileRefManager; }

	void addToWorld() override;
	void removeFromWorld() override;

	void cleanupBeforeDelete() override;

	bool loadData(SimpleProjectile const& simpleProjectile, BattleMap* map);
	bool reloadData(SimpleProjectile const& simpleProjectile, BattleMap* map);
	virtual DataProjectile* getData() const override { return static_cast<DataProjectile*>(m_data); }

	bool canDetect(Unit* target) const;
	bool canDetect(ItemBox* target) const;
	void testCollision(AttackableObject* target);
	bool isInProjectedPath(AttackableObject* target, float* precision = nullptr) const;

private:
	Point calcShadowPosition();
	bool isCollideWith(AttackableObject* target, float* precision);

	void setProjectileState(ProjectileState state);
	void inactivate();

	void sendLaunchResult(int32 status, AttackableObject* target = nullptr);

	void addCollidedObject(AttackableObject* object, float precision);
	void updateCollidedObjects();
	void removeAllCollidedObjects();

	ProjectileState m_state;
	DelayTimer m_inactivationTimer;

	LaunchReference m_launcher;
	std::unordered_map<ObjectGuid, ProjectileCollisionReference*> m_collidedObjects;
	ProjectileHostileRefManager m_projHostileRefManager;
	ProjectileMoveSpline* m_moveSpline;
};



#endif // __PROJECTILE_H__
