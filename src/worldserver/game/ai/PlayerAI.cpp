#include "PlayerAI.h"

#include "game/behaviors/Player.h"
#include "game/behaviors/ItemBox.h"
#include "game/behaviors/Item.h"
#include "game/behaviors/Projectile.h"

PlayerAI::PlayerAI(Player* player) :
	m_me(player)
{
}


PlayerAI::~PlayerAI()
{
	m_me = nullptr;
}

void PlayerAI::updateAI(NSTime diff)
{
}

void PlayerAI::moveInLineOfSight(ItemBox* itemBox)
{
}

void PlayerAI::moveInLineOfSight(Item* item)
{
}

void PlayerAI::moveInLineOfSight(Projectile* proj)
{
}
