#ifndef __DATA_LOCATOR_OBJECT_H__
#define __DATA_LOCATOR_OBJECT_H__

#include "DataBasic.h"
#include "Point.h"

class DataLocatorObject: public DataBasic
{
public:
	DataLocatorObject();
	~DataLocatorObject();

	virtual void writeFields(FieldUpdateMask& updateMask, UpdateType updateType, uint32 updateFlags, DataOutputStream* output) const override {}

	virtual void setPosition(Point const& position) = 0;
	virtual Point const& getPosition() const = 0;
};

#endif // __DATA_LOCATOR_OBJECT_H__