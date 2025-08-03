#include "World.h"

#include "utilities/TimeUtil.h"
#include "configuration/Config.h"
#include "game/theater/TheaterManager.h"
#include "game/maps/MapDataManager.h"
#include "ObjectMgr.h"

#if PLATFORM == PLATFORM_WINDOWS
#include <Windows.h>
#include <timeapi.h>

#define TIMER_TARGET_RESOLUTION		1		// 计时器目标分辨率为1毫秒

#endif // PLATFORM_WINDOWS

World::World():
	m_worldUpdateInterval(0),
	m_updateDiff(0)
{

}

World::~World()
{

}


World* World::instance()
{
	static World instance;
	return &instance;
}

bool World::initWorld()
{
	srand((unsigned int)time(NULL));

	this->loadConfigs();

	if(!sMapDataManager->load()) 
		return false;

	sTheaterManager->initialize();

	if (!sObjectMgr->load())
		return false;

	return true;
}

void World::loadConfigs()
{
	m_worldUpdateInterval = sConfigMgr->getIntDefault("WorldUpdateInterval", 50);

	m_configs[CONFIG_WITHDRAWAL_DELAY] = sConfigMgr->getIntDefault("WithdrawalDelay", 2000);
	m_configs[CONFIG_CONCEALED_DELAY] = sConfigMgr->getFloatDefault("ConcealedDelay", 2000);

	m_configs[CONFIG_ENTERING_DANGER_STATE_DELAY] = sConfigMgr->getIntDefault("EnteringDangerStateDelay", 3000);
	m_configs[CONFIG_DANGER_STATE_HEALTH_LOSS_INTERVAL] = sConfigMgr->getIntDefault("DangerStateHealthLossInterval", 5000);

	m_configs[CONFIG_BATTLE_PREPARATION_DURATION] = sConfigMgr->getIntDefault("BattlePreparationDuration", 3000);
	m_configs[CONFIG_BATTLE_END_AD_CHANCE] = sConfigMgr->getFloatDefault("BattleEndAdChance", 100.0f);
	m_configs[CONFIG_BATTLE_ENABLE_APP_REVIEW_MODE] = sConfigMgr->getBoolDefault("EnableAppReviewMode", false);

}

bool World::getBoolConfig(int32 key)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		return (*it).second.asBool();
	return false;
}

void World::setBoolConfig(int32 key, bool value)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		(*it).second = value;
	else
		m_configs.emplace(key, value);
}

float World::getFloatConfig(int32 key)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		return (*it).second.asFloat();
	return 0.f;
}

void World::setFloatConfig(int32 key, float value)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		(*it).second = value;
	else
		m_configs.emplace(key, value);
}


int32 World::getIntConfig(int32 key)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		return (*it).second.asInt();
	return 0;
}

void World::setIntConfig(int32 key, int value)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		(*it).second = value;
	else
		m_configs.emplace(key, value);
}


void World::stopNow()
{
	sTheaterManager->stop();
}

bool World::isStopped() const
{
	return sTheaterManager->isStopped();
}

void World::run()
{
#if PLATFORM == PLATFORM_WINDOWS
	UINT wTimerRes = 0;

	TIMECAPS tc;
	if (TIMERR_NOERROR == timeGetDevCaps(&tc, sizeof(TIMECAPS)))
	{
		wTimerRes = std::min(std::max(tc.wPeriodMin, UINT(TIMER_TARGET_RESOLUTION)), tc.wPeriodMax);
		timeBeginPeriod(wTimerRes);
	}
#endif  // PLATFORM_WINDOWS
	this->updateLoop();

#if PLATFORM == PLATFORM_WINDOWS
	if (wTimerRes != 0)
		timeEndPeriod(wTimerRes);
#endif  // PLATFORM_WINDOWS

	sMapDataManager->unload();
}

void World::updateLoop()
{
	int64 realCurrTime = 0;
	int64 realPrevTime = getUptimeMillis();

	while (true)
	{
		realCurrTime = getUptimeMillis();

		m_updateDiff = static_cast<NSTime>(realCurrTime - realPrevTime);
		realPrevTime = realCurrTime;

		if (!sTheaterManager->update(m_updateDiff))
			break;

		// 平衡接近WORLD_SLEEP_CONST的Tick持续时间
		int64 executionTime = getUptimeMillis() - realPrevTime;
		if (executionTime < m_worldUpdateInterval)
		{
#if PLATFORM == PLATFORM_WINDOWS
			// Windows的计时器精度通过“timeBeginPeriod()”设置为最高（1ms），但仍然不够精确。 
			// 例如，如果定时器的精度为1ms，则Sleep(3)可能会休眠2ms或4ms。 
			// 因此，我们这里减去1ms，使休眠时间变短。 如果“sleepTime”等于或小于1ms，则不休眠并进入下一个循环，以将CPU准确地提升到下一帧。
			int64 sleepTime = m_worldUpdateInterval - executionTime - 1L;
			if (sleepTime > 1L)
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
#else
			int64 sleepTime = m_worldUpdateInterval - executionTime;
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
#endif  // PLATFORM_WINDOWS
		}
	}
}
