#include "game/server/WorldSession.h"

#include "game/server/protocol/pb/UseItem.pb.h"
#include "game/behaviors/Player.h"
#include "game/world/ObjectMgr.h"

void WorldSession::handleUseItem(WorldPacket& recvPacket)
{
	Player* user = this->getPlayer();
	if (!user->isInWorld() || !user->isAlive() || user->getMap()->isBattleEnded())
		return;

	UseItem useItem;
	recvPacket.unpack(useItem);

	int32 slot = useItem.slot();
	ObjectGuid itemGuid(useItem.item());

	CarriedItem* item = user->getItem(slot);
	if (!item)
	{
		user->sendItemUseResult(itemGuid, ITEM_USE_STATUS_FAILED);
		return;
	}

	if (item->getData()->getGuid() != itemGuid)
	{
		user->sendItemUseResult(itemGuid, ITEM_USE_STATUS_FAILED);
		return;
	}

	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(item->getData()->getItemId());
	if (!tmpl)
	{
		user->sendItemUseResult(itemGuid, ITEM_USE_STATUS_FAILED);
		return;
	}

	if (!user->canUseItem(item))
	{
		user->sendItemUseResult(itemGuid, ITEM_USE_STATUS_FAILED);
		return;
	}

	user->useItem(item);

	if (user->getData()->isTrainee())
		user->getMap()->getSpawnManager()->fillRobotsIfNeeded();
}