#ifndef __DATA_BASIC_H__
#define __DATA_BASIC_H__

#include "Common.h"
#include "updates/ObjectUpdateFields.h"
#include "updates/FieldUpdateMask.h"
#include "updates/UpdateObject.h"
#include "updates/Updatable.h"
#include "DataTypes.h"
#include "ObjectGuid.h"


class Object;
class Player;
class DataPlayer;
class DataRobot;
class DataUnit;
class DataItemBox;
class DataItem;
class DataCarriedItem;
class DataProjectile;
class DataLocatorObject;
class DataUnitLocator;

class DataObserver
{
public:
	virtual void addToObjectUpdateIfNeeded() = 0;
};


class DataBasic: public Updatable
{
public:
	DataBasic();
	virtual ~DataBasic();

	virtual void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const = 0;

	void setDataObserver(DataObserver* observer) { m_dataObserver = observer; }
	void removeDataObserver() { m_dataObserver = nullptr; }

	// Notify that the object data has been updated
	virtual void notifyDataUpdated();

	// Update data for the specified player
	virtual void updateDataForPlayer(UpdateType updateType, Player* player) { }

	virtual void setGuid(ObjectGuid const& guid) { m_guid = guid; }
	ObjectGuid const& getGuid() const { return m_guid; }

	bool isType(uint16 mask) const { return (m_type & mask) != 0; }
	DataTypeID getTypeId() const { return m_typeId; }
	void setTypeId(DataTypeID typeId) { m_typeId = typeId; }

	DataPlayer* asDataPlayer() { if (isType(DATA_TYPEMASK_PLAYER)) return reinterpret_cast<DataPlayer*>(this); else return nullptr; }
	DataRobot* asDataRobot() { if (isType(DATA_TYPEMASK_ROBOT)) return reinterpret_cast<DataRobot*>(this); else return nullptr; }
	DataUnit* asDataUnit() { if (isType(DATA_TYPEMASK_UNIT)) return reinterpret_cast<DataUnit*>(this); else return nullptr; }
	DataItemBox* asDataItemBox() { if (isType(DATA_TYPEMASK_ITEMBOX)) return reinterpret_cast<DataItemBox*>(this); else return nullptr; }
	DataItem* asDataItem() { if (isType(DATA_TYPEMASK_ITEM)) return reinterpret_cast<DataItem*>(this); else return nullptr; }
	DataCarriedItem* asDataCarriedItem() { if (isType(DATA_TYPEMASK_CARRIED_ITEM)) return reinterpret_cast<DataCarriedItem*>(this); else return nullptr; }
	DataProjectile* asDataProjectile() { if (isType(DATA_TYPEMASK_PROJECTILE)) return reinterpret_cast<DataProjectile*>(this); else return nullptr; }
	DataLocatorObject* asObjectLocator() { if (isType(DATA_TYPEMASK_LOCATOR_OBJECT)) return reinterpret_cast<DataLocatorObject*>(this); else return nullptr; }
	DataUnitLocator* asUnitLocator() { if (isType(DATA_TYPEMASK_UNIT_LOCATOR)) return reinterpret_cast<DataUnitLocator*>(this); else return nullptr; }

protected:
	DataTypeID m_typeId;
	uint16 m_type;
	ObjectGuid m_guid;

	DataObserver* m_dataObserver;
};

#endif // __DATA_BASIC_H__