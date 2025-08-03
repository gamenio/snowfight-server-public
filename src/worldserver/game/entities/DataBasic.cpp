#include "DataBasic.h"


DataBasic::DataBasic() :
	m_typeId(DATA_TYPEID_BASIC),
	m_type(DataTypeMask::DATA_TYPEMASK_BASIC),
	m_dataObserver(nullptr)
{
}

DataBasic::~DataBasic()
{
	m_dataObserver = nullptr;
}

void DataBasic::notifyDataUpdated()
{
	if (m_dataObserver)
		m_dataObserver->addToObjectUpdateIfNeeded();
}


