#ifndef __MESSAGE_BUFFER_H__
#define __MESSAGE_BUFFER_H__

#include <memory>
#include <algorithm>
#include <cstring>

#include "Common.h"


class MessageBuffer
{
public:
	MessageBuffer() :
		m_rpos(0),
		m_wpos(0),
		m_timestamp(0)
	{

	}

	explicit MessageBuffer(uint16 bufferSize) :
		m_rpos(0),
		m_wpos(0),
		m_timestamp(0)
	{
		m_storage.resize(bufferSize);
	}

	MessageBuffer(MessageBuffer const& right) :
		m_rpos(0),
		m_wpos(0),
		m_timestamp(0)
	{
		this->copy(right);
	}


	MessageBuffer(MessageBuffer&& right) :
		m_rpos(0),
		m_wpos(0),
		m_timestamp(0)
	{
		this->move(right);
	}

	MessageBuffer& operator=(MessageBuffer const& right)
	{
		if (this != &right)
			this->copy(right);
		return *this;
	}

	MessageBuffer& operator=(MessageBuffer&& right)
	{
		if (this != &right)
			this->move(right);

		return *this;
	}

	~MessageBuffer()
	{
	}


	uint8* getBasePointer() { return m_storage.data(); }
	uint8 const* getBasePointer() const { return m_storage.data(); }

	uint8* getReadPointer() { return getBasePointer() + m_rpos; }
	uint8* getWritePointer() { return getBasePointer() + m_wpos; }

	void readCompleted(uint16 bytes) { m_rpos += bytes; }
	void writeCompleted(uint16 bytes) { m_wpos += bytes; }

	uint16 getActiveSize() const { return m_wpos - m_rpos; }
	uint16 getRemainingSpace() const { return static_cast<uint16>(m_storage.size() - m_wpos); }
	uint16 getBufferSize() const { return static_cast<uint16>(m_storage.size()); }

	void reset()
	{
		m_wpos = 0;
		m_rpos = 0;
	}

	void resize(uint16 newSize)
	{
		m_storage.resize(newSize);
	}

	// 丢弃不活动的数据
	void normalize()
	{
		if (m_rpos)
		{
			if (m_rpos != m_wpos)
				memmove(getBasePointer(), getReadPointer(), getActiveSize());
			m_wpos -= m_rpos;
			m_rpos = 0;
		}
	}

	// 确保剩余空间足够容纳期望的数据大小
	void ensureFreeSpace(uint16 expectSize)
	{
		this->normalize();
		if (getRemainingSpace() < expectSize)
		{
			auto newSize = ((expectSize + this->getActiveSize() + m_storage.size() - 1) / m_storage.size()) * m_storage.size();
			m_storage.resize(newSize);
		}
	}

	void setTimestamp(int64 timestamp) { m_timestamp = timestamp; }
	int64 getTimestamp() const { return m_timestamp; }

private:
	void copy(MessageBuffer const& right)
	{
		m_rpos = right.m_rpos;
		m_wpos = right.m_wpos;
		m_storage = right.m_storage;
		m_timestamp = right.m_timestamp;
	}

	void move(MessageBuffer& right)
	{
		m_storage = std::move(right.m_storage);
		std::swap(m_rpos, right.m_rpos);
		std::swap(m_wpos, right.m_wpos);
		std::swap(m_timestamp, right.m_timestamp);
	}

	uint16 m_rpos;
	uint16 m_wpos;
	std::vector<uint8> m_storage;

	int64 m_timestamp;
};


#endif //__MESSAGE_BUFFER_H__
