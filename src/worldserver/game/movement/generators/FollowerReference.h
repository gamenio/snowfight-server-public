#ifndef __FOLLOWER_REFERENCE_H__
#define __FOLLOWER_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"

template<typename T> class TargetMovementGenerator;

template<typename TARGET>
class FollowerReference : public Reference<TARGET, TargetMovementGenerator<TARGET>>
{
public:
	~FollowerReference()
	{
		this->unlink();
	}

protected:
	void buildLink() override
	{
		this->getTarget()->getFollowerRefManager()->insertFirst(this);
		this->getTarget()->getFollowerRefManager()->incSize();
	}

	void destroyLink() override 
	{ 
		this->getTarget()->getFollowerRefManager()->decSize();
	}
};


#endif // __FOLLOWER_REFERENCE_H__
