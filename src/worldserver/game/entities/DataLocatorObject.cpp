#include "DataLocatorObject.h"

DataLocatorObject::DataLocatorObject()
{
	m_type |= DataTypeMask::DATA_TYPEMASK_LOCATOR_OBJECT;
	m_typeId = DataTypeID::DATA_TYPEID_OBJECT_LOCATOR;
}

DataLocatorObject::~DataLocatorObject()
{
}

