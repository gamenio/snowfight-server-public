#ifndef __AUTH_SESSION_H__
#define __AUTH_SESSION_H__

#include "server/protocol/pb/LogonResult.pb.h"

#include "utilities/Timer.h"
#include "containers/ConcurrentQueue.h"
#include "realm/RealmInfo.h"
#include "AuthSocket.h"

enum Platform
{
	PLATFORM_UNKNOWN			=  0,
	PLATFORM_IOS				=  1,
	PLATFORM_ANDROID			=  2,
	PLATFORM_WIN32				=  3,
	PLATFORM_MAC				=  8,
};

enum NetworkType
{
	NETWORK_TYPE_UNKNOWN		= 0,
	NETWORK_TYPE_2G				= 1,
	NETWORK_TYPE_3G				= 2,
	NETWORK_TYPE_4G				= 3,
	NETWORK_TYPE_5G				= 4,
	NETWORK_TYPE_WIFI			= 8,
};


enum RequiredCapabilities
{
	// 禁止使用玩家IP地理位置进行区域映射
	// 如果被禁用则使用玩家上报的位置（LogonChallenge协议的country字段）
	REQUIRES_DISABLE_REGION_MAPPING_WITH_GEOIP = 0x00000001
};

class AuthSocket;

class AuthSession
{
public:
	explicit AuthSession(AuthSocket* socket);
	~AuthSession();

	void handlePacket(AuthPacket&& newPacket);
	void packAndSend(AuthPacket&& packet, MessageLite const& message);

	void setSessionId(uint32 id) { m_sessionId = id; }
	uint32 getSessionId() const { return m_sessionId; }

	void setGameVersion(uint32 version) { m_gameVersion = version; }
	uint32 getGameVersion() const { return m_gameVersion; }
	void setRealmList(std::list<RealmInfo> const& realmList) { m_realmList = realmList; }
	std::list<RealmInfo> const& getRealmList() const { return m_realmList;  }
	void setPlatform(uint8 platform) { m_platform = platform; }
	uint8 getPlatform() const { return m_platform; }
	// 玩家需要的功能
	void setRequiredCapabilities(uint32 capabilities) { m_requiredCapabilities = capabilities; }
	uint32 getRequiredCapabilities() const { return m_requiredCapabilities; }
	bool isRequiresCapability(uint32 capability) { return (m_requiredCapabilities & capability) != 0; }

	void kick();

	// LogonHandler
	void handleLogonChallenge(AuthPacket& recvPacket);
	void sendLogonResult(LogonResult::AuthResult result, LogonResult::ErrorCode errorCode = LogonResult::ERROR_NONE);
	void handleGetRealmList(AuthPacket& recvPacket);
	void sendRealmList();

private:
	std::string getNetworkTypeName(NetworkType type);

	uint32 m_sessionId;
	std::string m_remoteAddress;
	uint16 m_remotePort;

	AuthSocket* m_socket;

	uint32 m_gameVersion;
	std::list<RealmInfo> m_realmList;
	uint8 m_platform;
	uint32 m_requiredCapabilities;
};

#endif //__AUTH_SESSION_H__