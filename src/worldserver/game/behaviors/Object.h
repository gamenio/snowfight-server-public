#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "Common.h"
#include "game/entities/DataBasic.h"

enum TypeID
{
	TYPEID_OBJECT			= 0,
	TYPEID_WORLDOBJECT,
	TYPEID_UNIT,
	TYPEID_PLAYER,
	TYPEID_ROBOT,
	TYPEID_ITEMBOX,
	TYPEID_ITEM,
	TYPEID_CARRIED_ITEM,
	TYPEID_PROJECTILE,
	TYPEID_LOCATOR_OBJECT,
	TYPEID_UNIT_LOCATOR,
};

enum TypeMask
{
	TYPEMASK_OBJECT				= 1 << 0,
	TYPEMASK_WORLDOBJECT		= 1 << 1,
	TYPEMASK_UNIT				= 1 << 2,
	TYPEMASK_PLAYER				= 1 << 3,
	TYPEMASK_ROBOT				= 1 << 4,
	TYPEMASK_ITEMBOX			= 1 << 5,
	TYPEMASK_ITEM				= 1 << 6,
	TYPEMASK_CARRIED_ITEM		= 1 << 7,
	TYPEMASK_PROJECTILE			= 1 << 8,
	TYPEMASK_LOCATOR_OBJECT		= 1 << 9,
	TYPEMASK_UNIT_LOCATOR		= 1 << 10,
};

class Unit;
class Player;
class Robot;
class ItemBox;
class Item;
class CarriedItem;
class Projectile;
class UnitLocator;

class Object: public DataObserver
{
public:
	Object();
	virtual ~Object() = 0;

	virtual void setData(DataBasic* data);
    virtual DataBasic* getData() const { return m_data; }
    
	bool isType(uint16 mask) const { return (m_type & mask) != 0; }
	TypeID getTypeID() const { return m_typeId; }
	void setTypeID(TypeID typeId) { m_typeId = typeId; }

	Player* asPlayer() { if (isType(TYPEMASK_PLAYER)) return reinterpret_cast<Player*>(this); else return nullptr; }
	Robot* asRobot() { if (isType(TYPEMASK_ROBOT)) return reinterpret_cast<Robot*>(this); else return nullptr; }
	Unit* asUnit() { if (isType(TYPEMASK_UNIT)) return reinterpret_cast<Unit*>(this); else return nullptr; }
	ItemBox* asItemBox() { if (isType(TYPEMASK_ITEMBOX)) return reinterpret_cast<ItemBox*>(this); else return nullptr; }
	Item* asItem() { if (isType(TYPEMASK_ITEM)) return reinterpret_cast<Item*>(this); else return nullptr; }
	CarriedItem* asCarriedItem() { if (isType(TYPEMASK_CARRIED_ITEM)) return reinterpret_cast<CarriedItem*>(this); else return nullptr; }
	Projectile* asProjectile() { if (isType(TYPEMASK_PROJECTILE)) return reinterpret_cast<Projectile*>(this); else return nullptr; }
	UnitLocator* asUnitLocator() { if (isType(TYPEMASK_UNIT_LOCATOR)) return reinterpret_cast<UnitLocator*>(this); else return nullptr; }

	virtual bool isInWorld() const { return m_isInWorld; }
	virtual void addToWorld();
	virtual void removeFromWorld();

	void sendValuesUpdateToPlayer(Player* player) const;
	void sendCreateUpdateToPlayer(Player* player) const;
	void sendOutOfRangeUpdateToPlayer(Player* player) const;
	void sendDestroyToPlayer(Player* player) const;

	void buildCreateUpdateBlockForPlayer(Player* player, UpdateObject* update) const;
	void buildValuesUpdateBlockForPlayer(Player* player, UpdateObject* update) const;
	void buildOutOfRangeUpdateBlock(UpdateObject* update) const;

	void buildValuesUpdateForPlayerInMap(Player* player, PlayerUpdateMapType& updateMap) const;
	virtual void buildValuesUpdate(PlayerUpdateMapType& updateMap) {}

	// 发送对象更新数据到相关玩家后清理对象，例如：对象更新掩码
	virtual void cleanupAfterUpdate();

	// 将对象增加到更新列表，如果成功则返回true
	virtual bool addToObjectUpdate() = 0;
	virtual void removeFromObjectUpdate() = 0;
	void addToObjectUpdateIfNeeded() override;

protected:
	// 清理更新掩码，如果remove为true并且对象在更新列表中则将其移除
	void clearUpdateMask(bool remove);

	TypeID m_typeId;
	uint16 m_type;

	DataBasic* m_data;
	bool m_objectUpdated;

	bool m_isInWorld;
};

#endif // __OBJECT_H__