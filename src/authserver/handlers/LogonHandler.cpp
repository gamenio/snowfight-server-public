#include "server/AuthSession.h"

#include "server/protocol/pb/LogonChallenge.pb.h"
#include "server/protocol/pb/LogonResult.pb.h"
#include "server/protocol/pb/RealmList.pb.h"
#include "realm/RealmManager.h"
#include "realm/GeoIPManager.h"
#include "banned/BannedManager.h"
#include "Application.h"

void AuthSession::handleLogonChallenge(AuthPacket& recvPacket)
{
	LogonChallenge message;
	recvPacket.unpack(message);

	uint32 version = message.version();
	uint16 build = static_cast<uint16>(message.build());
	std::string deviceModel = message.device_model();
	std::string os = message.os();
	std::string lang = message.lang();
	std::string country = message.country();
	int32 timezone = message.timezone();
	uint8 platform = static_cast<uint8>(message.platform());
	std::string playerId = message.playerid();
	std::string originalPlayerId = message.original_playerid();
	NetworkType networkType = static_cast<NetworkType>(message.network_type());
	uint32 channelId =  message.channel_id();

	this->setRequiredCapabilities(message.required_capabilities());

	GeoIPLookupResult result;
	if (sApplication->getBoolConfig(CONFIG_REGION_MAPPING_WITH_GEOIP))
	{
		if (!this->isRequiresCapability(REQUIRES_DISABLE_REGION_MAPPING_WITH_GEOIP))
			sGeoIPManager->lookupSockaddr(m_socket->getRemoteEndpoint().data(), result);
	}

	if(result.countryCode.empty())
		result.countryCode = country;

	NS_LOG_INFO("auth.handler.logon", "Logon challenge from client %s:%d, playerid: %s, original playerid: %s, version: %x.%x.%x(Build %d), device model: %s, os: %s, lang: %s, country: %s (Name: %s), timezone: UTC%+03d:%02d, platform: %d, network type: %s, channelid: %d",
		m_remoteAddress.c_str(),
		m_remotePort,
		playerId.c_str(),
		originalPlayerId.c_str(),
		((version >> 16) & 0x000000FF), ((version >> 8) & 0x000000FF), (version & 0x000000FF), build,
		deviceModel.c_str(),
		os.c_str(),
		lang.c_str(),
		result.countryCode.c_str(), result.countryName.empty() ? "N/A" : result.countryName.c_str(),
		timezone / 3600, std::abs(timezone % 3600 / 60),
		platform,
		this->getNetworkTypeName(networkType).c_str(),
		channelId
	);

	if (sBannedManager->isIPBanned(m_remoteAddress))
	{
		NS_LOG_INFO("auth.handler.logon", "%s:%d has be banned.", m_remoteAddress.c_str(), m_remotePort);
		this->sendLogonResult(LogonResult::AUTH_FAILED, LogonResult::ERROR_NONE);
		return;
	}

	if (sBannedManager->isAccountBanned(playerId))
	{
		NS_LOG_INFO("auth.handler.logon", "Account '%s' from client %s:%d has be banned.", playerId.c_str(), m_remoteAddress.c_str(), m_remotePort);
		this->sendLogonResult(LogonResult::AUTH_FAILED, LogonResult::ERROR_NONE);
		return;
	}

	if(version >= sRealmManager->getMinRequiredVersion(result.countryCode))
	{
		std::list<RealmInfo> realmList;
		sRealmManager->getRealmList(result.countryCode, version, platform, realmList);
		if(realmList.empty())
			this->sendLogonResult(LogonResult::AUTH_FAILED, LogonResult::ERROR_LOW_CLIENT_VERSION);
		else
		{
			this->setGameVersion(version);
			this->setRealmList(realmList);
			this->setPlatform(platform);
			this->sendLogonResult(LogonResult::AUTH_OK);
		}
	}
	else
	{
		this->sendLogonResult(LogonResult::AUTH_FAILED, LogonResult::ERROR_LOW_CLIENT_VERSION);
	}
}

void AuthSession::sendLogonResult(LogonResult::AuthResult result, LogonResult::ErrorCode errorCode)
{
	LogonResult message;
	message.set_result(result);
	if(errorCode != LogonResult::ERROR_NONE)
		message.set_error_code(errorCode);

	AuthPacket packet(SMSG_LOGON_RESULT);
	this->packAndSend(std::move(packet), message);

}

void AuthSession::handleGetRealmList(AuthPacket& recvPacket)
{
	this->sendRealmList();
}

void AuthSession::sendRealmList()
{
	RealmList realmList;
	std::list<RealmInfo> const& realms = this->getRealmList();
	for (auto it = realms.begin(); it != realms.end(); ++it)
	{
		RealmInfo const& info = *it;
		Realm* realm = realmList.add_result();
		realm->set_address(info.address); // 如果地址为"0.0.0.0"，客户端将指向验证服务器地址
		realm->set_port(info.port);
		realm->set_name(info.name);
		//realm->set_flag();
		//realm->set_timezone();
		//realm->set_population_level();
	}

	AuthPacket packet(SMSG_REALM_LIST);
	this->packAndSend(std::move(packet), realmList);

}