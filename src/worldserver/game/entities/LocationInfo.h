#ifndef __LOCATION_INFO_H__
#define __LOCATION_INFO_H__

#include "Common.h"
#include "game/server/protocol/Parcel.h"
#include "Point.h"
#include "ObjectGuid.h"


class LocationInfo : public Parcel
{
public:
	LocationInfo();
	~LocationInfo();

	LocationInfo(LocationInfo const& right);
	LocationInfo& operator=(LocationInfo const& right);

	std::string GetTypeName() const override { return "LocationInfo"; }
	void Clear() override;

	virtual size_t sizeInBytes() const override;
	bool readFromStream(DataInputStream* input) override;
	virtual void writeToStream(DataOutputStream* output) const override;

	std::string description() const;

	ObjectGuid guid;
	Point position;
	int32 time;

private:
	void copyFrom(LocationInfo const& right);
};

#endif // __LOCATION_INFO_H__


