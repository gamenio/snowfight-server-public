#ifndef __PARCEL_H__
#define __PARCEL_H__

#include <google/protobuf/message_lite.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/wire_format_lite_inl.h>

#include "Common.h"


#define CHECK_READ(EXPRESSION) if (!GOOGLE_PREDICT_TRUE(EXPRESSION)) return false

using ::google::protobuf::internal::WireFormatLite;

typedef ::google::protobuf::io::CodedInputStream DataInputStream;
typedef ::google::protobuf::io::CodedOutputStream DataOutputStream;

class Parcel : public google::protobuf::MessageLite
{
public:
	Parcel();
	Parcel(Parcel const& right);
	Parcel(Parcel&& right);
	Parcel& operator=(Parcel const& right);

	inline Parcel* New() const  override { return NULL; }
	void Clear()  override;
	::std::string GetTypeName() const  override { return "Parcel"; }
	bool IsInitialized() const  override { return true; }
	void CheckTypeAndMergeFrom(::google::protobuf::MessageLite const& from)  override { }

	bool MergePartialFromCodedStream(::google::protobuf::io::CodedInputStream* input) override;
	void SerializeWithCachedSizes(::google::protobuf::io::CodedOutputStream* output) const override;

	int GetCachedSize() const  override { return m_cachedSize; }
	// 获取数据的字节数。存储字节数的变量将会被缓存下来避免重复计算。
	size_t ByteSizeLong() const override;

	virtual size_t sizeInBytes() const = 0;
	virtual bool readFromStream(DataInputStream* input) = 0;
	virtual void writeToStream(DataOutputStream* output) const = 0;


	static void writeInt32(int32 value, DataOutputStream* output);
	static void writeInt64(int64 value, DataOutputStream* output);
	static void writeUInt32(uint32 value, DataOutputStream* output);
	static void writeFixedUInt32(uint32 value, DataOutputStream* output);
	static void writeUInt64(uint64 value, DataOutputStream* output);
	static void writeFloat(float value, DataOutputStream* output);
	static void writeDouble(double value, DataOutputStream* output);
	static void writeBool(bool value, DataOutputStream* output);
	static void writeEnum(int value, DataOutputStream* output);
	static void writeString(std::string const& fieldName, std::string const& value, DataOutputStream* output);

	static bool readInt32(DataInputStream* input, int32* value);
	static bool readInt64(DataInputStream* input, int64* value);
	static bool readUInt32(DataInputStream* input, uint32* value);
	static bool readFixedUint32(DataInputStream* input, uint32* value);
	static bool readUInt64(DataInputStream* input, uint64* value);
	static bool readFloat(DataInputStream* input, float* value);
	static bool readDouble(DataInputStream* input, double* value);
	static bool readBool(DataInputStream* input, bool* value);
	static bool readEnum(DataInputStream* input, int* value);
	static bool readString(std::string const& fieldName, DataInputStream* input, std::string* value);

	static const size_t kFixedUInt32Size = 4;
	static const size_t kFloatSize = 4;
	static const size_t kDoubleSize = 8;
	static const size_t kBoolSize = 1;

	static size_t int32Size(int32 value);
	static size_t int64Size(int64 value);
	static size_t uint32Size(uint32 value);
	static size_t uint64Size(uint64 value);
	static size_t enumSize(int value);
	static size_t stringSize(std::string const& value);

private:
	void copyFrom(Parcel const& right);
	void moveFrom(Parcel& right);

	mutable int m_cachedSize;
};



inline void Parcel::writeInt32(int32 value, DataOutputStream* output)
{
	WireFormatLite::WriteInt32NoTag(value, output);
}

inline void Parcel::writeInt64(int64 value, DataOutputStream* output)
{
	WireFormatLite::WriteInt64NoTag(value, output);
}

inline void Parcel::writeUInt32(uint32 value, DataOutputStream* output)
{
	WireFormatLite::WriteUInt32NoTag(value, output);
}

inline void Parcel::writeFixedUInt32(uint32 value, DataOutputStream* output)
{
	WireFormatLite::WriteFixed32NoTag(value, output);
}

inline void Parcel::writeUInt64(uint64 value, DataOutputStream* output)
{
	WireFormatLite::WriteUInt64NoTag(value, output);
}

inline void Parcel::writeFloat(float value, DataOutputStream* output)
{
	WireFormatLite::WriteFloatNoTag(value, output);
}

inline void Parcel::writeDouble(double value, DataOutputStream* output)
{
	WireFormatLite::WriteDoubleNoTag(value, output);
}

inline void Parcel::writeBool(bool value, DataOutputStream* output)
{
	WireFormatLite::WriteBoolNoTag(value, output);
}

inline void Parcel::writeEnum(int value, DataOutputStream* output)
{
	WireFormatLite::WriteEnumNoTag(value, output);
}

inline void Parcel::writeString(std::string const& fieldName, std::string const& value, DataOutputStream* output)
{
	WireFormatLite::VerifyUtf8String(value.data(), static_cast<int>(value.length()), WireFormatLite::SERIALIZE, fieldName.c_str());
	output->WriteVarint32(static_cast<uint32>(value.size()));
	output->WriteString(value);
}

inline bool Parcel::readInt32(DataInputStream* input, int32* value)
{
	return WireFormatLite::ReadPrimitive<int32, WireFormatLite::TYPE_INT32>(input, value);
}

inline bool Parcel::readInt64(DataInputStream* input, int64* value)
{
	return WireFormatLite::ReadPrimitive<int64, WireFormatLite::TYPE_INT64>(input, value);
}

inline bool Parcel::readUInt32(DataInputStream* input, uint32* value)
{
	return WireFormatLite::ReadPrimitive<uint32, WireFormatLite::TYPE_UINT32>(input, value);
}

inline bool Parcel::readFixedUint32(DataInputStream* input, uint32* value)
{
	return WireFormatLite::ReadPrimitive<uint32, WireFormatLite::TYPE_FIXED32>(input, value);
}

inline bool Parcel::readUInt64(DataInputStream* input, uint64* value)
{
	return WireFormatLite::ReadPrimitive<uint64, WireFormatLite::TYPE_UINT64>(input, value);
}

inline bool Parcel::readFloat(DataInputStream* input, float* value)
{
	return WireFormatLite::ReadPrimitive<float, WireFormatLite::TYPE_FLOAT>(input, value);
}

inline bool Parcel::readDouble(DataInputStream* input, double* value)
{
	return WireFormatLite::ReadPrimitive<double, WireFormatLite::TYPE_DOUBLE>(input, value);
}

inline bool Parcel::readBool(DataInputStream * input, bool* value)
{
	return WireFormatLite::ReadPrimitive<bool, WireFormatLite::TYPE_BOOL>(input, value);
}

inline bool Parcel::readEnum(DataInputStream* input, int* value)
{
	return WireFormatLite::ReadPrimitive<int, WireFormatLite::TYPE_ENUM>(input, value);
}

inline bool Parcel::readString(std::string const& fieldName, DataInputStream* input, std::string* value)
{
	CHECK_READ(WireFormatLite::ReadString(input, value));
	CHECK_READ(WireFormatLite::VerifyUtf8String((*value).data(), static_cast<int>((*value).length()),WireFormatLite::PARSE, fieldName.c_str()));
	return true;
}


inline size_t Parcel::int32Size(int32 value)
{
	return WireFormatLite::Int32Size(value);
}

inline size_t Parcel::int64Size(int64 value)
{
	return WireFormatLite::Int64Size(value);
}

inline size_t Parcel::uint32Size(uint32 value)
{
	return WireFormatLite::UInt32Size(value);
}

inline size_t Parcel::uint64Size(uint64 value)
{
	return WireFormatLite::UInt64Size(value);
}

inline size_t Parcel::enumSize(int value)
{
	return WireFormatLite::EnumSize(value);
}

inline size_t Parcel::stringSize(std::string const& value)
{
	return WireFormatLite::StringSize(value);
}


#endif //__PARCEL_H__