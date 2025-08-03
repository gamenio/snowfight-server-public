#include "StaminaInfo.h"

#include <iomanip>

#include "DataUnit.h"

StaminaInfo::StaminaInfo():
	guid(ObjectGuid::EMPTY),
	counter(0),
	flags(0),
	stamina(0),
	maxStamina(0),
	staminaRegenRate(0),
	chargeStartStamina(0),
	chargedStamina(0),
	chargeConsumesStamina(0),
	chargeCounter(0),
	time(0),
	attackCounter(0),
	consumedStaminaTotal(0),
	attackInfoCounter(0)
{
}

StaminaInfo::~StaminaInfo()
{
}

StaminaInfo::StaminaInfo(StaminaInfo const& right) :
	Parcel(right)
{
	this->copyFrom(right);
}

StaminaInfo& StaminaInfo::operator=(StaminaInfo const& right)
{
	if (this != &right)
	{
		Parcel::operator=(right);
		this->copyFrom(right);
	}

	return *this;
}

void StaminaInfo::Clear()
{
	guid = ObjectGuid::EMPTY;
	counter = 0;
	flags = 0;
	stamina = 0;
	maxStamina = 0;
	staminaRegenRate = 0;
	chargeStartStamina = 0;
	chargedStamina = 0;
	chargeConsumesStamina = 0;
	chargeCounter = 0;
	time = 0;
	attackCounter = 0;
	consumedStaminaTotal = 0;
	attackInfoCounter = 0;

	Parcel::Clear();
}

size_t StaminaInfo::sizeInBytes() const
{
	size_t totalSize = 0;
	totalSize += Parcel::uint32Size(guid.getRawValue());
	totalSize += Parcel::uint32Size(counter);
	totalSize += Parcel::uint32Size(flags);
	totalSize += Parcel::int32Size(stamina);
	totalSize += Parcel::int32Size(maxStamina);
	totalSize += Parcel::kFloatSize;
	totalSize += Parcel::int32Size(chargeStartStamina);
	totalSize += Parcel::int32Size(chargedStamina);
	totalSize += Parcel::int32Size(chargeConsumesStamina);
	totalSize += Parcel::uint32Size(chargeCounter);
	totalSize += Parcel::int32Size(time);
	totalSize += Parcel::uint32Size(attackCounter);
	totalSize += Parcel::uint32Size(consumedStaminaTotal);
	totalSize += Parcel::uint32Size(attackInfoCounter);

	return totalSize;
}


bool StaminaInfo::readFromStream(DataInputStream* input)
{
	uint32 _guid;
	CHECK_READ(Parcel::readUInt32(input, &_guid));
	guid.setRawValue(_guid);

	CHECK_READ(Parcel::readUInt32(input, &counter));
	CHECK_READ(Parcel::readUInt32(input, &flags));

	CHECK_READ(Parcel::readInt32(input, &stamina));
	CHECK_READ(Parcel::readInt32(input, &maxStamina));
	CHECK_READ(Parcel::readFloat(input, &staminaRegenRate));
	CHECK_READ(Parcel::readInt32(input, &chargeStartStamina));
	CHECK_READ(Parcel::readInt32(input, &chargedStamina));
	CHECK_READ(Parcel::readInt32(input, &chargeConsumesStamina));
	CHECK_READ(Parcel::readUInt32(input, &chargeCounter));
	CHECK_READ(Parcel::readInt32(input, &time));

	CHECK_READ(Parcel::readUInt32(input, &attackCounter));
	CHECK_READ(Parcel::readUInt32(input, &consumedStaminaTotal));
	CHECK_READ(Parcel::readUInt32(input, &attackInfoCounter));

	return true;
}


void StaminaInfo::writeToStream(DataOutputStream* output) const
{
	Parcel::writeUInt32(guid.getRawValue(), output);
	Parcel::writeUInt32(counter, output);
	Parcel::writeUInt32(flags, output);

	Parcel::writeInt32(stamina, output);
	Parcel::writeInt32(maxStamina, output);
	Parcel::writeFloat(staminaRegenRate, output);
	Parcel::writeInt32(chargeStartStamina, output);
	Parcel::writeInt32(chargedStamina, output);
	Parcel::writeInt32(chargeConsumesStamina, output);
	Parcel::writeUInt32(chargeCounter, output);
	Parcel::writeInt32(time, output);

	Parcel::writeUInt32(attackCounter, output);
	Parcel::writeUInt32(consumedStaminaTotal, output);
	Parcel::writeUInt32(attackInfoCounter, output);
}

void StaminaInfo::copyFrom(StaminaInfo const& right)
{
	guid = right.guid;
	counter = right.counter;
	flags = right.flags;
	stamina = right.stamina;
	maxStamina = right.maxStamina;
	staminaRegenRate = right.staminaRegenRate;
	chargeStartStamina = right.chargeStartStamina;
	chargedStamina = right.chargedStamina;
	chargeConsumesStamina = right.chargeConsumesStamina;
	chargeCounter = right.chargeCounter;
	time = right.time;
	attackCounter = right.attackCounter;
	consumedStaminaTotal = right.consumedStaminaTotal;
	attackInfoCounter = right.attackInfoCounter;
}

std::string StaminaInfo::description() const
{
	std::ostringstream ss;

	ss << "STAMINA INFO"
		<< std::setfill('0')
		<< std::hex
		<< " guid: 0x" << std::setw(8) << guid.getRawValue()
		<< std::dec
		<< " counter: " << counter
		<< " stamina: " << stamina
		<< " maxStamina: " << maxStamina
		<< " staminaRegenRate: " << staminaRegenRate
		<< " chargeStartStamina: " << chargeStartStamina
		<< " chargedStamina: " << chargedStamina
		<< " chargeConsumesStamina: " << chargeConsumesStamina
		<< " chargeCounter: " << chargeCounter
		<< " time: " << time
		<< " attackCounter: " << attackCounter
		<< " consumedStaminaTotal: " << consumedStaminaTotal
		<< " attackInfoCounter: " << attackInfoCounter;

	ss << " flags: ";

	if (flags == STAMINA_FLAG_NONE)
		ss << "STAMINA_FLAG_NONE";
	else
	{
		std::string flagStr;
		if ((flags & STAMINA_FLAG_ATTACK) != 0)
			flagStr.append("STAMINA_FLAG_ATTACK|");
		if ((flags & STAMINA_FLAG_CHARGING) != 0)
			flagStr.append("STAMINA_FLAG_CHARGING|");

		if (!flagStr.empty())
			ss << flagStr.substr(0, flagStr.size() - 1);
	}

	return ss.str();
}


