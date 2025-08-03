#ifndef __FIELD_UPDATE_MASK_H__
#define __FIELD_UPDATE_MASK_H__

#include "Common.h"
#include "game/server/protocol/Parcel.h"


class FieldUpdateMask
{
	typedef uint32 BlockType;

	enum
	{
		BLOCK_BITS = sizeof(BlockType) * 8
	};

public:
	FieldUpdateMask() :
		m_bits(nullptr),
		m_fieldCount(0),
		m_blockCount(0)
	{

	}

	~FieldUpdateMask()
	{
		if (m_bits)
		{
			delete[] m_bits;
			m_bits = nullptr;
		}
	}

	void setCount(uint32 fieldCount)
	{
		if (m_bits)
		{
			delete[] m_bits;
			m_bits = nullptr;
		}

		m_fieldCount = fieldCount;
		m_blockCount = (m_fieldCount + BLOCK_BITS - 1) / BLOCK_BITS;

		m_bits = new uint8[m_blockCount * BLOCK_BITS];
		this->reset();
	}

	uint32 count() const { return m_fieldCount; }

	void setBit(uint32 index, bool value = true)
	{
		if(m_bits)
			m_bits[index] = value;
	}

	void resetBit(uint32 index)
	{
		if(m_bits)
			m_bits[index] = 0;
	}

	bool testBit(uint32 index) const
	{
		if(m_bits)
			return m_bits[index] != 0;

		return false;
	}

	void reset()
	{
		if (m_bits)
			memset(m_bits, 0, sizeof(uint8) * m_blockCount * BLOCK_BITS);
	}

	uint32 byteSize() const
	{
		return m_blockCount * Parcel::kFixedUInt32Size;
	}

	void writeMask(DataOutputStream* output) const
	{
		for (uint32 i = 0; i < m_blockCount; ++i)
		{
			BlockType maskPart = 0;
			for (uint32 j = 0; j < BLOCK_BITS; ++j)
				if (m_bits[i * BLOCK_BITS + j])
					maskPart |= 1 << j;

			Parcel::writeFixedUInt32(maskPart, output);
		}
	}

private:
	uint8* m_bits;
	uint32 m_fieldCount;
	uint32 m_blockCount;
};

#endif //__FIELD_UPDATE_MASK_H__