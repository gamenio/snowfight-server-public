#ifndef __LAUNCH_REFERENCE_H__
#define __LAUNCH_REFERENCE_H__

#include "game/dynamic/reference/Reference.h"

class Unit;
class LaunchRefManager;

class LaunchReference : public Reference<LaunchRefManager, Unit>
{
public:
	~LaunchReference() { this->unlink(); }
	LaunchReference* next() { return (LaunchReference*)Reference<LaunchRefManager, Unit>::next(); }

protected:
	void buildLink() override;
	void destroyLink() override;
};

#endif // __LAUNCH_REFERENCE_H__

