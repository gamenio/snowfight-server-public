#include "Application.h"

#include <iostream>

#include "debugging/Errors.h"
#include "configuration/Config.h"
#include "threading/ProcessPriority.h"
#include "logging/Log.h"
#include "utilities/Util.h"
#include "server/NTSSocketMgr.h"

#define DEFAULT_CONFIG_FILE     "ntsserver.conf"
#define DEFAULT_LOG_DIR         "log"

// Build version number
#define BUILD_NUMBER                1

// Return EXIT_FAILURE after printing the error and releasing resource
#define RETURN_FAILURE(...) \
	do { \
		NS_LOG_ERROR("server.ntsserver", __VA_ARGS__); \
		this->shutdownThreadPool(); \
		return EXIT_FAILURE; \
	} while(0)


#define MAX_MSG_COUNT					15
#define MAX_MSG_SIZE					1024
#define MESSAGE_QUEUE_NAME_FORMAT		"ntsserver_msg_queue_%d"

Application::Application() :
	m_msgQueue(nullptr)
{
}


Application::~Application()
{
	if (m_msgQueue)
	{
		delete m_msgQueue;
		m_msgQueue = nullptr;
	}
}

void Application::loadConfigs()
{
	m_configs[CONFIG_CONNECTION_TIMEOUT] = sConfigMgr->getIntDefault("ConnectionTimeout", 10000);
}

Application* Application::instance()
{
	static Application instance;
	return &instance;
}

int Application::run(int argc, char** argv)
{
	signal(SIGABRT, &errors::abortHandler);

	m_appRoot = boost::filesystem::path(argv[0]).parent_path().generic_string();
	if (!m_appRoot.empty())
		m_appRoot += '/';

	std::string configFile = m_appRoot + DEFAULT_CONFIG_FILE;
	variables_map options;
	try
	{
		if(parseCommandLine(argc, argv, configFile, options))
			return EXIT_SUCCESS;
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}

	std::string configError;
	if (!sConfigMgr->loadInitial(configFile, configError))
	{
		printf("Error in config file: %s\n", configError.c_str());
		return EXIT_FAILURE;
	}

	// Get the PID of the previous instance
	uint32 pid = 0;
	std::string pidFile = sConfigMgr->getStringDefault("PidFile", "");
	if (!pidFile.empty())
		pid = getPIDFromFile(m_appRoot + pidFile);

	//  Send options to instance
	try
	{
		if (!options.empty())
		{
			this->sendOptionsToInstance(pid, options);
			return EXIT_SUCCESS;
		}
	}
	catch (interprocess_exception const& e)
	{
		printf("Failed to send options to instance (PID=%u). error: %s\n", pid, e.what());
		return EXIT_FAILURE;
	}


	// Check whether the instance is already running
	if (pid && isProcessRunning(pid))
	{
		printf("An instance (PID=%u) of server is already running.\n", pid);
		return EXIT_FAILURE;
	}

	// If you want to output logs asynchronously, you need to pass io_service to the Log singleton
	// Normally, logs are output in the thread that calls it, which may be the main thread
	std::string logPath = m_appRoot + DEFAULT_LOG_DIR + '/';
	sLog->initialize(sConfigMgr->getBoolDefault("Log.Async.Enable", false) ? &m_ioService : nullptr, logPath);

	uint16 ntsPort = uint16(sConfigMgr->getIntDefault("NTSServerPort", 18123));
	std::string ntsListener = sConfigMgr->getStringDefault("BindIP", "0.0.0.0");
	NS_LOG_INFO("server.ntsserver", "NTSServer-Daemon (Build %d)", BUILD_NUMBER);
	NS_LOG_INFO("server.ntsserver", "<Ctrl-C> to stop.\n");
	NS_LOG_INFO("server.ntsserver", "#===============================================#");
	NS_LOG_INFO("server.ntsserver", "#  ____ _  _ ____ _ _ _   ____ _ ____ _  _ ___  #");
	NS_LOG_INFO("server.ntsserver", "#  [__  |\\ | |  | | | |   |___ | | __ |__|  |   #");
	NS_LOG_INFO("server.ntsserver", "#  ___] | \\| |__| |_|_|   |    | |__] |  |  |   #");
	NS_LOG_INFO("server.ntsserver", "#                                               #");
	NS_LOG_INFO("server.ntsserver", "#     Network Time Synchronization (NTS) Server #");
	NS_LOG_INFO("server.ntsserver", "#===============================================#\n");
	NS_LOG_INFO("server.ntsserver", "Running from path %s", m_appRoot.c_str());
	NS_LOG_INFO("server.ntsserver", "Using configuration file %s.", configFile.c_str());
	NS_LOG_INFO("server.ntsserver", "Using Boost version: %i.%i.%i", BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);
	NS_LOG_INFO("server.ntsserver", "Bind Address: %s:%d", ntsListener.c_str(), ntsPort);

	// Set the signal handler
	// This must be set before starting the io_service thread pool, otherwise these threads will exit
	boost::asio::signal_set signals(m_ioService, SIGINT, SIGTERM);
#if PLATFORM == PLATFORM_WINDOWS
	signals.add(SIGBREAK);
#endif
	signals.async_wait(std::bind(&Application::signalHandler, this, std::placeholders::_1, std::placeholders::_2));

	// Start the io_service thread pool
	int numThreads = std::max(1, sConfigMgr->getIntDefault("ThreadPool", 1));
	for (int i = 0; i < numThreads; ++i)
		m_threadPool.emplace_back(boost::bind(&boost::asio::io_service::run, &m_ioService));

	// Create the PID file for ntsserver
	uint32 newPID = 0;
	if (!pidFile.empty())
	{
		if (newPID = createPIDFile(m_appRoot + pidFile))
			NS_LOG_INFO("server.ntsserver", "Daemon PID: %u", newPID);
		else
			RETURN_FAILURE("Cannot create PID file %s.", pidFile.c_str());
	}

	//  Set the priority of the process based on the configuration file
	setProcessPriority("server.ntsserver");

	// Load application configuration
	this->loadConfigs();

	int networkThreads = sConfigMgr->getIntDefault("Network.Threads", 1);
	if (networkThreads <= 0)
	{
		RETURN_FAILURE("Network.Threads must be greater than 0");
	}
	// Start listening socket for ntsserver
	if (!sNTSSocketMgr->startNetwork(m_ioService, ntsListener, ntsPort, networkThreads))
	{
		RETURN_FAILURE("Failed to start network.");
	}

	this->receiveIPCMsg(pid, newPID);

	// Start shutting down the server

	shutdownThreadPool();

	sLog->setSynchronous();

	NS_LOG_INFO("server.ntsserver", "Halting process...");


	sNTSSocketMgr->stopNetwork();

	return EXIT_SUCCESS;
}

bool Application::getBoolConfig(int32 key)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		return (*it).second.asBool();
	return false;
}

void Application::setBoolConfig(int32 key, bool value)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		(*it).second = value;
	else
		m_configs.emplace(key, value);
}

float Application::getFloatConfig(int32 key)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		return (*it).second.asFloat();
	return 0.f;
}

void Application::setFloatConfig(int32 key, float value)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		(*it).second = value;
	else
		m_configs.emplace(key, value);
}


int32 Application::getIntConfig(int32 key)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		return (*it).second.asInt();
	return 0;
}

void Application::setIntConfig(int32 key, int value)
{
	auto it = m_configs.find(key);
	if (it != m_configs.end())
		(*it).second = value;
	else
		m_configs.emplace(key, value);
}

// Returns true if the command option is processed, otherwise returns false
// The options contain unprocessed command options
bool Application::parseCommandLine(int argc, char** argv, std::string& configFile, variables_map& options)
{
	options_description desc("Allowed options");
	desc.add_options()
		("help,h", "print usage message")
		("version,v", "print version info")
		("config,c", value<std::string>(&configFile), StringUtil::format("use <arg> as configuration file(e.g. \"%s\").", DEFAULT_CONFIG_FILE).c_str())
		("stop", "stop the server");

	variables_map vm;
	store(command_line_parser(argc, argv).options(desc).run(), vm);
	notify(vm);

	if (vm.count("help"))
		std::cout << desc << "\n";
	else if (vm.count("version"))
		std::cout << "NTSServer (Build " << BUILD_NUMBER << ")\n";
	else
	{
		options = vm;
		return false;
	}

	return true;
}

void Application::sendOptionsToInstance(uint32 pid, variables_map const& options)
{
	if (!m_msgQueue)
	{
		std::string msgQueueName = StringUtil::format(MESSAGE_QUEUE_NAME_FORMAT, pid);
		m_msgQueue = new message_queue(open_only, msgQueueName.c_str());
	}

	for (auto it = options.begin(); it != options.end(); ++it)
	{
		std::string opt = (*it).first;
		if (!this->sendIPCMsg(m_msgQueue, opt))
			throw interprocess_exception("IPC message queue is full.");
	}
}

void Application::receiveIPCMsg(uint32 oldPID, uint32 newPID)
{
	message_queue::remove(StringUtil::format(MESSAGE_QUEUE_NAME_FORMAT, oldPID).c_str());

	std::string msgQueueName = StringUtil::format(MESSAGE_QUEUE_NAME_FORMAT, newPID);
	try {
		m_msgQueue = new message_queue(open_or_create, msgQueueName.c_str(), MAX_MSG_COUNT, MAX_MSG_SIZE);

		uint32 priority;
		message_queue::size_type recvdSize;
		char buff[MAX_MSG_SIZE];

		while (!m_ioService.stopped())
		{
			m_msgQueue->receive(&buff, MAX_MSG_SIZE, recvdSize, priority);
			std::string msg(buff, recvdSize);
			if (!msg.empty())
				this->handleIPCMsg(msg);
		}
	}
	catch (interprocess_exception const& e) 
	{
		NS_LOG_ERROR("server.ntsserver", "Failed to receive IPC message. error: %s", e.what());
	}

	message_queue::remove(msgQueueName.c_str());
}

// If the message is sent successfully, true is returned, otherwise false is returned, 
// indicating that the IPC message queue is full
bool Application::sendIPCMsg(message_queue* msgQueue, std::string const& msg)
{
	return msgQueue->try_send(msg.data(), msg.size(), 0);
}

void Application::handleIPCMsg(std::string const& msg)
{
	if (msg == "stop")
	{
		m_ioService.stop();
	}
}

void Application::signalHandler(const boost::system::error_code& error, int /*signalNumber*/)
{
	if (!error)
		m_ioService.stop();

	if (m_msgQueue)
	{
		if (!this->sendIPCMsg(m_msgQueue, "stop"))
			NS_LOG_ERROR("server.ntsserver", "Sending stop message failed because IPC message queue is full.");
	}
}

void Application::shutdownThreadPool()
{
	if (!m_ioService.stopped())
		m_ioService.stop();

	for (auto& thread : m_threadPool)
	{
		thread.join();
	}
	m_threadPool.clear();
}
