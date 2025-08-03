#include "MovementInfo.h"

#include "DataUnit.h"


MovementInfo::MovementInfo():
	guid(ObjectGuid::EMPTY),
	counter(0),
	flags(0),
	position(Point::ZERO),
	orientation(0),
	time(0)
{
}

MovementInfo::~MovementInfo()
{
}

MovementInfo::MovementInfo(MovementInfo const& right) :
	Parcel(right)
{
	this->copyFrom(right);
}

MovementInfo& MovementInfo::operator=(MovementInfo const& right)
{
	if (this != &right)
	{
		Parcel::operator=(right);
		this->copyFrom(right);
	}

	return *this;
}

void MovementInfo::Clear()
{
	guid = ObjectGuid::EMPTY;
	counter = 0;
	flags = 0;
	position = Point::ZERO;
	orientation = 0;
	time = 0;

	Parcel::Clear();
}

size_t MovementInfo::sizeInBytes() const
{
	size_t totalSize = 0;
	totalSize += Parcel::uint32Size(guid.getRawValue());
	totalSize += Parcel::uint32Size(counter);
	totalSize += Parcel::uint32Size(flags);
	totalSize += kFloatSize * 2; // position
	totalSize += kFloatSize; // orientation
	totalSize += Parcel::int32Size(time);

	return totalSize;
}


bool MovementInfo::readFromStream(DataInputStream* input)
{
	uint32 _guid;
	CHECK_READ(Parcel::readUInt32(input, &_guid));
	guid.setRawValue(_guid);

	CHECK_READ(Parcel::readUInt32(input, &counter));
	CHECK_READ(Parcel::readUInt32(input, &flags));

	CHECK_READ(Parcel::readFloat(input, &position.x));
	CHECK_READ(Parcel::readFloat(input, &position.y));

	CHECK_READ(Parcel::readFloat(input, &orientation));
	CHECK_READ(Parcel::readInt32(input, &time));

	return true;
}


void MovementInfo::writeToStream(DataOutputStream* output) const
{
	Parcel::writeUInt32(guid.getRawValue(), output);
	Parcel::writeUInt32(counter, output);
	Parcel::writeUInt32(flags, output);

	Parcel::writeFloat(position.x, output);
	Parcel::writeFloat(position.y, output);

	Parcel::writeFloat(orientation, output);
	Parcel::writeInt32(time, output);
}

void MovementInfo::copyFrom(MovementInfo const& right)
{
	guid = right.guid;
	counter = right.counter;
	flags = right.flags;
	position = right.position;
	orientation = right.orientation;
	time = right.time;
}

std::string MovementInfo::description() const
{
	std::ostringstream ss;

	ss << "MOVEMENT INFO"
		<< " guid: " << guid.toString()
		<< " counter: " << counter
		<< " orient: " << orientation
		<< " pos: [" << position.x << ", " << position.y << "]"
		<< " time: " << time;

	ss << " movementflags: ";

	if (flags == MOVEMENT_FLAG_NONE)
		ss << "MOVEMENT_FLAG_NONE";
	else
	{
		std::string mfstr;
		if ((flags & MOVEMENT_FLAG_WALKING) != 0)
			mfstr.append("MOVEMENT_FLAG_WALKING|");
		if ((flags & MOVEMENT_FLAG_HANDUP) != 0)
			mfstr.append("MOVEMENT_FLAG_HANDUP|");

		if (!mfstr.empty())
			ss << mfstr.substr(0, mfstr.size() - 1);
	}

	return ss.str();
}

