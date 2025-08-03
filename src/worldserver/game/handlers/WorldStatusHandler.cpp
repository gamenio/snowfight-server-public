#include "game/server/WorldSession.h"

#include "game/server/protocol/pb/QueryWorldStatus.pb.h"
#include "game/server/protocol/pb/WorldStatus.pb.h"
#include "game/server/protocol/pb/QueryTheaterStatusList.pb.h"
#include "game/server/protocol/pb/TheaterStatusList.pb.h"
#include "game/server/protocol/pb/QueryPlayerStatusList.pb.h"
#include "game/server/protocol/pb/PlayerStatusList.pb.h"

#include "game/world/World.h"
#include "game/theater/TheaterManager.h"
#include "game/behaviors/Player.h"


void WorldSession::handleQueryWorldStatus(WorldPacket& recvPacket)
{
	QueryWorldStatus message;
	recvPacket.unpack(message);

	this->sendWorldStatus();
}

void WorldSession::sendWorldStatus()
{
	WorldPacket packet(SMSG_WORLD_STATUS);
	WorldStatus worldStatus;

	worldStatus.set_max_players(sTheaterManager->getPlayerLimit());
	worldStatus.set_online_players(sTheaterManager->getOnlineCount());
	worldStatus.set_update_diff(sWorld->getUpdateDiff());
	worldStatus.set_theater_count(sTheaterManager->getTheaterCount());
	worldStatus.set_queued_players(sTheaterManager->getQueuedPlayerCount());

	this->packAndSend(std::move(packet), worldStatus);
}


void WorldSession::handleQueryTheaterStatusList(WorldPacket& recvPacket)
{
	QueryTheaterStatusList message;
	recvPacket.unpack(message);

	this->sendTheaterStatusList();
}

void WorldSession::sendTheaterStatusList()
{
	WorldPacket packet(SMSG_THEATER_STATUS_LIST);
	TheaterStatusList theaterStatusList;

	std::vector<Theater*> const& theaterList = sTheaterManager->getTheaterList();
	for (auto it = theaterList.begin(); it != theaterList.end(); ++it)
	{
		Theater* theater = *it;
		BattleMap const* map = theater->getMap();

		TheaterStatus* theaterStatus = theaterStatusList.add_result();
		theaterStatus->set_id(theater->getTheaterId());
		theaterStatus->set_update_diff(theater->getUpdateDiff());
		theaterStatus->set_map_id(map->getMapId());
		theaterStatus->set_player_count(theater->getOnlineCount());
		theaterStatus->set_robot_count(map->getRobotCount());
		theaterStatus->set_item_count(map->getItemCount());
		theaterStatus->set_combat_grade(map->getCombatGrade().grade);
	}

	this->packAndSend(std::move(packet), theaterStatusList);
}

void WorldSession::handleQueryPlayerStatusList(WorldPacket& recvPacket)
{
	QueryPlayerStatusList message;
	recvPacket.unpack(message);

	uint32 theaterId = message.theater_id();
	this->sendPlayerStatusList(theaterId);
}

void WorldSession::sendPlayerStatusList(uint32 theaterId)
{
	WorldPacket packet(SMSG_PLAYER_STATUS_LIST);
	PlayerStatusList playerStatusList;

	Theater const* theater = sTheaterManager->findTheater(theaterId);
	if (theater)
	{
		auto const& sessionList = theater->getSessionList();
		for (auto it = sessionList.begin(); it != sessionList.end(); ++it)
		{
			WorldSession* session = (*it).second;
			Player* player = session->getPlayer();
			if (!player)
				continue;

			PlayerStatus* playerStatus = playerStatusList.add_result();
			playerStatus->set_guid(player->getData()->getGuid().getRawValue());
			playerStatus->set_name(player->getData()->getName());
			playerStatus->set_latency(session->getLatency(LATENCY_LATEST));
			playerStatus->set_attack_total(player->getAttackTotal());
			playerStatus->set_viewport_width(player->getData()->getViewport().width);
			playerStatus->set_viewport_height(player->getData()->getViewport().height);
			playerStatus->set_lang((int32)player->getData()->getLang());
			playerStatus->set_country(player->getData()->getCountry());
			playerStatus->set_gm_level(session->getGMLevel());
		}
	}


	this->packAndSend(std::move(packet), playerStatusList);
}