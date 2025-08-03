#ifndef __WORLD_H__
#define __WORLD_H__

#include "containers/Value.h"
#include "game/server/WorldSession.h"

enum WorldConfigs
{
	CONFIG_WITHDRAWAL_DELAY,
	CONFIG_CONCEALED_DELAY,
	CONFIG_ENTERING_DANGER_STATE_DELAY,
	CONFIG_DANGER_STATE_HEALTH_LOSS_INTERVAL,
	CONFIG_BATTLE_PREPARATION_DURATION,
	CONFIG_BATTLE_END_AD_CHANCE,
	CONFIG_BATTLE_ENABLE_APP_REVIEW_MODE,
};

class World
{
	typedef std::unordered_map<uint32, WorldSession*> SessionMap;

public:
	static World* instance();

	bool initWorld();
    
    void loadConfigs();

	bool getBoolConfig(int32 key);
	void setBoolConfig(int32 key, bool value);

	float getFloatConfig(int32 key);
	void setFloatConfig(int32 key, float value);

	int32 getIntConfig(int32 key);
	void setIntConfig(int32 key, int value);

	void stopNow();
	bool isStopped() const;
	void run();

	NSTime getUpdateDiff() const { return m_updateDiff; }

private:
	World();
	~World();

	void updateLoop();
	
	ValueMapIntKey m_configs;
	int32 m_worldUpdateInterval;
	NSTime m_updateDiff;
};

#define sWorld World::instance()

#endif // __WORLD_H__
