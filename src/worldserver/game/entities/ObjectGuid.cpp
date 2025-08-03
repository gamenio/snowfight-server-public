#include "ObjectGuid.h"

#include <iomanip>

const ObjectGuid ObjectGuid::EMPTY = ObjectGuid();


char const* ObjectGuid::getTypeName(GuidType type)
{
	switch (type)
	{
	case GUIDTYPE_PLAYER:       return "Player";
	case GUIDTYPE_ROBOT:        return "Robot";
	case GUIDTYPE_ITEMBOX:		return "ItemBox";
	case GUIDTYPE_ITEM:			return "Item";
	case GUIDTYPE_CARRIED_ITEM:	return "CarriedItem";
	case GUIDTYPE_PROJECTILE:	return "Projectile";
	default:
		return "<unknown>";
	}
}

std::string ObjectGuid::toString() const
{
	std::ostringstream str;
	str << "GUID[Full: 0x" << std::hex << std::setw(8) << std::setfill('0') << m_guid << std::dec;
	str << " Type: " << getTypeName();
	str << " Counter: " << getCounter();
	str << "]";
	return str.str();
}