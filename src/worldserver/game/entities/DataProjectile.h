#ifndef __DATA_PROJECTILE_H__
#define __DATA_PROJECTILE_H__

#include "Common.h"
#include "DataWorldObject.h"
#include "game/combat/TrajectoryGenerator.h"

#define PROJECTILE_SPEED			355.f			// Projectile speed. Unit: points/second

enum LaunchStatus
{
	LAUNCHSTATUS_NONE				= 0,
	LAUNCHSTATUS_FINISHED,					// Landed at the specified destination
	LAUNCHSTATUS_HIT_TARGET,				// Hit the target
	LAUNCHSTATUS_HIT_BUILDING,				// Collision with a building
	LAUNCHSTATUS_FAILED,					// Launch failed
};

enum ProjectileType
{
	PROJECTILE_TYPE_NORMAL,
	PROJECTILE_TYPE_CHARGED,
	PROJECTILE_TYPE_INTENSIFIED,
};

class DataProjectile : public DataWorldObject
{
public:
	DataProjectile();
	virtual ~DataProjectile();

	ObjectGuid const&  getLauncher() const { return m_launcher; }
	void setLauncher(ObjectGuid const& guid) { m_launcher = guid; }

	Point const& getLauncherOrigin() const { return m_launcherOrigin; }
	void setLauncherOrigin(Point origin) { m_launcherOrigin = origin; }

	void setAttackRange(float attackRange) { m_attackRange = attackRange; }
	float getAttackRange() const { return m_attackRange; }

	Point const& getLaunchCenter() const { return m_launchCenter; }
	void setLaunchCenter(Point const& center) { m_launchCenter = center; }
	float getLaunchRadiusInMap() const { return m_launchRadiusInMap; };
	void setLaunchRadiusInMap(float radius) { m_launchRadiusInMap = radius; }

	Point const& getPosition() const override { return m_position; }
	void setPosition(Point const& position)  override { m_position = position; }

	void setOrientation(float rad) { m_orientation = rad; }
	float getOrientation() const { return m_orientation; }

	void setProjectileType(ProjectileType type) { m_projType = type; }
	ProjectileType getProjectileType() const { return m_projType; }

	// Damage bonus ratio
	void setDamageBonusRatio(float ratio) { m_damageBonusRatio = ratio; }
	float getDamageBonusRatio() const { return m_damageBonusRatio; }

	// Duration. Unit: milliseconds
	int32 getDuration() const { return m_duration; }
	void setDuration(int32 duration) { m_duration = duration; }
	// Time elapsed. Unit: milliseconds
	void setElapsed(int32 elapsed) { m_elapsed = elapsed; }
	int32 getElapsed() const { return m_elapsed; }

	void setAttackCounter(uint32 counter) { m_attackCounter = counter; }
	uint32 getAttackCounter() const { return m_attackCounter; }

	void setConsumedStamina(int32 stamina) { m_consumedStamina = stamina; }
	int32 getConsumedStamina() const { return m_consumedStamina; }

	void setChargedStamina(int32 stamina) { m_chargedStamina = stamina; }
	int32 getChargedStamina() const { return m_chargedStamina; }

	void setScale(float scale) { m_scale = scale; }
	float getScale() const { return m_scale; }

	void setAttackInfoCounter(uint32 counter) { m_attackInfoCounter = counter; }
	uint32 getAttackInfoCounter() const { return m_attackInfoCounter; }

	void setStatus(LaunchStatus status) { m_status = status; }
	LaunchStatus getStatus() const { return m_status; }

	void setTrajectory(BezierCurveConfig const& trajectory) { m_trajectory = trajectory; }
	BezierCurveConfig const& getTrajectory() const { return m_trajectory; }
    
	void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override;
	void updateDataForPlayer(UpdateType updateType, Player* player) override;

private:
	ObjectGuid m_launcher;
	Point m_launcherOrigin;
	float m_attackRange;

	Point m_launchCenter;
	float m_launchRadiusInMap;
	Point m_position;
	float m_orientation;

	ProjectileType m_projType;
	float m_damageBonusRatio;
	int32 m_elapsed;
	int32 m_duration;
	uint32 m_attackCounter;
	int32 m_consumedStamina;
	int32 m_chargedStamina;
	float m_scale;
	uint32 m_attackInfoCounter;
	LaunchStatus m_status;

	BezierCurveConfig m_trajectory;
};



#endif // __DATA_PROJECTILE_H__