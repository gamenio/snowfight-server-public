#ifndef __ROBOT_STAMINA_UPDATER_H__
#define __ROBOT_STAMINA_UPDATER_H__

#include "Common.h"
#include "utilities/Timer.h"
#include "game/entities/StaminaInfo.h"

enum ChargeState
{
	CHARGE_STATE_NONE,
	CHARGE_STATE_CHARGING,
	CHARGE_STATE_FULLY,
};

class Robot;

class RobotStaminaUpdater
{
public:
	RobotStaminaUpdater(Robot* owner);
	~RobotStaminaUpdater();

	void update(NSTime diff);
	void stop();

	void charge();
	void chargeUpdate();
	void chargeStop();
	bool isInCharge() const { return m_chargeState != CHARGE_STATE_NONE; }
	ChargeState getChargeState() const { return m_chargeState; }

	void startRegenStamina();

	bool canAttack() const;
	void deductStaminaForAttack();

private:
	void updateStamina(NSTime diff);

	void stopRegenStamina();
	void stopChargeProgress();

	void sendChargeStart();
	void sendChargeStop();
	void sendSyncStamina(uint32 flags = 0);

	Robot* m_owner;

	int32 m_startStamina;
	int32 m_diffStamina;
	DelayTimer m_staminaTimer;
	ChargeState m_chargeState;

	IntervalTimer m_staminaSyncTimer;
};

#endif // __ROBOT_STAMINA_UPDATER_H__