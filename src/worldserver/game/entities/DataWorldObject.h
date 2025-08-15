#ifndef __DATA_WORLDOBJECT_H__
#define __DATA_WORLDOBJECT_H__


#include "Common.h"
#include "game/maps/MapData.h"
#include "DataBasic.h"
#include "Size.h"
#include "Point.h"
#include "Rect.h"

struct ObjectDistribution
{
	TileArea spawnArea;
};

class DataWorldObject: public DataBasic
{
public:
	DataWorldObject();
	virtual ~DataWorldObject();

	virtual void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override {}

	void setMapData(MapData* mapData);
	MapData* getMapData() { return m_mapData; }
	MapData const* getMapData() const { return m_mapData; }

	virtual Point const& getPosition() const = 0;
	virtual void setPosition(Point const& position) = 0;
	
	void setAnchorPoint(Point const& anchor) { m_anchorPoint = anchor; }
    Point const& getAnchorPoint() const { return m_anchorPoint; }

	Size getObjectSize() const { return m_objectSize; }
	void setObjectSize(Size const& size) { m_objectSize = size; }

	// The radius of the object in the map
	float getObjectRadiusInMap() const { return m_objectRadiusInMap; };
	void setObjectRadiusInMap(float radius) { m_objectRadiusInMap = radius; }

protected:
	MapData* m_mapData;

	Point m_anchorPoint;
	Size m_objectSize;
	float m_objectRadiusInMap;
};


#endif // __DATA_WORLDOBJECT_H__