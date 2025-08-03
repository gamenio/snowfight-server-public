#include "LaunchReference.h"

#include "LaunchRefManager.h"

void LaunchReference::buildLink()
{
	this->getTarget()->insertFirst(this);
	this->getTarget()->incSize();
}

void LaunchReference::destroyLink()
{
	this->getTarget()->decSize();
}