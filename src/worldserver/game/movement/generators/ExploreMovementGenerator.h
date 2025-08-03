#ifndef __EXPLORE_MOVEMENT_GENERATOR_H__
#define __EXPLORE_MOVEMENT_GENERATOR_H__

#include "MovementGenerator.h"
#include "step/TargetStepGenerator.h"
#include "game/maps/ExplorArea.h"

class Robot;

class ExploreMovementGenerator : public MovementGenerator
{
	enum ExplorState
	{
		EXPLOR_STATE_NONE,
		EXPLOR_STATE_NO_AREAS,
		EXPLOR_STATE_EXPLORING,
		EXPLOR_STATE_GOTO_UNEXPLORED,
		EXPLOR_STATE_GOTO_WAYPOINT,
		EXPLOR_STATE_GOTO_LINKED_WAYPOINT,
		EXPLOR_STATE_PATROLLING,

	};

    enum ExplorFilterType
    {
        FILTER_TYPE_NONE                    = 0,
        FILTER_TYPE_SAME_DISTRICT           = 1 << 0,
        FILTER_TYPE_UNEXPLORED              = 1 << 1,
		FILTER_TYPE_NOT_EXCLUDED			= 1 << 2,
    };
    
    typedef std::multimap<float, ExplorArea, std::less<float>> DistExplorAreaContainer;

public:
	ExploreMovementGenerator(Robot* owner);
	~ExploreMovementGenerator();

	MovementGeneratorType getType() const override { return MOVEMENT_GENERATOR_TYPE_EXPLORE; }
	bool update(NSTime diff) override;
	void finish() override;

private:
	void initialize();

	void sendMoveStart();
	void sendMoveStop();

	void moveStart();
	void moveStop();

	ExplorState nextExplorArea(ExplorArea const& currArea, ExplorArea& result);
	void getAdjacentExplorAreas(ExplorArea const& currArea, DistExplorAreaContainer& result, uint32 filter);
	void addExplorAreaIfNeeded(ExplorArea const& currArea, ExplorArea const& newArea, DistExplorAreaContainer& areaList, uint32 filter);
	bool getReachableTileInExplorArea(ExplorArea const& area, TileCoord& result);
	bool getSafeTileInExplorArea(ExplorArea const& area, TileCoord& result);

	bool moveAndExploreNewArea(ExplorArea const& startArea, TileCoord const& currCoord, ExplorArea const& goalArea, ExplorArea& newArea);
	bool moveAndExploreSafeArea(ExplorArea const& startArea, TileCoord const& currCoord, TileCoord& nextStep, ExplorArea& safeArea);
	bool move(TileCoord const& currCoord, TileCoord const& goalCoord);

	Robot* m_owner;
	TargetStepGenerator m_stepGenerator;

	ExplorState m_explorState;
	ExplorArea m_currExplorArea;
	ExplorArea m_goalExplorArea;
	ExplorArea m_currWaypointExplorArea;
	TileCoord m_targetWaypoint;
	TileCoord m_goalPatrolPoint;
};

#endif // __EXPLORE_MOVEMENT_GENERATOR_H__