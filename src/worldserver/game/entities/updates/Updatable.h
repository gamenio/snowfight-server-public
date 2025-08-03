#ifndef __UPDATABLE_H__
#define __UPDATABLE_H__

#include <boost/core/noncopyable.hpp>

#include "FieldUpdateMask.h"


class Updatable: private boost::noncopyable
{
public:
	Updatable() { }
	virtual ~Updatable() {}

	uint32 getNumberOfFields() const { return m_updateMask.count(); }

	void setUpdatedField(uint32 fieldIndex) {  m_updateMask.setBit(fieldIndex); }
	virtual void clearUpdateFlags() { m_updateMask.reset(); }

	bool hasUpdatedField(uint32 fieldIndex) const { return m_updateMask.testBit(fieldIndex); }

protected:
	FieldUpdateMask m_updateMask;
};


#endif // __UPDATABLE_H__
