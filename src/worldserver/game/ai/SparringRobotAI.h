#ifndef __SPARRING_ROBOT_AI_H__
#define __SPARRING_ROBOT_AI_H__

#include <bitset>

#include "utilities/Timer.h"
#include "game/movement/generators/MovementGenerator.h"
#include "RobotAI.h"
#include "TargetSelector.h"


class AttackableObject;

class SparringRobotAI : public RobotAI
{
	enum HidingSpotPurpose
	{
		HIDING_SPOT_FOR_HIDE,
		HIDING_SPOT_FOR_SEEK,
		MAX_HIDING_SPOT_PURPOSES
	};

	enum NoTargetAction
	{
		NOTARGET_ACTION_NONE,
		NOTARGET_ACTION_EXPLORE,
		MAX_NOTARGET_ACTIONS,
	};
public:
	SparringRobotAI(Robot* robot);
	~SparringRobotAI();

	RobotAIType getType() const override { return ROBOTAI_TYPE_SPARRING; }

	void updateAI(NSTime diff) override;
	void resetAI() override;

	void combatDelayed(Unit* victim) override;
	void combatStart(Unit* victim) override;
	void exploreDelayed();

	void moveInLineOfSight(Unit* who) override;
	void moveInLineOfSight(ItemBox* itemBox) override;
	void moveInLineOfSight(Item* item) override;
	void moveInLineOfSight(Projectile* proj) override;
	void discoverHidingSpot(TileCoord const& tileCoord) override;

	void sayOpeningSmileyIfNeeded() override;

private:
	void updateReactionTimer(NSTime diff);

	void sayOneOfSmileys(std::vector<uint16> const& smileys);
	void updateOpeningSmileyTimer(NSTime diff);

	void updateCollect();
	void updateHide();
	void updateCombat();
	void updateUnlock();
	void updateExplore();

	void attackStart(AttackableObject* target, AIActionType action);
	void clearAttackInAction(AIActionType action);

	void setPendingHidingSpot(TileCoord const& hidingSpot, HidingSpotPurpose purpose);
	void setPendingNoTargetAction(NoTargetAction action);

	void doUse();
	void doCollect();
	void doHide();
	void doCombat();
	void doUnlock();
	void doSeek();
	void doExplore();

	void applyCombatMotion(Unit* target);

	void unlockDelayed(ItemBox* itemBox);
	void unlockStart(ItemBox* itemBox);
	void applyUnlockMotion(ItemBox* itemBox, AIActionType action);

	void collectDelayed(Item* item);
	void collectStart(Item* item);
	void applyCollectionMotion(Item* item, AIActionType action);

	void discoverSpotToHide(TileCoord const& tileCoord);
	void hideStart(TileCoord const& hidingSpot);

	void discoverSpotToSeek(TileCoord const& tileCoord);
	void seekStart(TileCoord const& hidingSpot);

	void exploreStart();

	DelayTimer m_reactionTimer;
	bool m_isReactionTimeDirty;

	TargetSelector m_targetSelector;
	std::array<TileCoord, MAX_HIDING_SPOT_PURPOSES> m_pendingHidingSpots;
	std::bitset<MAX_NOTARGET_ACTIONS> m_pendingNoTargetActionFlags;

	AIActionType m_attackInAction;
	DelayTimer m_openingSmileyTimer;
};


#endif // __SPARRING_ROBOT_AI_H__

