#include "game/server/WorldSession.h"

#include "game/server/protocol/pb/AttackInfo.pb.h"

#include "game/world/ObjectAccessor.h"
#include "game/server/protocol/OpcodeHandler.h"
#include "game/behaviors/Unit.h"
#include "game/behaviors/Player.h"
#include "game/behaviors/Projectile.h"
#include "game/grids/ObjectSearcher.h"

void broadcastStaminaWorker(Player* source, uint16 opcode, StaminaInfo& info)
{
	info.flags = source->getData()->getStaminaFlags();
	info.chargeStartStamina = source->getData()->getChargeStartStamina();

	// Send an operation acknowlegement packet to the sender.
	if (WorldSession* session = source->getSession())
	{
		WorldPacket packet(opcode);
		info.time = session->getClientNowTimeMillis();
		info.attackCounter = source->getData()->getAttackCounter();
		info.consumedStaminaTotal = source->getData()->getConsumedStaminaTotal();
		session->packAndSend(std::move(packet), info);
	}

	// Send the charge state and changes in stamina during the charge period to nearby players
	if (source->hasUnitState(UNIT_STATE_CHARGING) || opcode == MSG_CHARGE_START || opcode == MSG_CHARGE_STOP)
	{
		info.stamina = source->getData()->getStamina();
		info.chargedStamina = source->getData()->getChargedStamina();
		info.chargeConsumesStamina = source->getData()->getChargeConsumesStamina();
		info.attackCounter = 0;
		info.consumedStaminaTotal = 0;

		PlayerClientExistsObjectFilter filter(source);
		std::list<Player*> result;
		ObjectSearcher<Player, PlayerClientExistsObjectFilter> searcher(filter, result);

		source->visitNearbyObjectsInMaxVisibleRange(searcher);

		for (auto it = result.begin(); it != result.end(); ++it)
		{
			Player* target = *it;
			if (WorldSession* session = target->getSession())
			{
				WorldPacket packet(opcode);
				info.time = session->getClientNowTimeMillis();
				session->packAndSend(std::move(packet), info);
			}
		}
	}
}

void WorldSession::handleAttackInfo(WorldPacket& recvPacket)
{
	Player* attacker = this->getPlayer();
	if (!attacker->isInWorld() || !attacker->isAlive() || attacker->getMap()->isBattleEnded())
		return;

	AttackInfo info;
	recvPacket.unpack(info);

	if (attacker->getData()->getMovementCounter() != info.movement_counter())
	{
		NS_LOG_WARN("world.handler.combat", "Player(%s)'s attack is blocked because movement counter is wrong.(%d != %d)", attacker->getData()->getGuid().toString().c_str(), attacker->getData()->getMovementCounter(), info.movement_counter());
		return;
	}

	//NS_LOG_DEBUG("world.handler.combat", "handleAttackInfo attacker: %s flags: 0x%08X stamina: %d", getPlayer()->getData()->getGuid().toString().c_str(), info.flags(), attacker->getData()->getStamina());
	
	attacker->stopContinuousAttack();
	if (!attacker->hasUnitState(UNIT_STATE_CHARGING))
	{
		// If stamina is not enough, the attack will fail
		if (attacker->getData()->getStamina() < attacker->getData()->getAttackTakesStamina())
		{
			NS_LOG_DEBUG("world.handler.combat", "handleAttackInfo attacker: %s Not enough stamina.", attacker->getData()->getGuid().toString().c_str());
			attacker->sendLaunchResult(info.counter(), LAUNCHSTATUS_FAILED);
			return;
		}
	}

	attacker->getData()->setAttackInfoCounter(info.counter());
	attacker->addUnitState(UNIT_STATE_IN_COMBAT);
	attacker->attack(info);
}

void WorldSession::handleStaminaInfo(WorldPacket& recvPacket)
{
	Player* owner = this->getPlayer();
	if (!owner->isInWorld() || !owner->isAlive() || owner->getMap()->isBattleEnded())
		return;

	StaminaInfo info;
	recvPacket.unpack(info);

	uint16 opcode = recvPacket.getOpcode();
	int32 newStamina = info.stamina;
	uint32 newFlags = info.flags;
	//NS_LOG_DEBUG("world.handler.combat", "handleStaminaInfo owner: %s opcode: %s staminaCounter: %d/%d charging: %d attackCounter: %d/%d consumedStaminaTotal: %d/%d flags: 0x%08X stamina: %d->%d", owner->getData()->getGuid().toString().c_str(), getOpcodeNameForLogging(opcode).c_str(), info.counter, owner->getData()->getStaminaCounter(), owner->hasUnitState(UNIT_STATE_CHARGING), info.attackCounter, owner->getData()->getAttackCounter(), info.consumedStaminaTotal, owner->getData()->getConsumedStaminaTotal(), info.flags, owner->getData()->getStamina(), info.stamina);

	if (info.counter != owner->getData()->getStaminaCounter())
	{
		NS_LOG_WARN("world.handler.combat", "Player(%s)'s stamina counter is wrong. (%d != %d) opcode: %s", owner->getData()->getGuid().toString().c_str(), owner->getData()->getStaminaCounter(), info.counter, getOpcodeNameForLogging(opcode).c_str());
		return;
	}

	if (info.attackCounter < owner->getData()->getAttackCounter())
		newStamina = newStamina - (int32)(owner->getData()->getConsumedStaminaTotal() - info.consumedStaminaTotal);

	if (!owner->hasUnitState(UNIT_STATE_CHARGING))
	{
		int32 staminaDiff = newStamina - owner->getData()->getStamina();
		int32 maxPoints = (int32)std::ceil(STAMINA_SYNC_INTERVAL * (owner->getData()->getStaminaRegenRate() * owner->getData()->getMaxStamina()));
		if (staminaDiff < 0 || staminaDiff > maxPoints)
		{
			NS_LOG_WARN("world.handler.combat", "Player(%s)'s stamina points regenerated (%d) is out of range (0-%d).", owner->getData()->getGuid().toString().c_str(), staminaDiff, maxPoints);
			this->kickPlayer();
			return;
		}

		if (opcode == MSG_CHARGE_START)
		{
			if (owner->hasItemEffectType(ITEM_EFFECT_CHARGED_ATTACK_ENABLED))
				owner->getData()->setChargeStartStamina(newStamina);
			else
			{
				NS_LOG_WARN("world.handler.combat", "Player(%s) hasn't enabled charged attack.", owner->getData()->getGuid().toString().c_str());
				return;
			}
		}
	}
	else
	{
		int32 staminaDiff = owner->getData()->getStamina() - newStamina;
		int32 maxPoints = (int32)std::ceil(STAMINA_SYNC_INTERVAL * owner->getData()->getChargeConsumesStamina());
		if (staminaDiff < 0 || staminaDiff > maxPoints)
		{
			NS_LOG_WARN("world.handler.combat", "Player(%s)'s stamina points consumed (%d) is out of range (0-%d).", owner->getData()->getGuid().toString().c_str(), staminaDiff, maxPoints);
			this->kickPlayer();
			return;
		}

		int32 chargedStamina = owner->getData()->getChargeStartStamina() - newStamina;
		owner->getData()->setChargedStamina(chargedStamina);
		//NS_LOG_DEBUG("world.handler.combat", "handleStaminaInfo chargedStamina: %d", chargedStamina);
	}

	owner->getData()->setStamina(newStamina);
	owner->getData()->setStaminaFlags(newFlags);

	broadcastStaminaWorker(owner, opcode, info);

	switch (opcode)
	{
	case MSG_CHARGE_START:
		owner->charge();
		break;
	case MSG_CHARGE_STOP:
		owner->chargeStop();
		break;
	}
}