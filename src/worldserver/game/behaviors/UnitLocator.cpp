#include "UnitLocator.h"

UnitLocator::UnitLocator()
{
	m_type |= TypeMask::TYPEMASK_UNIT_LOCATOR;
	m_typeId = TypeID::TYPEID_UNIT_LOCATOR;
}

UnitLocator::~UnitLocator()
{

}

void UnitLocator::addToWorld()
{
	if (this->isInWorld())
		return;

	LocatorObject::addToWorld();
}

void UnitLocator::removeFromWorld()
{
	if (!this->isInWorld())
		return;

	LocatorObject::removeFromWorld();
}