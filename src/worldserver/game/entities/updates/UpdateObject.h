#ifndef __UPDATE_OBJECT_H__
#define __UPDATE_OBJECT_H__

#include "Common.h"
#include "game/server/protocol/Parcel.h"
#include "game/entities/DataTypes.h"
#include "game/entities/ObjectGuid.h"

enum UpdateType
{
	UPDATE_TYPE_VALUES,
	UPDATE_TYPE_CREATE,
	UPDATE_TYPE_OUT_OF_RANGE_OBJECTS
};

enum UpdateFlag
{
	UPDATE_FLAG_NONE = 0,
	UPDATE_FLAG_SELF = 1 << 0,
};

class Player;
class DataBasic;
class UpdateObject;

typedef std::unordered_map<Player*, UpdateObject> PlayerUpdateMapType;

class UpdateObject : public Parcel
{
public:
	UpdateObject();
	UpdateObject(UpdateObject&& right);

	std::string GetTypeName() const override { return "UpdateObject"; }

	void addData(DataBasic* data, UpdateType updateType, uint32 updateFlags = UPDATE_FLAG_NONE);
	bool hasUpdate() const { return !m_datas.empty() || !m_outOfRangeGUIDs.empty(); }

	void addOutOfRangeGUID(ObjectGuid const& guid) { m_outOfRangeGUIDs.insert(guid); }
	void addOutOfRangeGUIDSet(GuidUnorderedSet& guids) { m_outOfRangeGUIDs.insert(guids.begin(), guids.end()); }

	virtual size_t sizeInBytes() const override;

	bool readFromStream(DataInputStream* input) override { return false; }
	void writeToStream(DataOutputStream* output) const override;

private:
	std::vector<std::string> m_datas;
	GuidUnorderedSet m_outOfRangeGUIDs;
};

#endif //__UPDATE_OBJECT_H__