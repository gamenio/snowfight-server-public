#include "game/server/WorldSession.h"

#include "game/server/protocol/pb/PlayerLogin.pb.h"
#include "game/server/protocol/pb/JoinTheater.pb.h"
#include "game/server/protocol/pb/TheaterInfo.pb.h"
#include "game/server/protocol/pb/QueryCharacterInfo.pb.h"
#include "game/server/protocol/pb/CharacterInfo.pb.h"
#include "game/server/protocol/pb/SmileyChat.pb.h"
#include "game/server/protocol/pb/PlayerActionMessage.pb.h"

#include "game/server/protocol/OpcodeHandler.h"
#include "game/behaviors/Player.h"
#include "game/world/ObjectAccessor.h"
#include "game/world/Locale.h"
#include "game/world/ObjectMgr.h"
#include "game/theater/Theater.h"
#include "game/theater/TheaterManager.h"
#include "game/world/World.h"
#include "game/utils/MathTools.h"
#include "game/maps/MapDataManager.h"

void WorldSession::handlePlayerLogin(WorldPacket& recvPacket)
{
	PlayerLogin playerLogin;
	recvPacket.unpack(playerLogin);

	Player* myChar = this->getPlayer();
	if (myChar)
	{
		NS_LOG_ERROR("world.handler.character", "Player(%s) has logged in from client %s:%d.", myChar->getData()->getGuid().toString().c_str(), m_remoteAddress.c_str(), m_remotePort);
		return;
	}
	myChar = new Player(this);
	if (!myChar->loadData(playerLogin.char_id()))
	{
		NS_LOG_ERROR("world.handler.character", "Player data load failed from client %s:%d.", m_remoteAddress.c_str(), m_remotePort);
		this->kickPlayer();

		delete myChar;
		myChar = nullptr;
		return;
	}
	myChar->getData()->setViewport(Size(float(playerLogin.screen_width()), float(playerLogin.screen_height())));
	LangType lang = getLangTypeByLangTag(playerLogin.lang());
	myChar->getData()->setLang(lang);
	myChar->getData()->setCountry(playerLogin.country());
	myChar->getData()->setProperty(playerLogin.property());
	myChar->getData()->setControllerType(static_cast<ControllerType>(playerLogin.controller_type()));
	if (playerLogin.level() == 0)
		myChar->getData()->setNewPlayer(true);
	myChar->getData()->setTrainee(playerLogin.trainee());
		
	myChar->getData()->setRewardStage(static_cast<uint8>(playerLogin.reward_stage()));
	myChar->getData()->setDailyRewardDays(playerLogin.daily_reward_days());

	// 设置玩家等级和经验值
	myChar->setLevelAndXP(playerLogin.level(), playerLogin.experience());

	// 给角色设置一个有效的名称
	if (playerLogin.char_name().empty())
		myChar->getData()->setName(sObjectMgr->generatePlayerName(lang));
	else
		myChar->getData()->setName(playerLogin.char_name());

	// 设置属性阶段并初始化属性
	for (int32 i = 0; i < playerLogin.stat_stage_list_size(); ++i)
	{
		PlayerLogin::StatStage const& statStage = playerLogin.stat_stage_list(i);
		myChar->getData()->setStatStage(static_cast<StatType>(statStage.type()), static_cast<uint8>(statStage.stage()));
	}
	myChar->resetStats();

	// 找到玩家的战斗等级
	if (myChar->getData()->isTrainee())
		myChar->getData()->setCombatGrade(TRAINING_GROUND_COMBAT_GRADE);
	else
	{
		CombatGrade const* combatGrade = sObjectMgr->findCombatGrade(myChar->getData()->getCombatPower());
		NS_ASSERT(combatGrade);
		myChar->getData()->setCombatGrade(combatGrade->grade);
	}

	// 为玩家选择地图
	uint16 mapId = sTheaterManager->selectMapForPlayer(this);
	myChar->getData()->setSelectedMapId(mapId);

	sTheaterManager->addPlayerToTheater(this);

	NS_LOG_INFO("world.handler.character", "Player(%s) logged in from client %s:%d, playerid: %s, original playerid: %s", myChar->getData()->getGuid().toString().c_str(), m_remoteAddress.c_str(), m_remotePort, m_playerId.c_str(), m_originalPlayerId.c_str());
}

void WorldSession::onSessionAcceptedByTheater(Theater* theater)
{
	m_theater = theater;

	this->sendTheaterInfo();
}

void WorldSession::sendPlayerLoggedInMesssage()
{
	NS_ASSERT(this->getPlayer(), "Player not logged in.");

	PlayerActionMessage message;
	message.set_type(PlayerActionMessage::PLAYER_LOGGED_IN);
	message.set_name(this->getPlayer()->getData()->getName());
	
	this->getTheater()->getMap()->sendGlobalPlayerActionMessage(message, this->getPlayer());
}

void WorldSession::handlePlayerLogout(WorldPacket&)
{
	if (!this->getPlayer())
		return;

	this->logoutPlayer();
}

void WorldSession::handleJoinTheater(WorldPacket& recvPacket)
{
	JoinTheater joinTheater;
	recvPacket.unpack(joinTheater);

	Theater* theater = this->getTheater();
	NS_ASSERT(theater, "Theater was not created");

	BattleMap* map = theater->getMap();
	Player* myChar = this->getPlayer();
	if (!myChar)
	{
		NS_LOG_ERROR("world.handler.character", "Player(%s) not logged in from client %s:%d.", myChar->getData()->getGuid().toString().c_str(), m_remoteAddress.c_str(), m_remotePort);
		return;
	}
	// 恢复玩家在世界中的角色
	if (myChar->isInWorld())
	{
		myChar->setSession(this);

		myChar->chargeStop();
		myChar->getData()->resetAttackCounter();
		myChar->getData()->resetConsumedStaminaTotal();

		myChar->sendResetPackets();
		map->sendBattleUpdate(myChar, BATTLE_UPDATEFLAG_ALL);

		if (map->isBattleEnded() || myChar->getWithdrawalState() == WITHDRAWAL_STATE_WITHDREW)
		{
			myChar->sendBattleResult(myChar->getBattleOutcome(), myChar->getRankNo());
		}
		NS_LOG_INFO("world.handler.character", "Player(%s) restored from client %s:%d", myChar->getData()->getGuid().toString().c_str(), m_remoteAddress.c_str(), m_remotePort);
	}
	else
	{
		// 是否需要等待玩家
		if (theater->waitForPlayers())
		{
			if (theater->getSpawnManager()->addPlayerToQueue(myChar))
			{
				// 学员跳过等待玩家
				if (myChar->getData()->isTrainee())
					theater->skipWaitingPlayers();
				else
					theater->sendWaitForPlayersToPlayer(this);
			}
			else
				NS_LOG_ERROR("world.handler.character", "Player(%s) from client %s:%d are already in queue waiting to battle.", myChar->getData()->getGuid().toString().c_str(), m_remoteAddress.c_str(), m_remotePort);
		}
		else
		{
			myChar->spawn(map);

			map->addPlayerToMap(myChar);
			myChar->sendInitialPacketsAfterAddToMap();
			map->sendBattleUpdate(myChar, BATTLE_UPDATEFLAG_ALL);
		}
	}
}


void WorldSession::sendTheaterInfo()
{
	WorldPacket packet(SMSG_THEATER_INFO);

	TheaterInfo info;
	BattleMap* map = this->getTheater()->getMap();
	info.set_map_id(map->getMapId());
	info.set_theater_id(this->getTheaterId());
	info.set_combat_grade(map->getCombatGrade().grade);
	info.set_safezone_center_x(map->getSafeZoneCenter().x);
	info.set_safezone_center_y(map->getSafeZoneCenter().y);
	info.set_safezone_radius(map->getInitialSafeZoneRadius());
	info.set_battle_duration(map->getDurationByBattleState(BATTLE_STATE_IN_PROGRESS));
	float adChance = sWorld->getFloatConfig(CONFIG_BATTLE_END_AD_CHANCE);
	if(rollChance(adChance))
		info.set_request_battle_end_ad(true);
	info.set_enable_app_review_mode(sWorld->getBoolConfig(CONFIG_BATTLE_ENABLE_APP_REVIEW_MODE));

	this->packAndSend(std::move(packet), info);
}


void WorldSession::handleQueryCharacterInfo(WorldPacket& recvPacket)
{
	QueryCharacterInfo query;
	recvPacket.unpack(query);

	ObjectGuid guid(query.guid());
	Unit* unit = ObjectAccessor::getUnit(this->getPlayer(), guid);
	// 被查询的角色有可能已经退出游戏
	if(unit)
		this->sendCharacterInfo(unit);
}

void WorldSession::sendCharacterInfo(Unit* unit)
{
	WorldPacket packet(SMSG_CHARACTER_INFO);

	CharacterInfo info;
	info.set_guid(unit->getData()->getGuid().getRawValue());
	info.set_name(unit->getData()->getName());
	this->packAndSend(std::move(packet), info);
}

void WorldSession::handleSmileyChat(WorldPacket& recvPacket)
{
	Player* talker = this->getPlayer();
	if (!talker->isInWorld() || talker->getMap()->isBattleEnded())
		return;

	SmileyChat smileyChat;
	recvPacket.unpack(smileyChat);

	uint16 code = static_cast<uint16>(smileyChat.code());
	talker->saySmiley(code);
}
