#ifndef __MOVEMENTINFO_H__
#define __MOVEMENTINFO_H__

#include "Common.h"
#include "game/server/protocol/Parcel.h"
#include "game/entities/DataTypes.h"
#include "Point.h"
#include "ObjectGuid.h"


class MovementInfo: public Parcel
{
public:
	MovementInfo();
	~MovementInfo();

	MovementInfo(MovementInfo const& right);
	MovementInfo& operator=(MovementInfo const& right);

	std::string GetTypeName() const override { return "MovementInfo"; }
	void Clear() override;

	virtual size_t sizeInBytes() const override;
	bool readFromStream(DataInputStream* input) override;
	virtual void writeToStream(DataOutputStream* output) const override;

	std::string description() const;

	ObjectGuid guid;
	uint32 counter;
	uint32 flags;
	Point position;
	float orientation;
	int32 time;

private:
	void copyFrom(MovementInfo const& right);
};

#endif //__MOVEMENTINFO_H__


