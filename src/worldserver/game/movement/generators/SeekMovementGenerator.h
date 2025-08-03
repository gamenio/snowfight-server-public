#ifndef __SEEK_MOVEMENT_GENERATOR_H__
#define __SEEK_MOVEMENT_GENERATOR_H__

#include "game/behaviors/Robot.h"
#include "step/TargetStepGenerator.h"
#include "MovementGenerator.h"

class SeekMovementGenerator : public MovementGenerator
{
	typedef std::unordered_map<int32 /* TileIndex */, std::list<TileCoord>::iterator> TileListIteratorMap;
public:
	SeekMovementGenerator(Robot* owner, TileCoord const& hidingSpot);
	virtual ~SeekMovementGenerator();

	MovementGeneratorType getType() const override { return MOVEMENT_GENERATOR_TYPE_SEEK; }
	bool update(NSTime diff) override;
	void finish() override;

private:
	void sendMoveStart();
	void sendMoveStop();

	void moveStart();
	void moveStop();

	void addAdjacentHidingSpots(TileCoord const& currCoord);
	void addPendingHidingSpotIfNeeded(TileCoord const& hidingSpot);
	void updatePendingHidingSpots();
	bool selectNextPendingHidingSpot(TileCoord& hidingSpot);

	void addPendingHidingSpot(TileCoord const& hidingSpot);
	bool isPendingHidingSpot(TileCoord const& hidingSpot) const;
	void removePendingHidingSpot(TileCoord const& hidingSpot);

	Robot* m_owner;

	std::list<TileCoord> m_pendingHidingSpotList;
	TileListIteratorMap m_pendingHidingSpotLookupTable;
	TargetStepGenerator m_targetStepGenerator;
};


#endif // __SEEK_MOVEMENT_GENERATOR_H__

