#include "DataWorldObject.h"


#include "game/behaviors/Object.h"

DataWorldObject::DataWorldObject():
	m_mapData(nullptr),
	m_anchorPoint(Point::ZERO),
	m_objectSize(Size::ZERO),
	m_objectRadiusInMap(0)
{
	m_type |= DataTypeMask::DATA_TYPEMASK_WORLDOBJECT;
	m_typeId = DataTypeID::DATA_TYPEID_WORLDOBJECT;
}


DataWorldObject::~DataWorldObject()
{
	m_mapData = nullptr;
}


void DataWorldObject::setMapData(MapData* mapData)
{
	m_mapData = mapData;
}


