#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <boost/interprocess/ipc/message_queue.hpp>  
#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "Common.h"
#include "containers/Value.h"

using namespace boost::program_options;
using namespace boost::interprocess;

enum AppConfigs
{
	CONFIG_CONNECTION_TIMEOUT,
};

class Application
{
public:
	static Application* instance();

	int run(int argc, char** argv);
	// Get the root directory of the application
	// Format: /home/.../{SERVER_APP_DIR}/
	std::string const& getRoot() const { return m_appRoot; }

	bool getBoolConfig(int32 key);
	void setBoolConfig(int32 key, bool value);

	float getFloatConfig(int32 key);
	void setFloatConfig(int32 key, float value);

	int32 getIntConfig(int32 key);
	void setIntConfig(int32 key, int value);

private:
	Application();
	~Application();

	void loadConfigs();

	void shutdownThreadPool();
	void signalHandler(const boost::system::error_code& error, int signalNumber);

	bool parseCommandLine(int argc, char** argv, std::string& configFile, variables_map& options);

	void sendOptionsToInstance(uint32 pid, variables_map const& options);
	void receiveIPCMsg(uint32 oldPID, uint32 newPID);
	bool sendIPCMsg(message_queue* msgQueue, std::string const& msg);
	void handleIPCMsg(std::string const& msg);

	boost::asio::io_service m_ioService;
	std::string m_appRoot;
	std::vector<std::thread> m_threadPool;
	message_queue* m_msgQueue;

	ValueMapIntKey m_configs;
};

#define  sApplication Application::instance()

#endif //__APPLICATION_H__
