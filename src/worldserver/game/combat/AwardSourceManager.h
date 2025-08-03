#ifndef __AWARD_SOURCE_MANAGER_H__
#define __AWARD_SOURCE_MANAGER_H__

#include "game/dynamic/reference/RefManager.h"
#include "AwardeeReference.h"

class Unit;
class RewardManager;

class AwardSourceManager : public RefManager<Unit, RewardManager>
{
public:
	AwardeeReference* getFirst() { return ((AwardeeReference*)RefManager<Unit, RewardManager>::getFirst()); }
};

#endif // __AWARD_SOURCE_MANAGER_H__
