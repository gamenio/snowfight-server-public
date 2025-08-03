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
	// 修改移动速度。
	// 语法：movespeed <value> [player|robot <spawn_id>]
	// 选项： 
	//		value: 1-150
	//
	bool executeMoveSpeedCommand(ArgList& args, std::string& error);

	//
	// 修改角色等级。
	// 语法：level <value> [player|robot <spawn_id>]
	// 选项： 
	//		value: 1-100
	//
	bool executeLevelCommand(ArgList& args, std::string& error);

	//
	// 修改攻击范围
	// 语法：attackrange <value> [player|robot <spawn_id>]
	// 选项： 
	//		value: 1-343
	//
	bool executeAttackRangeCommand(ArgList& args, std::string& error);

	//
	// 修改攻击力。
	// 语法：damage <value> [player|robot <spawn_id>]
	//
	bool executeDamageCommand(ArgList& args, std::string& error);

	//
	// 修改体力。
	// 语法：stamina <value> [player|robot <spawn_id>]
	// 选项： 
	//		value: -1 无限体力
	//
	bool executeStaminaCommand(ArgList& args, std::string& error);

	//
	// 修改体力恢复速度。
	// 语法：stamina regenrate <value> [player|robot <spawn_id>]
	//
	bool executeStaminaRegenRateCommand(ArgList& args, std::string& error);

	//
	// 修改蓄力攻击每秒消耗的体力。
	// 语法：stamina chargeconsumes <value> [player|robot <spawn_id>]
	//
	bool executeStaminaChargeConsumesCommand(ArgList& args, std::string& error);

	//
	// 修改生命值。
	// 语法：health <value> [player|robot <spawn_id>]
	//
	bool executeHealthCommand(ArgList& args, std::string& error);

	//
	// 修改生命恢复速度。
	// 语法：health regenrate <value> [player|robot <spawn_id>]
	//
	bool executeHealthRegenRateCommand(ArgList& args, std::string& error);

	//
	// 修改防御力。
	// 语法：defense <value> [player|robot <spawn_id>]
	//
	bool executeDefenseCommand(ArgList& args, std::string& error);

	//
	// 单位受到伤害。
	// 语法：receivedamage <value> [player|robot <spawn_id>]
	//
	bool executeReceiveDamageCommand(ArgList& args, std::string& error);

	//
	// 修改经验值。
	// 语法：xp <value> [player|robot <spawn_id>]
	//
	bool executeXPCommand(ArgList& args, std::string& error);

	//
	// 传输单位到地图瓦片坐标。
	// 语法：transport <x> <y> [player|robot <spawn_id>]
	// 选项： 
	//		x: 瓦片的X坐标
	//		y: 瓦片的Y坐标
	//
	bool executeTransportCommand(ArgList& args, std::string& error);

	//
	// 传输单位到地图坐标。
	// 语法：transport xy <x> <y> [player|robot <spawn_id>]
	// 选项： 
	//		x: X坐标
	//		y: Y坐标
	//
	bool executeTransportXYCommand(ArgList& args, std::string& error);

	//
	// 移动机器人到地图瓦片坐标。
	// 语法：moveto <x> <y> [robot <spawn_id>]
	// 选项： 
	//		x: 瓦片的X坐标
	//		y: 瓦片的Y坐标
	//
	bool executeMoveToCommand(ArgList& args, std::string& error);

	//
	// 添加对象。
	// 语法：add robot <template_id> [combat_power]
	//		add itembox <template_id>
	//		add item <template_id> [item_count]
	//		add carrieditem <template_id> [item_count]
	// 选项： 
	//		combat_power: 1-1000 受限于战区的战斗力范围
	//
	bool executeAddCommand(ArgList& args, std::string& error);

	//
	// 删除对象。
	// 语法：del <robot|itembox|item> <spawn_id>
	//		del carrieditem <template_id>
	//
	bool executeDelCommand(ArgList& args, std::string& error);

	//
	// 杀死单位。
	// 语法：die <player|robot> <spawn_id>
	//
	bool executeDieCommand(ArgList& args, std::string& error);
    
    //
    // 重新加载服务器配置文件。
    // 语法： reload config
    //
    bool executeReloadConfigCommand(ArgList& args, std::string& error);

	//
	// 修改日志等级。
	// 语法： loglevel <name> <level>
	// 选项： 
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
	// 修改钱数。
	// 语法：money <value> [player <spawn_id>]
	//
	bool executeMoneyCommand(ArgList& args, std::string& error);

	//
	// 修改机器人熟练等级。
	// 语法：proficiency <value> robot <spawn_id>
	// 选项： 
	//		value: 1-6
	//
	bool executeProficiencyCommand(ArgList& args, std::string& error);

	//
	// 修改瓦片标记。
	// 语法：tileflag <x> <y> <close|open>
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
