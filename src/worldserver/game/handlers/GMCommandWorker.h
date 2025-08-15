#ifndef __GM_COMMAND_WORKER_H__
#define __GM_COMMAND_WORKER_H__

#include <boost/container/map.hpp>

#include "game/server/WorldSession.h"
#include "game/behaviors/Player.h"

struct GMCommandHolder;
class GMCommandWorker;

typedef std::vector<std::string> ArgList;
typedef bool(GMCommandWorker::*CallbackPtr)(ArgList&, std::string&);

class GMCommandWorker
{
	enum ExecutionResult
	{
		RESULT_EXECUTION_SUCCESS,
		RESULT_EXECUTION_FAILED,
		RESULT_CMD_NOT_FOUND,
	};

public:
	GMCommandWorker(WorldSession* session);

	bool parseCommand(std::string const& line);
	bool executeGMCommand(ArgList& args, std::string& error);

	//
	// Modify moving speed.
	// Syntax: movespeed <value> [player|robot <spawn_id>]
	// Options: 
	//		value: 1-150
	//
	bool executeMoveSpeedCommand(ArgList& args, std::string& error);

	//
	// Modify character level.
	// Syntax: level <value> [player|robot <spawn_id>]
	// Options:  
	//		value: 1-100
	//
	bool executeLevelCommand(ArgList& args, std::string& error);

	//
	// Modify attack range.
	// Syntax: attackrange <value> [player|robot <spawn_id>]
	// Options:  
	//		value: 1-343
	//
	bool executeAttackRangeCommand(ArgList& args, std::string& error);

	//
	// Modify damage.
	// Syntax: damage <value> [player|robot <spawn_id>]
	//
	bool executeDamageCommand(ArgList& args, std::string& error);

	//
	// Modify stamina.
	// Syntax: stamina <value> [player|robot <spawn_id>]
	// Options:  
	//		value: -1 Infinite stamina.
	//
	bool executeStaminaCommand(ArgList& args, std::string& error);

	//
	// Modify stamina regeneration rate.
	// Syntax: stamina regenrate <value> [player|robot <spawn_id>]
	//
	bool executeStaminaRegenRateCommand(ArgList& args, std::string& error);

	//
	// Modify the stamina consumed per second by charged attack.
	// Syntax: stamina chargeconsumes <value> [player|robot <spawn_id>]
	//
	bool executeStaminaChargeConsumesCommand(ArgList& args, std::string& error);

	//
	// Modify health.
	// Syntax: health <value> [player|robot <spawn_id>]
	//
	bool executeHealthCommand(ArgList& args, std::string& error);

	//
	// Modify health regeneration rate.
	// Syntax: health regenrate <value> [player|robot <spawn_id>]
	//
	bool executeHealthRegenRateCommand(ArgList& args, std::string& error);

	//
	// Modify defense.
	// Syntax: defense <value> [player|robot <spawn_id>]
	//
	bool executeDefenseCommand(ArgList& args, std::string& error);

	//
	// The unit receives damage.
	// Syntax: receivedamage <value> [player|robot <spawn_id>]
	//
	bool executeReceiveDamageCommand(ArgList& args, std::string& error);

	//
	// Modify experience.
	// Syntax: xp <value> [player|robot <spawn_id>]
	//
	bool executeXPCommand(ArgList& args, std::string& error);

	//
	// Transport the unit to the map tile coordinates.
	// Syntax: transport <x> <y> [player|robot <spawn_id>]
	// Options:  
	//		x: The X coordinate in tiles.
	//		y: The Y coordinate in tiles.
	//
	bool executeTransportCommand(ArgList& args, std::string& error);

	//
	// Transport the unit to map coordinates.
	// Syntax: transport xy <x> <y> [player|robot <spawn_id>]
	// Options:  
	//		x: X coordinate.
	//		y: Y coordinate.
	//
	bool executeTransportXYCommand(ArgList& args, std::string& error);

	//
	// Move the robot to the map tile coordinates.
	// Syntax: moveto <x> <y> [robot <spawn_id>]
	// Options:  
	//		x: The X coordinate in tiles.
	//		y: The Y coordinate in tiles.
	//
	bool executeMoveToCommand(ArgList& args, std::string& error);

	//
	// Add object.
	// Syntax: add robot <template_id> [combat_power]
	//		add itembox <template_id>
	//		add item <template_id> [item_count]
	//		add carrieditem <template_id> [item_count]
	// Options:  
	//		combat_power: 1-1000 Limited by the combat power range of the theater.
	//
	bool executeAddCommand(ArgList& args, std::string& error);

	//
	// Delete object.
	// Syntax: del <robot|itembox|item> <spawn_id>
	//		del carrieditem <template_id>
	//
	bool executeDelCommand(ArgList& args, std::string& error);

	//
	// Kill the unit.
	// Syntax: die <player|robot> <spawn_id>
	//
	bool executeDieCommand(ArgList& args, std::string& error);
    
    //
    // Reload the server configuration file.
    // Syntax:  reload config
    //
    bool executeReloadConfigCommand(ArgList& args, std::string& error);

	//
	// Modify log level.
	// Syntax:  loglevel <name> <level>
	// Options:  
	//		name: Appender.<filter>
	//			  Logger.<filter>
	//		level:
	//			  0 - (Disabled)
	//			  1 - (Trace)
	//			  2 - (Debug)
	//			  3 - (Info)
	//			  4 - (Warn)
	//			  5 - (Error)
	//			  6 - (Fatal)
	//
	bool executeLogLevelCommand(ArgList& args, std::string& error);

	//
	// Modify the amount of money.
	// Syntax: money <value> [player <spawn_id>]
	//
	bool executeMoneyCommand(ArgList& args, std::string& error);

	//
	// Modify robot proficiency level.
	// Syntax: proficiency <value> robot <spawn_id>
	// Options:  
	//		value: 1-6
	//
	bool executeProficiencyCommand(ArgList& args, std::string& error);

	//
	// Modify tile flag.
	// Syntax: tileflag <x> <y> <close|open>
	//
	bool executeTileFlagCommand(ArgList& args, std::string& error);

private:
	ExecutionResult executeCommandInTable(ArgList& args, boost::container::map<std::string, GMCommandHolder> const& table, std::string& error);
	Unit* getExecutionTarget(Player* sender, ArgList& args, std::string& error);
	std::string takeOutArg(ArgList& args);

	bool addRobot(Player* owner, uint32 templateId, uint16 combatPower, std::string& error);
	bool addItembox(Player* owner, uint32 templateId, std::string& error);
	bool addItem(Player* owner, uint32 templateId, int32 count, std::string& error);
	bool addCarrieditem(Player* owner, uint32 templateId, int32 count, std::string& error);

	bool delRobot(Player* owner, uint32 spawnId, std::string& error);
	bool delItemBox(Player* owner, uint32 spawnId, std::string& error);
	bool delItem(Player* owner, uint32 spawnId, std::string& error);
	bool delCarrieditem(Player* owner, uint32 templateId, std::string& error);

	WorldSession* m_session;
};

#endif // __GM_COMMAND_WORKER_H__
