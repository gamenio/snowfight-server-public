#include "UpdateObject.h"

#include "google/protobuf/io/zero_copy_stream_impl_lite.h"

#include "logging/Log.h"
#include "game/entities/DataBasic.h"

#define DATA_BUFFER_SIZE		512 // The size of the buffer used to store data

UpdateObject::UpdateObject()
{
}

UpdateObject::UpdateObject(UpdateObject&& right):
	Parcel(std::move(right)),
	m_datas(std::move(right.m_datas)),
	m_outOfRangeGUIDs(std::move(right.m_outOfRangeGUIDs))
{
}

// Adds an updated object data
void UpdateObject::addData(DataBasic* data, UpdateType updateType, uint32 updateFlags)
{
	NS_ASSERT(data->getNumberOfFields() > 0);

	std::string buff;
	buff.reserve(DATA_BUFFER_SIZE);
	google::protobuf::io::StringOutputStream sos(&buff);
	DataOutputStream output(&sos);

	// Write object metadata
	Parcel::writeEnum(updateType, &output);
	Parcel::writeUInt32(updateFlags, &output);
	Parcel::writeUInt32(data->getGuid().getRawValue(), &output);
	if (updateType == UPDATE_TYPE_CREATE)
	{
		Parcel::writeEnum(data->getTypeId(), &output);
	}

	// Write fields and update masks
	std::string fieldBuff;
	fieldBuff.reserve(DATA_BUFFER_SIZE);
	google::protobuf::io::StringOutputStream fieldSos(&fieldBuff);
	DataOutputStream fieldOutput(&fieldSos);

	FieldUpdateMask updateMask;
	updateMask.setCount(data->getNumberOfFields());
	data->writeFields(updateMask, updateType, updateFlags, &fieldOutput);

	updateMask.writeMask(&output);
	fieldOutput.Trim();
	output.WriteRaw(fieldBuff.data(), fieldBuff.size());

	output.Trim();
	if (buff.size() > DATA_BUFFER_SIZE)
		NS_LOG_DEBUG("entities.update", "Object's (%s) data size of %d bytes exceeds the default buffer size (%d bytes).", data->getGuid().toString().c_str(), (int32)buff.size(), DATA_BUFFER_SIZE);
	m_datas.emplace_back(std::move(buff));
}


size_t UpdateObject::sizeInBytes() const
{
	size_t totalSize = 0;

	uint32 blockCount = uint32(m_datas.size());
	if (!m_outOfRangeGUIDs.empty())
		blockCount++;

	totalSize += Parcel::uint32Size(blockCount);

	// The number of bytes in the GUID of objects that are out of range
	if (!m_outOfRangeGUIDs.empty())
	{
		totalSize += Parcel::enumSize(UPDATE_TYPE_OUT_OF_RANGE_OBJECTS);
		totalSize += Parcel::uint32Size(uint32(m_outOfRangeGUIDs.size()));
		for (auto it = m_outOfRangeGUIDs.begin(); it != m_outOfRangeGUIDs.end(); ++it)
			totalSize += Parcel::uint32Size((*it).getRawValue());
	}

	// The number of bytes of data in the updated objects
	for (auto const& d : m_datas)
	{
		totalSize += d.size();
	}

	return totalSize;
}


void UpdateObject::writeToStream(DataOutputStream* output) const
{
	uint32 blockCount = uint32(m_datas.size());
	if (!m_outOfRangeGUIDs.empty())
		blockCount++;

	Parcel::writeUInt32(blockCount, output);

	// Write the GUID of the object that is out of range
	if (!m_outOfRangeGUIDs.empty())
	{
		Parcel::writeEnum(UPDATE_TYPE_OUT_OF_RANGE_OBJECTS, output);
		Parcel::writeUInt32(uint32(m_outOfRangeGUIDs.size()), output);
		for(auto it = m_outOfRangeGUIDs.begin(); it != m_outOfRangeGUIDs.end(); ++it)
			Parcel::writeUInt32((*it).getRawValue(), output);
	}
	
	// Write the data of the updated objects
	for (auto const& d : m_datas)
	{
		output->WriteRaw(d.data(), d.size());
	}
}