#ifndef __UNIT_LOCATOR_H__
#define __UNIT_LOCATOR_H__

#include "LocatorObject.h"
#include "game/entities/DataUnitLocator.h"

class UnitLocator : public LocatorObject
{
public:
	UnitLocator();
    virtual ~UnitLocator();

	void addToWorld() override;
	void removeFromWorld() override;

	virtual DataUnitLocator* getData() const override { return static_cast<DataUnitLocator*>(m_data); }
};



#endif // __UNIT_LOCATOR_H__
