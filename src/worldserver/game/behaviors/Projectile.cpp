#include "Projectile.h"

#include "logging/Log.h"
#include "game/utils/MathTools.h"
#include "game/behaviors/Unit.h"
#include "game/behaviors/ItemBox.h"
#include "game/server/protocol/pb/LaunchResult.pb.h"
#include "game/server/WorldSession.h"
#include "game/grids/ObjectSearcher.h"
#include "game/world/ObjectAccessor.h"
#include "Player.h"
#include "ObjectShapes.h"
#include "UnitHelper.h"

#define INACTIVATION_DELAY							500 // 失活状态延迟时间。单位：毫秒

Projectile::Projectile() :
	m_state(PROJECTILE_STATE_ACTIVE),
	m_moveSpline(new ProjectileMoveSpline(this))
{
    m_type |= TypeMask::TYPEMASK_PROJECTILE;
    m_typeId = TypeID::TYPEID_PROJECTILE;
}

Projectile::~Projectile()
{
	this->removeAllCollidedObjects();
	if (m_moveSpline)
	{
		delete m_moveSpline;
		m_moveSpline = nullptr;
	}
}

bool Projectile::canDetect(Unit* target) const
{
	if ((!this->isActive() && !this->isCollided()) || !m_launcher.isValid())
		return false;

	Unit* launcher = m_launcher.getSource();

	if (!launcher->isAlive() || !target->isInWorld() || !target->isVisible() || !target->isAlive())
		return false;

	if (target == launcher)
		return false;

	// 如果发射者没有开启GM模式，则抛射体无法探测到其他GM玩家
	if (target->isType(TYPEMASK_PLAYER))
	{
		DataPlayer* dPlayer = target->asPlayer()->getData();
		if (dPlayer->isGM() && !(launcher->asPlayer() && launcher->asPlayer()->getData()->isGM()))
			return false;
	}

	// 如果发射者开启GM模式，则抛射体只能探测到GM玩家，不能探测到其他目标
	if (launcher->isType(TYPEMASK_PLAYER))
	{
		DataPlayer* dPlayer = launcher->asPlayer()->getData();
		if (dPlayer->isGM() && !(target->asPlayer() && target->asPlayer()->getData()->isGM()))
			return false;
	}

	if (target->isType(TYPEMASK_UNIT))
	{
		if (!launcher->hasItemEffectType(ITEM_EFFECT_DISCOVER_CONCEALED_UNIT))
		{
			// 抛射体无法探测到已隐蔽并且不在发现范围内的目标
			DataUnit* dUnit = target->asUnit()->getData();
			if (dUnit->getConcealmentState() == CONCEALMENT_STATE_CONCEALED && !launcher->isWithinDist(target, DISCOVER_CONCEALED_UNIT_DISTANCE))
				return false;
		}
	}

	return true;
}

bool Projectile::canDetect(ItemBox* target) const
{
	if ((!this->isActive() && !this->isCollided()) || !m_launcher.isValid())
		return false;

	Unit* launcher = m_launcher.getSource();

	if (!launcher->isAlive() || !target->isInWorld() || !target->isVisible())
		return false;

	// 如果发射者开启GM模式，则抛射体不能探测到目标
	if (launcher->isType(TYPEMASK_PLAYER))
	{
		DataPlayer* dPlayer = launcher->asPlayer()->getData();
		if (dPlayer->isGM())
			return false;
	}

	return true;
}

void Projectile::testCollision(AttackableObject* target)
{
	float precision;
	if (this->isCollideWith(target, &precision))
	{
		if (!this->isCollided())
		{
			this->sendLaunchResult(LAUNCHSTATUS_HIT_TARGET, target);
			this->getData()->setStatus(LAUNCHSTATUS_HIT_TARGET);
			this->setProjectileState(PROJECTILE_STATE_COLLIDED);
		}
		this->addCollidedObject(target, precision);
	}
}

bool Projectile::isInProjectedPath(AttackableObject* target, float* precision) const
{
	float projRadius = this->getData()->getObjectRadiusInMap() * this->getData()->getScale();
	float radius = target->getData()->getObjectRadiusInMap() + projRadius;

	if (precision)
		*precision = 0;

	MapData* mapData = this->getMap()->getMapData();
	Point endPos = this->getData()->getTrajectory().startPosition + this->getData()->getTrajectory().endPosition;
	Point endMapPos = mapData->openGLToMapPos(endPos);
	Point targetMapPos = mapData->openGLToMapPos(target->getData()->getPosition());
	Point launcherMapPos = mapData->openGLToMapPos(this->getData()->getLauncherOrigin());
	float dist = MathTools::minDistanceFromPointToSegment(launcherMapPos, endMapPos, targetMapPos);
	if (dist <= radius)
	{
		if (precision)
			*precision = 1.0f - dist / radius;

		return true;
	}

	return false;
}

void Projectile::setProjectileState(ProjectileState state)
{
	m_state = state;
	switch (state)
	{
	case PROJECTILE_STATE_INACTIVE:
		m_launcher.unlink();
		m_projHostileRefManager.clearReferences();
		m_observerRefManager.clearReferences();
		this->removeAllCollidedObjects();
		break;
	case PROJECTILE_STATE_ACTIVE:
		NS_ASSERT(m_moveSpline->isFinished());
		m_moveSpline->move();
		break;
	default:
		break;
	}
}

void Projectile::inactivate()
{
	this->setProjectileState(PROJECTILE_STATE_INACTIVE);
	this->getMap()->addObjectToRemoveList(this);
}

void Projectile::sendLaunchResult(int32 status, AttackableObject* target)
{
	LaunchResult message;
	if (m_launcher.isValid())
		message.set_launcher(m_launcher.getSource()->getData()->getGuid().getRawValue());
	message.set_projectile(this->getData()->getGuid().getRawValue());
	Point collPos = this->getData()->getPosition();
	if (target)
	{
		message.set_target(target->getData()->getGuid().getRawValue());
		// 修正碰撞位置
		if (target->isType(TYPEMASK_UNIT))
		{
			Rect collbox = target->getBoundingBox();
			Point startPos = m_moveSpline->getPrevPosition();
			Point endPos = this->getData()->getPosition();

			Point center;
			MathTools::findIntersectionTwoLines(startPos, endPos, Point(collbox.getMidX(), collbox.getMinY()), Point(collbox.getMidX(), collbox.getMaxY()), center);
			center.y = std::min(collbox.getMaxY(), std::max(collbox.getMinY(), center.y));
			center.x = collbox.getMidX();
			collPos = center;
		}
	}
	message.set_position_x(collPos.x);
	message.set_position_y(collPos.y);
	message.set_status(status);

	// 发送给周围客户端中包含这个抛射体的玩家
	PlayerClientExistsObjectFilter filter(this);
	std::list<Player*> result;
	ObjectSearcher<Player, PlayerClientExistsObjectFilter> searcher(filter, result);

	this->visitNearbyObjectsInMaxVisibleRange(searcher);

	for (auto it = result.begin(); it != result.end(); ++it)
	{
		Player* target = *it;
		if (WorldSession* session = target->getSession())
		{
			WorldPacket packet(SMSG_LAUNCH_RESULT);
			session->packAndSend(std::move(packet), message);
		}
	}
}

void Projectile::addCollidedObject(AttackableObject* object, float precision)
{
	auto it = m_collidedObjects.find(object->getData()->getGuid());
	if (it == m_collidedObjects.end())
	{
		ProjectileCollisionReference* ref = new ProjectileCollisionReference(object, this, precision);
		auto it = m_collidedObjects.emplace(object->getData()->getGuid(), ref);
		NS_ASSERT(it.second);
	}
}

void Projectile::updateCollidedObjects()
{
	for (auto it = m_collidedObjects.begin(); it != m_collidedObjects.end(); ++it)
	{
		ProjectileCollisionReference* ref = (*it).second;
		if (ref->isValid())
		{
			AttackableObject* object = ref->getTarget();
			object->enterCollision(this, ref->getPrecision());
		}
	}
}

void Projectile::removeAllCollidedObjects()
{
	for (auto it = m_collidedObjects.begin(); it != m_collidedObjects.end();)
	{
		ProjectileCollisionReference* ref = (*it).second;
		ref->unlink();
		it = m_collidedObjects.erase(it);

		delete ref;
	}
}

void Projectile::update(NSTime diff)
{
	WorldObject::update(diff);

	if (!this->isInWorld())
		return;

	switch (m_state)
	{
	case PROJECTILE_STATE_COLLIDED:
		this->updateCollidedObjects();
		this->setProjectileState(PROJECTILE_STATE_INACTIVATE);
		break;
	case PROJECTILE_STATE_INACTIVATE:
		m_moveSpline->stop();
		m_inactivationTimer.reset();
		this->setProjectileState(PROJECTILE_STATE_INACTIVATING);
		break;
	case PROJECTILE_STATE_INACTIVATING:
		m_inactivationTimer.update(diff);
		if (m_inactivationTimer.passed())
		{
			m_inactivationTimer.reset();
			this->inactivate();
		}
		break;
	case PROJECTILE_STATE_ACTIVE:
		m_moveSpline->update(diff);
		if (m_moveSpline->isFinished())
		{
			this->getData()->setStatus(LAUNCHSTATUS_FINISHED);
			this->setProjectileState(PROJECTILE_STATE_INACTIVATE);
		}
		break;
	default:
		break;
	}
}

void Projectile::updatePosition(Point const& newPosition)
{
	bool relocated = (this->getData()->getPosition().x != newPosition.x || this->getData()->getPosition().y != newPosition.y);
	if (relocated)
	{
		this->getMap()->projectileRelocation(this, newPosition);

		// 抛射体在地面的位置
		MapData const* mapData = this->getMap()->getMapData();
		Point shadowPosition = this->calcShadowPosition();
		TileCoord groundCoord(mapData->getMapSize(), shadowPosition);

		if (mapData->isCollidable(groundCoord) || !mapData->isValidTileCoord(groundCoord))
		{
			this->getData()->setStatus(LAUNCHSTATUS_HIT_BUILDING);
			this->sendLaunchResult(LAUNCHSTATUS_HIT_BUILDING);
			this->setProjectileState(PROJECTILE_STATE_INACTIVATE);
		}
	}
}

Point Projectile::calcShadowPosition()
{
	Point launcherOrigin = this->getData()->getLauncherOrigin();
	Point startPos = this->getData()->getTrajectory().startPosition;
	Point endPos = this->getData()->getTrajectory().startPosition + this->getData()->getTrajectory().endPosition;

	// 计算阴影起始位置
	Point shadowStartPos;
	shadowStartPos.x = startPos.x;
	shadowStartPos.y = startPos.y - this->getData()->getLaunchCenter().y;

	// 计算阴影当前位置
	float scale = std::max(0.f, std::min(1.f, m_moveSpline->getElapsed() / (float)m_moveSpline->getDuration()));
	float dx = (endPos.x - shadowStartPos.x) * scale;
	float dy = (endPos.y - shadowStartPos.y) * scale;
	Point shadowCurrPos(shadowStartPos.x + dx, shadowStartPos.y + dy);

	return shadowCurrPos;
}

bool Projectile::isCollideWith(AttackableObject* target, float* precision)
{
	float projWidth = this->getData()->getObjectSize().width * this->getData()->getScale();
	float projHeight = this->getData()->getObjectSize().height * this->getData()->getScale();

	Rect collbox = target->getBoundingBox();
	collbox.origin.x -= projWidth / 2;
	collbox.origin.y -= projHeight / 2;
	collbox.size.width += projWidth;
	collbox.size.height += projHeight;
	if (collbox.intersectsLine(m_moveSpline->getPrevPosition(), this->getData()->getPosition()))
	{
		if (this->isInProjectedPath(target, precision))
			return true;
	}

	if (precision)
		*precision = 0;

	return false;
}



void Projectile::addToWorld()
{
	if (this->isInWorld())
		return;

	WorldObject::addToWorld();

	ObjectAccessor::addObject(this);
}

void Projectile::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	m_moveSpline->stop();
	this->invisibleForNearbyPlayers();

	ObjectAccessor::removeObject(this);

	WorldObject::removeFromWorld();
}


void Projectile::cleanupBeforeDelete()
{
	if (this->isInWorld())
		this->removeFromWorld();

	m_launcher.unlink();
	m_projHostileRefManager.clearReferences();
	this->removeAllCollidedObjects();

	WorldObject::cleanupBeforeDelete();
}

bool Projectile::loadData(SimpleProjectile const& simpleProjectile, BattleMap* map)
{
	DataProjectile* data = new DataProjectile();
	data->setGuid(ObjectGuid(GUIDTYPE_PROJECTILE, map->generateProjectileSpawnId()));
	this->setData(data);

	return this->reloadData(simpleProjectile, map);
}

bool Projectile::reloadData(SimpleProjectile const& simpleProjectile, BattleMap* map)
{
	DataProjectile* data = this->getData();
	if (!data)
		return false;

	data->setObjectSize(PROJECTILE_OBJECT_SIZE);
	data->setAnchorPoint(PROJECTILE_ANCHOR_POINT);
	data->setObjectRadiusInMap(PROJECTILE_OBJECT_RADIUS_IN_MAP);

	data->setLauncher(simpleProjectile.launcher);
	data->setLauncherOrigin(simpleProjectile.launcherOrigin);
	data->setAttackRange(simpleProjectile.attackRange);
	data->setLaunchCenter(simpleProjectile.launchCenter);
	data->setLaunchRadiusInMap(simpleProjectile.launchRadiusInMap);

	data->setOrientation(simpleProjectile.orientation);

	// 计算抛物线
	Point landingPos = UnitHelper::computeLandingPosition(simpleProjectile.launcherOrigin, simpleProjectile.attackRange, simpleProjectile.orientation);
	Point launchPos = UnitHelper::computeLaunchPosition(map->getMapData(), simpleProjectile.launcherOrigin, simpleProjectile.launchCenter, simpleProjectile.launchRadiusInMap, landingPos);
	TrajectoryGenerator trajGenerator(TRAJECTORY_TYPE_PROJECTILE, launchPos, landingPos);
	trajGenerator.compute();
	data->setPosition(launchPos);
	BezierCurveConfig const& config = trajGenerator.getBezierCurveConfig();
	data->setTrajectory(config);

	// 计算持续时间
	int32 duration = (int32)(config.length / PROJECTILE_SPEED * 1000);
	data->setDuration(duration);

	data->setProjectileType(simpleProjectile.projectileType);
	data->setDamageBonusRatio(simpleProjectile.damageBonusRatio);
	data->setElapsed(0);
	data->setAttackCounter(simpleProjectile.attackCounter);
	data->setConsumedStamina(simpleProjectile.consumedStamina);
	data->setChargedStamina(simpleProjectile.chargedStamina);
	data->setScale(simpleProjectile.scale);
	data->setAttackInfoCounter(simpleProjectile.attackInfoCounter);
	data->setStatus(LAUNCHSTATUS_NONE);

	m_inactivationTimer.setDuration(INACTIVATION_DELAY);
	this->setProjectileState(PROJECTILE_STATE_ACTIVE);

	return true;
}
