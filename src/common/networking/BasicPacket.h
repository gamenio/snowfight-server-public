#ifndef __BASIC_PACKET_H__
#define __BASIC_PACKET_H__

#include <iomanip>

#include <google/protobuf/message_lite.h>

#include "logging/Log.h"
#include "MessageBuffer.h"

using google::protobuf::MessageLite;

class PacketException : public std::exception
{
public:
	PacketException() noexcept { }
	explicit PacketException(std::string const& what) noexcept : m_message(what){ }
	~PacketException() noexcept { }

	char const* what() const noexcept { return m_message.c_str(); }

private:
	std::string m_message;
};

template<uint16 NUM_MSG_TYPES>
class BasicPacket
{
public:
	enum
	{
		// 数据头字节长度 = 主体2个字节 + 操作码2个字节
		HEADER_BYTE_SIZE = 4,

		// 最大允许的Body数据长度
		MAX_BODY_BYTE_SIZE = 8192,

		// 无效的操作码
		INVALID_OPCODE = 0,

		// 默认缓冲区大小
		DEFAULT_BUFFER_SIZE = 1024

	};

	BasicPacket() :
		m_bodySize(0),
		m_opcode(INVALID_OPCODE),
		m_bufferSize(0),
		m_buffer(nullptr),
		m_timestamp(0)
	{

	}

	explicit BasicPacket(uint16 opcode) :
		m_bodySize(0),
		m_opcode(opcode),
		m_bufferSize(0),
		m_buffer(nullptr),
		m_timestamp(0)
	{
	}

	BasicPacket(BasicPacket&& right) :
		m_bodySize(0),
		m_opcode(INVALID_OPCODE),
		m_bufferSize(0),
		m_buffer(nullptr),
		m_timestamp(0)
	{
		this->move(right);
	}

	BasicPacket& operator=(BasicPacket&& right)
	{
		if (this != &right)
			this->move(right);

		return *this;
	}

	BasicPacket(BasicPacket const& right) = delete;
	BasicPacket& operator=(BasicPacket const& right) = delete;

	~BasicPacket()
	{
		this->deallocBuffer();
	}

	void pack(MessageLite const& message)
	{
		m_bodySize = static_cast<uint16>(message.ByteSizeLong());
		this->allocBufferIfNeeded(m_bodySize);

		message.SerializeWithCachedSizesToArray(this->getBodyPointer());
	}


	void unpack(MessageLite& message) const
	{
		bool noerror = true;
		if (this->hasBody())
			noerror = message.ParseFromArray(this->getBodyPointer(), this->getBodyBytes());

		if (!noerror)
			throw PacketException(StringUtil::format("Message(%s) to parsing data failed. %s", message.GetTypeName().c_str(), this->description().c_str()));
	}


	// 将数据写入到MessageBuffer
	void write(MessageBuffer& buff)
	{
		this->encodeHeader(buff.getWritePointer());
		buff.writeCompleted(HEADER_BYTE_SIZE);

		if (this->hasBody())
		{
			memcpy(buff.getWritePointer(), this->getBodyPointer(), this->getBodyBytes());
			buff.writeCompleted(this->getBodyBytes());

		}
	}

	// 从MessageBuffer读取Header数据。
	// 如果Opcode的取值范围或者Body数据长度无效将抛出PacketException异常
	void decodeHeader(MessageBuffer& buff)
	{
		uint8* readPtr = buff.getReadPointer();
		uint16 size = 0;
		uint16 opcode = 0;

		size |= ((*(readPtr++) << 8) & 0xFF00);
		size |= (*(readPtr++) & 0xFF);
		opcode |= ((*(readPtr++) << 8) & 0xFF00);
		opcode |= (*(readPtr++) & 0xFF);

		// 无效的Body数据长度
		if (size > MAX_BODY_BYTE_SIZE)
		{
			throw PacketException(StringUtil::format("Failed to decode packet because body bytes(%d) > MAX_BODY_BYTE_SIZE(%d)", size, MAX_BODY_BYTE_SIZE));
		}


		// 无效的Opcode取值范围
		if (opcode >= NUM_MSG_TYPES)
		{
			throw PacketException(StringUtil::format("Failed to decode packet because unknown opcode(0x%X).", opcode));
		}

		m_bodySize = size;

		m_opcode = opcode;
		buff.readCompleted(HEADER_BYTE_SIZE);
	}

	void readBody(MessageBuffer& buff)
	{
		if (!hasBody())
			return;

		this->allocBufferIfNeeded(this->getBodyBytes());
		std::memcpy(this->getBodyPointer(), buff.getReadPointer(), this->getBodyBytes());

		buff.readCompleted(this->getBodyBytes());
	}

	void setOpcode(uint16 opcode) { m_opcode = opcode; }
	uint16 getOpcode() const { return m_opcode; }

	bool hasBody() const { return m_bodySize > 0; }
	uint16 getBodyBytes() const { return m_bodySize; }

	uint16 getByteSize() const { return HEADER_BYTE_SIZE + m_bodySize; }
    uint16 getBufferSize() const { return m_bufferSize;}

	void setTimestamp(int64 timestamp) { m_timestamp = timestamp; }
	int64 getTimestamp() const { return m_timestamp; }

	std::string description() const
	{
		uint8* bodyPtr = this->getBodyPointer();

		std::stringstream ss;
		ss << std::setfill('0');
		ss << std::hex;
		ss << "opcode 0x" << std::setw(4) << m_opcode;
		ss << std::dec;
		ss << ", timestamp " << m_timestamp;
		ss << ", buffer size " << m_bufferSize;
		ss << "\nbody size " << m_bodySize;
		ss << ", data: \n";
		ss << std::hex;
		int32 addr = 0;
		for (int32 i = 0; i < m_bodySize; ++i)
		{
			if (i % 16 == 0)
				ss << "\t0x" << std::setw(4) << addr << ": ";

			ss << std::setw(2) << (int32)(*bodyPtr++);
			if ((i + 1) % 16 == 0)
			{
				ss << "\n";
				addr += 16;
			}
			else
				ss << " ";

		}

		return ss.str();
	}
private:
	void encodeHeader(uint8* writePtr)
	{
		NS_ASSERT(m_opcode != INVALID_OPCODE);

		uint16 bodySize = this->getBodyBytes();

		*(writePtr++) = 0xFF & (bodySize >> 8);
		*(writePtr++) = 0xFF & bodySize;

		*(writePtr++) = 0xFF & (m_opcode >> 8);
		*(writePtr++) = 0xFF & m_opcode;
	}


	void move(BasicPacket& right)
	{
		std::swap(m_bodySize, right.m_bodySize);
		std::swap(m_opcode, right.m_opcode);
		std::swap(m_bufferSize, right.m_bufferSize);
		std::swap(m_buffer, right.m_buffer);
		std::swap(m_timestamp, right.m_timestamp);
	}

	void allocBufferIfNeeded(uint16 size)
	{
		if (m_buffer && m_bufferSize >= size)
			return;

		this->deallocBuffer();

		uint16 realSize = DEFAULT_BUFFER_SIZE;
		if (size > realSize)
			realSize = ((size + DEFAULT_BUFFER_SIZE - 1) / DEFAULT_BUFFER_SIZE) * DEFAULT_BUFFER_SIZE;

		m_bufferSize = realSize;
		m_buffer = new uint8[realSize];
	}

	void deallocBuffer()
	{
		if (m_buffer)
		{
			delete[] m_buffer;
			m_buffer = nullptr;
		}
	}

	uint8* getBodyPointer() const { return m_buffer; }

	uint16 m_bodySize;
	uint16 m_opcode;

	uint16 m_bufferSize;
	uint8* m_buffer;

	int64 m_timestamp;
};


#endif //__BASIC_PACKET_H__
