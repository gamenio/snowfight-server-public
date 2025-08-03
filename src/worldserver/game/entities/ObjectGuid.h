#ifndef __OBJECT_GUID_H__
#define __OBJECT_GUID_H__

#include "Common.h"

enum GuidType
{
	GUIDTYPE_PLAYER,
	GUIDTYPE_ROBOT,
	GUIDTYPE_ITEMBOX,
	GUIDTYPE_ITEM,
	GUIDTYPE_CARRIED_ITEM,
	GUIDTYPE_PROJECTILE,
	GUIDTYPE_UNIT_LOCATOR,
};

typedef uint32 GuidCounter;

class ObjectGuid
{
public:
	static const ObjectGuid EMPTY;

	ObjectGuid() :
		m_guid(0)
	{ }

	explicit ObjectGuid(uint32 guid) : m_guid(guid) { }

	ObjectGuid(GuidType type, GuidCounter counter) :
		m_guid(type << 24 | counter)
	{ }

	bool operator!() const { return isEmpty(); }
	bool operator== (ObjectGuid const& guid) const { return getRawValue() == guid.getRawValue(); }
	bool operator!= (ObjectGuid const& guid) const { return getRawValue() != guid.getRawValue(); }
	bool operator< (ObjectGuid const& guid) const { return getRawValue() < guid.getRawValue(); }

	uint32 getRawValue() const { return m_guid; }
	void setRawValue(uint32 guid) { m_guid = guid; }
	void clear() { m_guid = 0; }

	GuidType getType() const { return static_cast<GuidType>(m_guid >> 24 & 0xFF); }

	GuidCounter getCounter() const { return m_guid & 0x00FFFFFF; }
	static GuidCounter getMaxCounter() { return 0x00FFFFFF; }

	bool isEmpty() const { return m_guid == 0; }
	bool isPlayer() const { return this->getType() == GUIDTYPE_PLAYER; }
	bool isRobot() const { return this->getType() == GUIDTYPE_ROBOT; }
	bool isItemBox() const { return this->getType() == GUIDTYPE_ITEMBOX; }
	bool isItem() const { return this->getType() == GUIDTYPE_ITEM; }
	bool isCarriedItem() const { return this->getType() == GUIDTYPE_CARRIED_ITEM; }
	bool isProjectile() const { return this->getType() == GUIDTYPE_PROJECTILE; }
	bool isUnitLocator() const { return this->getType() == GUIDTYPE_UNIT_LOCATOR; }

	static char const* getTypeName(GuidType type);
	char const* getTypeName() const { return !isEmpty() ? getTypeName(getType()) : "None"; };
	std::string toString() const;

private:
	uint32 m_guid;
};

typedef std::set<ObjectGuid> GuidSet;
typedef std::unordered_set<ObjectGuid> GuidUnorderedSet;


namespace std
{
	template<>
	struct hash<ObjectGuid>
	{
	public:
		size_t operator()(ObjectGuid const& key) const
		{
			return hash<uint32>()(key.getRawValue());
		}
	};
}


#endif // __OBJECT_GUID_H__


