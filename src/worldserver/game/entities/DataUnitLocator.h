#ifndef __DATA_UNIT_LOCATOR_H__
#define __DATA_UNIT_LOCATOR_H__

#include "DataLocatorObject.h"
#include "LocationInfo.h"

class DataUnitLocator: public DataLocatorObject
{
public:
	DataUnitLocator();
	~DataUnitLocator();

	void setGuid(ObjectGuid const& guid) override;

	uint32 getDisplayId() const { return m_displayId; }
	void setDisplayId(uint32 id) { m_displayId = id; }

	Point const& getPosition() const override { return m_locationInfo.position; }
	void setPosition(Point const& position) override { m_locationInfo.position = position; }

	LocationInfo const& getLocationInfo() const { return m_locationInfo; }

	void setAlive(bool isAlive);
	bool isAlive() const { return m_isAlive; }

	void setMoveSpeed(int32 moveSpeed);
	int32 getMoveSpeed() const { return m_moveSpeed; }

	void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const;

private:
	uint32 m_displayId;
	LocationInfo m_locationInfo;
	bool m_isAlive;
	int32 m_moveSpeed;
};

#endif // __DATA_UNIT_LOCATOR_H__