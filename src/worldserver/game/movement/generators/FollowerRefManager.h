#ifndef __FOLLOWER_REFMANAGER__
#define __FOLLOWER_REFMANAGER__

#include "game/dynamic/reference/RefManager.h"

template<typename T> class TargetMovementGenerator;

template<typename TARGET>
class FollowerRefManager : public RefManager<TARGET, TargetMovementGenerator<TARGET>>
{

};


#endif // __FOLLOWER_REFMANAGER__
