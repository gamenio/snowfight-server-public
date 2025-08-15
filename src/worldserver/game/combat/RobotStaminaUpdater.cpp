#include "RobotStaminaUpdater.h"

#include "logging/Log.h"
#include "game/behaviors/Robot.h"
#include "game/behaviors/Projectile.h"

#define STAMINA_SYNC_INTERVAL			200	// Stamina synchronization interval. Unit: milliseconds

RobotStaminaUpdater::RobotStaminaUpdater(Robot* owner) :
	m_owner(owner),
	m_startStamina(0),
	m_diffStamina(0),
	m_chargeState(CHARGE_STATE_NONE),
	m_staminaSyncTimer(true)
{
	m_staminaSyncTimer.setInterval(STAMINA_SYNC_INTERVAL);
}

RobotStaminaUpdater::~RobotStaminaUpdater()
{
	m_owner = nullptr;
}

void RobotStaminaUpdater::update(NSTime diff)
{
	if (m_diffStamina == 0)
		return;

	if (m_chargeState == CHARGE_STATE_NONE)
		this->updateStamina(diff);
	else
	{
		NSTime updateDt;
		NSTime elapsed = m_staminaSyncTimer.getCurrent() + diff;
		if (elapsed + m_staminaSyncTimer.getRemainder() >= m_staminaSyncTimer.getInterval())
			updateDt = m_staminaSyncTimer.getInterval() - m_staminaSyncTimer.getCurrent();
		else
			updateDt = diff;

		int32 currStamina = m_owner->getData()->getStamina();
		this->updateStamina(updateDt);
		int32 diffStamina = currStamina - m_owner->getData()->getStamina();;
		//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::update(diff=%d) current: %d/%d remainder: %d updateDt: %d staminaDiff: %d", diff, m_staminaSyncTimer.getCurrent(), m_staminaSyncTimer.getInterval(), m_staminaSyncTimer.getRemainder(), updateDt, diffStamina);

		// Synchronize stamina at regular intervals
		m_staminaSyncTimer.update(diff);
		if (m_staminaSyncTimer.passed())
		{
			this->sendSyncStamina();
			m_staminaSyncTimer.reset();
		}
	}
}

void RobotStaminaUpdater::stop()
{
	this->stopRegenStamina();
	this->stopChargeProgress();

	m_staminaSyncTimer.reset();
}

void RobotStaminaUpdater::charge()
{
	if (m_chargeState != CHARGE_STATE_NONE)
		return;

	this->stopRegenStamina();
	NS_ASSERT(m_diffStamina == 0);

	m_startStamina = m_owner->getData()->getStamina();
	m_diffStamina = -m_startStamina;
	int32 duration = (int32)(m_startStamina / (float)m_owner->getData()->getChargeConsumesStamina() * 1000);
	duration = std::max(0, duration);
	m_staminaTimer.setDuration(duration);

	m_owner->getData()->setChargeStartStamina(m_startStamina);
	m_owner->addUnitState(UNIT_STATE_CHARGING);
	m_owner->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_CHARGED_POWER);

	m_chargeState = CHARGE_STATE_CHARGING;
	m_owner->getData()->addStaminaFlag(STAMINA_FLAG_CHARGING);

	m_staminaSyncTimer.reset();
	this->sendChargeStart();

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::charge startStamina: %d diffStamina: %d duration: %d", m_startStamina, m_diffStamina, m_staminaTimer.getDuration());
}

void RobotStaminaUpdater::chargeUpdate()
{
	if (m_chargeState != CHARGE_STATE_CHARGING)
		return;

	m_startStamina = m_owner->getData()->getChargeStartStamina();
	m_diffStamina = -m_startStamina;
	int32 duration = (int32)(m_startStamina / (float)m_owner->getData()->getChargeConsumesStamina() * 1000);
	duration = std::max(0, duration);
	m_staminaTimer.setDuration(duration);
	if (m_owner->getData()->getChargedStamina() > 0)
	{
		int32 elapsed = (int32)(m_owner->getData()->getChargedStamina() / (float)m_owner->getData()->getChargeConsumesStamina() * 1000);
		m_staminaTimer.update(elapsed);
	}

	m_staminaSyncTimer.reset();

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::chargeUpdate startStamina: %d diffStamina: %d chargedStamina: %d duration: %d", m_startStamina, m_diffStamina, m_owner->getData()->getChargedStamina(), m_staminaTimer.getDuration());
}

void RobotStaminaUpdater::chargeStop()
{
	if (m_chargeState == CHARGE_STATE_NONE)
		return;

	m_owner->getData()->clearStaminaFlag(STAMINA_FLAG_CHARGING);

	m_staminaSyncTimer.reset();
	this->sendChargeStop();

	this->startRegenStamina();
}

void RobotStaminaUpdater::startRegenStamina()
{
	this->stopChargeProgress();
	this->stopRegenStamina();

	int32 stamina = m_owner->getData()->getStamina();
	int32 maxStamina = m_owner->getData()->getMaxStamina();
	if (stamina >= maxStamina)
		return;

	int32 diff = maxStamina - stamina;
	NSTime duration = (int32)(diff / (m_owner->getData()->getStaminaRegenRate() * maxStamina) * 1000.f);
	m_staminaTimer.setDuration(duration);
	m_diffStamina = diff;
	m_startStamina = stamina;

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::startRegenStamina currStamina: %d startStamina: %d diffStamina: %d duration: %d", stamina, m_startStamina, m_diffStamina, m_staminaTimer.getDuration());
}

bool RobotStaminaUpdater::canAttack() const
{
	if (m_chargeState == CHARGE_STATE_NONE)
	{
		if (m_owner->getData()->getStamina() >= m_owner->getData()->getAttackTakesStamina())
			return true;
	}
	else
	{
		if (m_chargeState == CHARGE_STATE_FULLY)
			return true;
	}

	return false;
}

void RobotStaminaUpdater::deductStaminaForAttack()
{
	if (m_chargeState != CHARGE_STATE_NONE)
	{
		m_owner->getData()->clearStaminaFlag(STAMINA_FLAG_CHARGING);
		this->sendSyncStamina(STAMINA_FLAG_ATTACK);
		m_staminaSyncTimer.reset();
	}

	// Calculate new stamina
	int32 consumedStamina = m_owner->calcConsumedStamina();
	int32 newStamina = m_owner->getData()->getStamina() - consumedStamina;
	NS_ASSERT(newStamina >= 0);
	m_owner->getData()->setStamina(newStamina);

	this->startRegenStamina();
}

void RobotStaminaUpdater::updateStamina(NSTime diff)
{
	m_staminaTimer.update(diff);
	float scale = std::min(1.0f, m_staminaTimer.getRemainder() / (float)m_staminaTimer.getDuration());
	int32 newStamina = m_startStamina + (int32)(m_diffStamina * (1.0f - scale));
	m_owner->getData()->setStamina(newStamina);

	if (m_chargeState == CHARGE_STATE_CHARGING)
	{
		int32 chargedStamina = m_startStamina - newStamina;
		m_owner->getData()->setChargedStamina(chargedStamina);

		//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::updateStamina chargedStamina: %d", chargedStamina);
	}

	if (m_staminaTimer.passed())
	{
		if (m_chargeState == CHARGE_STATE_CHARGING)
		{
			m_chargeState = CHARGE_STATE_FULLY;
			//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::updateStamina stamina: %d fully charged!", newStamina);
		}
		//else
		//	NS_LOG_DEBUG("misc", "RobotStaminaUpdater::updateStamina stamina: %d is full!", newStamina);


		m_diffStamina = 0;
		m_startStamina = 0;
		m_staminaTimer.reset();
		m_staminaSyncTimer.setPassed();
	}
	//else
	//	NS_LOG_DEBUG("misc", "RobotStaminaUpdater::updateStamina stamina: %d", newStamina);
}

void RobotStaminaUpdater::stopRegenStamina()
{
	if (m_diffStamina <= 0)
		return;

	NS_ASSERT(m_chargeState == CHARGE_STATE_NONE);

	m_diffStamina = 0;
	m_startStamina = 0;
	m_staminaTimer.reset();

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::stopRegenStamina()");
}

void RobotStaminaUpdater::stopChargeProgress()
{
	if (m_chargeState == CHARGE_STATE_NONE)
		return;

	NS_ASSERT(m_diffStamina <= 0);

	m_owner->getData()->setChargeStartStamina(0);
	m_owner->getData()->setChargedStamina(0);
	m_owner->clearUnitState(UNIT_STATE_CHARGING);
	m_owner->getData()->clearStaminaFlag(STAMINA_FLAG_CHARGING);
	m_owner->getUnitHostileRefManager()->updateThreat(UNITTHREAT_ENEMY_CHARGED_POWER);

	m_chargeState = CHARGE_STATE_NONE;

	m_diffStamina = 0;
	m_startStamina = 0;
	m_staminaTimer.reset();

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::stopChargeProgress()");
}

void RobotStaminaUpdater::sendChargeStart()
{
	StaminaInfo stamina = m_owner->getData()->getStaminaInfo();
	m_owner->sendStaminaOpcodeToNearbyPlayers(MSG_CHARGE_START, stamina);

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::sendChargeStart %s", stamina.description().c_str());
}

void RobotStaminaUpdater::sendChargeStop()
{
	StaminaInfo stamina = m_owner->getData()->getStaminaInfo();
	m_owner->sendStaminaOpcodeToNearbyPlayers(MSG_CHARGE_STOP, stamina);

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::sendChargeStop %s", stamina.description().c_str());
}

void RobotStaminaUpdater::sendSyncStamina(uint32 flags)
{
	StaminaInfo stamina = m_owner->getData()->getStaminaInfo();
	stamina.flags = flags;
	m_owner->sendStaminaOpcodeToNearbyPlayers(MSG_STAMINA_SYNC, stamina);

	//NS_LOG_DEBUG("misc", "RobotStaminaUpdater::sendSyncStamina %s", stamina.description().c_str());
}
