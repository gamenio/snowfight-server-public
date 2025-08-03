#include "game/handlers/GMCommandWorker.h"

#include "game/server/protocol/pb/FlashMessage.pb.h"

#include "configuration/Config.h"
#include "game/world/World.h"
#include "game/world/ObjectAccessor.h"
#include "game/world/ObjectMgr.h"
#include "game/behaviors/UnitLocator.h"
#include "game/behaviors/Player.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/ItemBox.h"
#include "game/behaviors/Item.h"

struct GMCommandHolder
{
	CallbackPtr callback;
	boost::container::map<std::string, GMCommandHolder> subCommands;
};


boost::container::map<std::string, GMCommandHolder> gHealthCommandTable = {
	{ "regenrate",					{ &GMCommandWorker::executeHealthRegenRateCommand					} }, 
};

boost::container::map<std::string, GMCommandHolder> gStaminaCommandTable = {
	{ "regenrate",					{ &GMCommandWorker::executeStaminaRegenRateCommand					} }, 
	{ "chargeconsumes",				{ &GMCommandWorker::executeStaminaChargeConsumesCommand				} },
};

boost::container::map<std::string, GMCommandHolder> gTransportCommandTable = {
	{ "xy",							{ &GMCommandWorker::executeTransportXYCommand						} },
};

boost::container::map<std::string, GMCommandHolder> gReloadCommandTable = {
    { "config",                  { &GMCommandWorker::executeReloadConfigCommand                         } },
};

boost::container::map<std::string, GMCommandHolder> gCommandRootTable = {
	{ "gm",					{ &GMCommandWorker::executeGMCommand										} },
	{ "movespeed",			{ &GMCommandWorker::executeMoveSpeedCommand									} }, 
	{ "health",				{ &GMCommandWorker::executeHealthCommand,			gHealthCommandTable		} }, 
	{ "defense",			{ &GMCommandWorker::executeDefenseCommand,									} },
	{ "level",				{ &GMCommandWorker::executeLevelCommand										} }, 
	{ "attackrange",		{ &GMCommandWorker::executeAttackRangeCommand								} },
	{ "stamina",			{ &GMCommandWorker::executeStaminaCommand,			gStaminaCommandTable	} },
	{ "damage",				{ &GMCommandWorker::executeDamageCommand									} },
	{ "add",				{ &GMCommandWorker::executeAddCommand										} },
	{ "del",				{ &GMCommandWorker::executeDelCommand										} },
	{ "die",				{ &GMCommandWorker::executeDieCommand										} },
	{ "receivedamage",		{ &GMCommandWorker::executeReceiveDamageCommand								} },
	{ "xp",					{ &GMCommandWorker::executeXPCommand										} },
	{ "transport",			{ &GMCommandWorker::executeTransportCommand,		gTransportCommandTable	} },
	{ "moveto",				{ &GMCommandWorker::executeMoveToCommand,									} },
    { "reload",             { nullptr,											gReloadCommandTable		} },
	{ "loglevel",			{ &GMCommandWorker::executeLogLevelCommand,									} },
	{ "money",				{ &GMCommandWorker::executeMoneyCommand,									} },
	{ "proficiency",		{ &GMCommandWorker::executeProficiencyCommand,								} },
	{ "tileflag",			{ &GMCommandWorker::executeTileFlagCommand,									} },
};

GMCommandWorker::GMCommandWorker(WorldSession* session): 
	m_session(session)
{}

bool GMCommandWorker::parseCommand(std::string const& line)
{
	ArgList args;
	std::string token;
	std::istringstream input(line);
	while (std::getline(input, token, ' ')) {
		if (!token.empty())
			args.emplace_back(token);
	}

	if (args.empty())
		return false;

	FlashMessage flashMsg;

	std::string error;
	ExecutionResult result = this->executeCommandInTable(args, gCommandRootTable, error);
	switch (result)
	{
	case RESULT_CMD_NOT_FOUND:
		flashMsg.set_severity(FlashMessage::ALERT);
		flashMsg.set_message(StringUtil::format("\"%s\" command not found", line.c_str()));
        NS_LOG_INFO("commands.gm", "\"%s\" command not found.", line.c_str());
		break;
	case RESULT_EXECUTION_FAILED:
		flashMsg.set_severity(FlashMessage::ALERT);
		flashMsg.set_message(StringUtil::format("\"%s\" command execution failed", line.c_str()));
		if (error.empty())
			error = "Unknown.";
        NS_LOG_INFO("commands.gm", "\"%s\" command execution failed. error: %s", line.c_str(), error.c_str());
		break;
	default: // RESULT_EXECUTION_SUCCESS
		flashMsg.set_severity(FlashMessage::INFO);
		flashMsg.set_message(line);
        NS_LOG_INFO("commands.gm", "%s", line.c_str());
		break;
	}

	m_session->sendFlashMessage(flashMsg);

	return result == RESULT_EXECUTION_SUCCESS;
}

GMCommandWorker::ExecutionResult GMCommandWorker::executeCommandInTable(ArgList& args, boost::container::map<std::string, GMCommandHolder> const& table, std::string& error)
{
	auto argIt = args.begin();
	if (argIt != args.end())
	{
		auto tableIt = table.find(*argIt);
		if (tableIt != table.end())
		{
			args.erase(argIt); // 保留参数部分

			auto const& subCmds = (*tableIt).second.subCommands;
			if (!subCmds.empty())
			{
				ExecutionResult result = this->executeCommandInTable(args, subCmds, error);
				if (result != RESULT_CMD_NOT_FOUND)
					return result;
			}

			CallbackPtr callback = ((*tableIt).second).callback;
			if (callback && (this->*callback)(args, error))
				return RESULT_EXECUTION_SUCCESS;
			else
				return RESULT_EXECUTION_FAILED;

		}
		else
			return RESULT_CMD_NOT_FOUND;
	}
	else
		return RESULT_CMD_NOT_FOUND;
}

// GM开关。
// Syntax: gm <on|off>
bool GMCommandWorker::executeGMCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	std::string arg = this->takeOutArg(args);
	if (arg == "on")
	{
		if (!owner->getData()->isGM())
		{
			owner->getData()->setGM(true);

			owner->removeAllAttackers();
			owner->combatStop();
			owner->pickupStop();
			owner->unlockStop();
			owner->releaseDangerState();

			owner->updateVisibilityForPlayer();
			owner->updateObjectVisibility();
			owner->updateTraceabilityForPlayer();
			owner->updateObjectTraceability();
			owner->updateObjectSafety();
		}
	}
	else if (arg == "off")
	{
		if (owner->getData()->isGM())
		{
			owner->getData()->setGM(false);

			owner->updateVisibilityForPlayer();
			owner->updateObjectVisibility();
			owner->updateTraceabilityForPlayer();
			owner->updateObjectTraceability();
			owner->updateObjectSafety();
		}
	}
	else
	{
		return false;
	}

	return true;
}

// 用指定的参数查找执行的目标，如果未找到则返回NULL。
Unit* GMCommandWorker::getExecutionTarget(Player* sender, ArgList& args, std::string& error)
{
	std::string targetType = takeOutArg(args);
	Unit* target = nullptr;

	if (targetType.empty())
		target = sender;
	else
	{
		std::string spawnIdStr = takeOutArg(args);
		uint32 spawnId = (uint32)std::stoul(spawnIdStr);
		ObjectGuid guid;
		if (targetType == "player")
			guid = ObjectGuid(GuidType::GUIDTYPE_PLAYER, spawnId);
		else if (targetType == "robot")
			guid = ObjectGuid(GuidType::GUIDTYPE_ROBOT, spawnId);

		if (guid != ObjectGuid::EMPTY)
		{
			target = ObjectAccessor::getUnit(sender, guid);
			if (!target)
				error = StringUtil::format("Target %s not found.", guid.toString().c_str());
		}
		else
			error = StringUtil::format("Target type \"%s\" undefined.", targetType.c_str());

	}

	return target;
}

std::string GMCommandWorker::takeOutArg(ArgList& args)
{
	std::string arg = "";
	auto firstIt = args.begin();
	if (firstIt != args.end())
	{
		arg = *firstIt;
		args.erase(firstIt);
	}
	return arg;
}

bool GMCommandWorker::addRobot(Player* owner, uint32 templateId, uint16 combatPower, std::string& error)
{
	RobotTemplate const* robotTmpl = sObjectMgr->getRobotTemplate(templateId);
	if (!robotTmpl)
	{
		error = StringUtil::format("Robot template (id=%d) not found.", templateId);
		return false;
	}

	combatPower = std::max(owner->getMap()->getCombatGrade().minCombatPower, std::min(owner->getMap()->getCombatGrade().maxCombatPower, combatPower));

	MapData* mapData = owner->getMap()->getMapData();
	TileCoord currCoord(mapData->getMapSize(), owner->getData()->getPosition());
	SimpleRobot simpleRobot;
	simpleRobot.templateId = robotTmpl->id;
	simpleRobot.name = sObjectMgr->getNextRobotNameByCountry(owner->getData()->getCountry());
	simpleRobot.country = owner->getData()->getCountry();
	simpleRobot.spawnPoint = currCoord;
	simpleRobot.level = owner->getData()->getLevel();
	simpleRobot.stage = robotTmpl->findStageByCombatPower(combatPower, owner->getMap()->getCombatGrade().minCombatPower);
	simpleRobot.proficiencyLevel = PROFICIENCY_LEVEL_MIN;
	simpleRobot.natureType = NATURE_RASH;

	Robot* robot = owner->getMap()->takeReusableObject<Robot>();
	if (robot)
		robot->reloadData(simpleRobot, owner->getMap());
	else
	{
		robot = new Robot();
		robot->loadData(simpleRobot, owner->getMap());
	}
	owner->getMap()->addToMap(robot);

	return true;
}

bool GMCommandWorker::addItembox(Player* owner, uint32 templateId, std::string& error)
{
	ItemBoxTemplate const* tmpl = sObjectMgr->getItemBoxTemplate(templateId);
	if (!tmpl)
	{
		error = StringUtil::format("ItemBox template (id=%d) not found.", templateId);
		return false;
	}

	MapData* mapData = owner->getMap()->getMapData();
	TileCoord currCoord(mapData->getMapSize(), owner->getData()->getPosition());
	SimpleItemBox simpleItemBox;
	simpleItemBox.templateId = tmpl->id;
	simpleItemBox.spawnPoint = currCoord;
	simpleItemBox.direction = DataItemBox::LEFT_DOWN;

	owner->getMap()->setTileClosed(simpleItemBox.spawnPoint);

	ItemBox* itemBox = owner->getMap()->takeReusableObject<ItemBox>();
	if (itemBox)
		itemBox->reloadData(simpleItemBox, owner->getMap());
	else
	{
		itemBox = new ItemBox();
		itemBox->loadData(simpleItemBox, owner->getMap());
	}
	owner->getMap()->addToMap(itemBox);

	return true;
}

bool GMCommandWorker::addItem(Player* owner, uint32 templateId, int32 count, std::string& error)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(templateId);
	if (!tmpl)
	{
		error = StringUtil::format("Item template (id=%d) not found.", templateId);
		return false;
	}

	MapData* mapData = owner->getMap()->getMapData();
	TileCoord currCoord(mapData->getMapSize(), owner->getData()->getPosition());
	FloorItemPlace itemPlace(owner->getMap(), currCoord, FloorItemPlace::LEFT_DOWN);
	SimpleItem simpleItem;
	simpleItem.templateId = tmpl->id;
	simpleItem.count = count;
	simpleItem.holder = owner->getData()->getGuid();
	simpleItem.holderOrigin = owner->getData()->getPosition();
	simpleItem.launchCenter = owner->getData()->getLaunchCenter();
	simpleItem.launchRadiusInMap = owner->getData()->getLaunchRadiusInMap();
	simpleItem.spawnPoint = itemPlace.nextEmptyTile();
	simpleItem.dropDuration = ITEM_PARABOLA_DURATION;

	Item* item = owner->getMap()->takeReusableObject<Item>();
	if (item)
		item->reloadData(simpleItem, owner->getMap());
	else
	{
		item = new Item();
		item->loadData(simpleItem, owner->getMap());
	}
	owner->getMap()->addToMap(item);
	item->updateObjectVisibility(true);

	return true;
}

bool GMCommandWorker::addCarrieditem(Player* owner, uint32 templateId, int32 count, std::string& error)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(templateId);
	if (!tmpl)
	{
		error = StringUtil::format("CarriedItem template (id=%d) not found.", templateId);
		return false;
	}

	ItemDest dest;
	if (!owner->canStoreItem(tmpl, count, &dest) == PICKUP_STATUS_OK)
	{
		error = StringUtil::format("CarriedItem (templateid=%d) add failed.", templateId);
		return false;
	}

	owner->storeItem(tmpl, dest);
	owner->sendItemReceivedMessage(tmpl, dest.count);

	return true;
}

bool GMCommandWorker::delRobot(Player* owner, uint32 spawnId, std::string& error)
{
	ObjectGuid guid = ObjectGuid(GuidType::GUIDTYPE_ROBOT, spawnId);
	Unit* target = ObjectAccessor::getUnit(owner, guid);
	if (!target)
	{
		error = StringUtil::format("Target %s not found.", guid.toString().c_str());
		return false;
	}

	Robot* robot = target->asRobot();
	robot->cleanupBeforeDelete();
	owner->getMap()->removeFromMap(robot, false);

	return true;
}

bool GMCommandWorker::delItemBox(Player* owner, uint32 spawnId, std::string& error)
{
	ObjectGuid guid = ObjectGuid(GuidType::GUIDTYPE_ITEMBOX, spawnId);
	ItemBox* target = ObjectAccessor::getItemBox(owner, guid);
	if (!target)
	{
		error = StringUtil::format("Target %s not found.", guid.toString().c_str());
		return false;
	}

	target->cleanupBeforeDelete();
	owner->getMap()->removeFromMap(target, false);

	return true;
}

bool GMCommandWorker::delItem(Player* owner, uint32 spawnId, std::string& error)
{
	ObjectGuid guid = ObjectGuid(GuidType::GUIDTYPE_ITEM, spawnId);
	Item* target = ObjectAccessor::getItem(owner, guid);
	if (!target)
	{
		error = StringUtil::format("Target %s not found.", guid.toString().c_str());
		return false;
	}

	target->cleanupBeforeDelete();
	owner->getMap()->removeFromMap(target, false);

	return true;
}

bool GMCommandWorker::delCarrieditem(Player* owner, uint32 templateId, std::string& error)
{
	ItemTemplate const* tmpl = sObjectMgr->getItemTemplate(templateId);
	if (!tmpl)
	{
		error = StringUtil::format("CarriedItem template (id=%d) not found.", templateId);
		return false;
	}

	switch (tmpl->itemClass)
	{
	case ITEM_CLASS_MAGIC_BEAN:
		owner->modifyMagicBean(tmpl, -owner->getData()->getMagicBeanCount());
		break;
	case ITEM_CLASS_GOLD:
		owner->modifyMoney(-owner->getData()->getMoney());
		break;
	default:
		for (int32 i = 0; i < UNIT_SLOTS_COUNT; ++i)
		{
			CarriedItem* item = owner->getItem(i);
			if (item && item->getData()->getItemId() == templateId)
				owner->removeItem(item);
		}
		break;
	}

	return true;
}

bool GMCommandWorker::executeMoveSpeedCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string moveSpeedVal = takeOutArg(args);
		int32 moveSpeed = std::min(MAX_MOVE_SPEED, std::stoi(moveSpeedVal));
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			target->getData()->setBaseMoveSpeed(moveSpeed);
			target->normalizeMoveSpeed();
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();

	}		
	return false;
}

bool GMCommandWorker::executeLevelCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string levelVal = this->takeOutArg(args);
		int32 level = std::stoi(levelVal);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			uint8 newLevel = (uint8)std::max(LEVEL_MIN, std::min(level, LEVEL_MAX));
			if (target->getData()->getLevel() != newLevel)
			{
				if (newLevel < LEVEL_MAX)
					target->getData()->setNextLevelXP(assertNotNull(sObjectMgr->getUnitLevelXP(newLevel + 1))->experience);
				else
					target->getData()->setNextLevelXP(0);
				target->getData()->setLevel(newLevel);
			}
			target->getData()->setExperience(0);

			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}

	return false;
}

bool GMCommandWorker::executeAttackRangeCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string rangeVal = this->takeOutArg(args);
		float attackRange = std::min(MAX_ATTACK_RANGE, std::stof(rangeVal));
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			target->getData()->setBaseAttackRange(attackRange);
			target->normalizeAttackRange();
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeDamageCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string damageVal = this->takeOutArg(args);
		int32 damage = std::stoi(damageVal);
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			target->getData()->setBaseDamage(damage);
			target->normalizeDamage();
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}		
	return false;
}

bool GMCommandWorker::executeAddCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string type = this->takeOutArg(args);
		std::string templateIdVal = this->takeOutArg(args);
		uint32 templateId = (uint32)std::stoul(templateIdVal);
		if (type == "robot")
		{
			std::string combatPowerVal = this->takeOutArg(args);
			uint16 combatPower = 0;
			if (!combatPowerVal.empty())
				combatPower = (uint16)std::stoi(combatPowerVal);
			return this->addRobot(owner, templateId, combatPower, error);
		}
		else if (type == "itembox")
			return this->addItembox(owner, templateId, error);
		else if (type == "item")
		{
			std::string countVal = this->takeOutArg(args);
			int32 count = 1;
			if (!countVal.empty())
				count = std::max(1, std::stoi(countVal));
			return this->addItem(owner, templateId, count, error);
		}
		else if (type == "carrieditem")
		{
			std::string countVal = this->takeOutArg(args);
			int32 count = 1;
			if (!countVal.empty())
				count = std::max(1, std::stoi(countVal));
			return this->addCarrieditem(owner, templateId, count, error);
		}
		else
			error = StringUtil::format("WorldObject type \"%s\" undefined.", type.c_str());

	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeDelCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string type = this->takeOutArg(args);
		if (type == "robot")
		{
			std::string spawnIdVal = takeOutArg(args);
			uint32 spawnId = (uint32)std::stoul(spawnIdVal);
			return delRobot(owner, spawnId, error);
		}
		else if (type == "itembox")
		{
			std::string spawnIdVal = takeOutArg(args);
			uint32 spawnId = (uint32)std::stoul(spawnIdVal);
			return delItemBox(owner, spawnId, error);
		}
		else if (type == "item")
		{
			std::string spawnIdVal = takeOutArg(args);
			uint32 spawnId = (uint32)std::stoul(spawnIdVal);
			return delItem(owner, spawnId, error);
		}
		else if (type == "carrieditem")
		{
			std::string templateIdVal = this->takeOutArg(args);
			uint32 templateId = (uint32)std::stoul(templateIdVal);
			return delCarrieditem(owner, templateId, error);
		}
		else
			error = StringUtil::format("WorldObject type \"%s\" undefined.", type.c_str());

	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeDieCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			if (target->isAlive())
			{
				target->addDamageOfAwardee(owner, target->getData()->getMaxHealth());
				target->died(owner);
			}
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeStaminaCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string staminaVal = this->takeOutArg(args);
		int32 stamina = std::stoi(staminaVal);
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			target->getData()->setBaseMaxStamina(stamina);
			target->modifyMaxStamina(stamina);
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();

	}	
	return false;
}

bool GMCommandWorker::executeStaminaRegenRateCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string regenRateVal = this->takeOutArg(args);
		float regenRate = std::stof(regenRateVal);
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			target->getData()->setBaseStaminaRegenRate(regenRate);
			target->modifyStaminaRegenRate(regenRate);
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeStaminaChargeConsumesCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string chargeConsumesVal = this->takeOutArg(args);
		int32 chargeConsumes = std::stoi(chargeConsumesVal);
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			target->getData()->setBaseChargeConsumesStamina(chargeConsumes);
			target->modifyChargeConsumesStamina(chargeConsumes);
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeHealthCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string healthVal = this->takeOutArg(args);
		int32 health = std::stoi(healthVal);
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			target->getData()->setBaseMaxHealth(health);
			target->normalizeHealth();
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeHealthRegenRateCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string regenRateVal = this->takeOutArg(args);
		float regenRate = std::stof(regenRateVal);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			target->getData()->setBaseHealthRegenRate(regenRate);
			target->normalizeHealthRegenRate();
			return true;
		}
		
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeDefenseCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string defenseVal = this->takeOutArg(args);
		int32 defense = std::stoi(defenseVal);
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			target->getData()->setDefense(defense);
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeReceiveDamageCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string damageVal = this->takeOutArg(args);
		int32 damage = std::stoi(damageVal);
		Unit* target = this->getExecutionTarget(owner, args, error);

		if (target)
		{
			damage = owner->calcDefenseReducedDamage(target, damage);
			owner->dealDamage(target, damage);
			return true;
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeXPCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string xpVal = this->takeOutArg(args);
		int32 xp = std::stoi(xpVal);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			target->giveXP(xp);
			return true;
		}

	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeTransportCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
        std::string xVal = this->takeOutArg(args);
        std::string yVal = this->takeOutArg(args);
		int32 x = std::stoi(xVal);
		int32 y = std::stoi(yVal);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			MapData* mapData = target->getData()->getMapData();
			Point dest;
			TileCoord toCoord(x, y);
			if (!mapData->isValidTileCoord(toCoord))
				dest = owner->getData()->getPosition();
			else
				dest = toCoord.computePosition(mapData->getMapSize());
			target->transport(dest);
			return true;
		}

	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeTransportXYCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
        std::string xVal = this->takeOutArg(args);
        std::string yVal = this->takeOutArg(args);
		float x = std::stof(xVal);
		float y = std::stof(yVal);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			Point dest(x, y);
			target->transport(dest);
			return true;
		}

	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeMoveToCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string xVal = this->takeOutArg(args);
		std::string yVal = this->takeOutArg(args);
		int32 x = std::stoi(xVal);
		int32 y = std::stoi(yVal);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target && target->isType(TYPEMASK_ROBOT))
		{
			MapData* mapData = target->getData()->getMapData();
			Robot* robot = target->asRobot();
			TileCoord toCoord(x, y);
			if (!mapData->isValidTileCoord(toCoord))
				toCoord = TileCoord(mapData->getMapSize(), owner->getData()->getPosition());

			robot->setAI(ROBOTAI_TYPE_NONE);
			robot->getMotionMaster()->movePoint(toCoord);

			return true;
		}

	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeReloadConfigCommand(ArgList& args, std::string& error)
{
    std::string configError;
    if(!sConfigMgr->reload(configError))
    {
        error = StringUtil::format("Reload config failed. error: %s", configError.c_str());
        return false;
    }
    
    sWorld->loadConfigs();
    
    return true;
}

bool GMCommandWorker::executeLogLevelCommand(ArgList& args, std::string& error)
{
	try
	{
		std::string name = this->takeOutArg(args);
		size_t found = name.find('.');
		if (found != std::string::npos)
		{
			std::string loggerOrAppender = name.substr(0, found);
			if (!loggerOrAppender.empty())
			{
				name = name.substr(found + 1);
				if (!name.empty())
				{
					std::string levelVal = this->takeOutArg(args);
					int32 level = std::stoi(levelVal);
					bool success = sLog->setLogLevel(name, level, loggerOrAppender == "Logger");
					return success;
				}
			}
		}
	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}


bool GMCommandWorker::executeMoneyCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string amountVal = this->takeOutArg(args);
		int32 amount = std::stoi(amountVal);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			int32 realAmount = owner->modifyMoney(amount);
			if(realAmount)
				owner->sendRewardMoneyMessage(realAmount);
			return true;
		}

	}
	catch (std::exception& e)
	{
		error = e.what();
	}
	return false;
}

bool GMCommandWorker::executeProficiencyCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string proficiencyVal = takeOutArg(args);
		uint8 proficiency = std::min(std::max(PROFICIENCY_LEVEL_MIN, std::stoi(proficiencyVal)), PROFICIENCY_LEVEL_MAX);
		Unit* target = this->getExecutionTarget(owner, args, error);
		if (target)
		{
			if (Robot* robot = target->asRobot())
			{
				auto const& grade = owner->getMap()->getCombatGrade();
				robot->getData()->setProficiencyLevel(proficiency);
				return true;
			}
		}
	}
	catch (std::exception& e)
	{
		error = e.what();

	}
	return false;
}
bool GMCommandWorker::executeTileFlagCommand(ArgList& args, std::string& error)
{
	Player* owner = m_session->getPlayer();
	if (!owner)
		return false;

	try
	{
		std::string xVal = this->takeOutArg(args);
		std::string yVal = this->takeOutArg(args);
		TileCoord coord;
		coord.x = std::stoi(xVal);
		coord.y = std::stoi(yVal);
		std::string flagVal = takeOutArg(args);
		bool isClosed = flagVal == "close";
		owner->getMap()->setTileClosed(coord, isClosed);
		return true;
	}
	catch (std::exception& e)
	{
		error = e.what();

	}
	return false;
}
