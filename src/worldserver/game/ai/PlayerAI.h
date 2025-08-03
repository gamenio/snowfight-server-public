#ifndef __PLAYER_AI_H__
#define __PLAYER_AI_H__

#include "UnitAI.h"

class Player;
class ItemBox;
class Item;
class Projectile;

class PlayerAI : public UnitAI
{
public:
	PlayerAI(Player* player);
	~PlayerAI();

	void updateAI(NSTime diff) override;
	void moveInLineOfSight(ItemBox* itemBox);
	void moveInLineOfSight(Item* item);
	void moveInLineOfSight(Projectile* proj);

private:
	Player* m_me;
};

#endif // __PLAYER_AI_H__

