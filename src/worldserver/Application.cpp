#include "Application.h"

#include "debugging/Errors.h"
#include "configuration/Config.h"
#include "threading/ProcessPriority.h"
#include "logging/Log.h"
#include "utilities/Util.h"
#include "game/server/WorldSocketMgr.h"
#include "game/world/World.h"

#define DEFAULT_CONFIG_FILE     "worldserver.conf"
#define DEFAULT_LOG_DIR         "log"

// Minimum compatible game version number (client version number)
//    Format:        0x00  Major  Minor  Revision
//                   00  01     00     00
#define GAME_VERSION				0x00030100

// Build version number
#define BUILD_NUMBER                27

// Return EXIT_FAILURE after printing the error and releasing resource
#define RETURN_FAILURE(...) \
	do { \
		NS_LOG_ERROR("server.worldserver", __VA_ARGS__); \
		this->shutdownThreadPool(); \
		return EXIT_FAILURE; \
	} while(0)

Application::Application()
{

}


Application::~Application()
{
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
	catch (std::exception& e)
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

	std::string pidFile = sConfigMgr->getStringDefault("PidFile", "");
	// Check whether the instance is already running
	if (!pidFile.empty())
	{
		uint32 pid = getPIDFromFile(m_appRoot + pidFile);
		if (pid && isProcessRunning(pid))
		{
			printf("An instance (PID=%u) of server is already running.\n", pid);
			return EXIT_FAILURE;
		}
	}

	// If you want to output logs asynchronously, you need to pass io_service to the Log singleton
	// Normally, logs are output in the thread that calls it, which may be the main thread
    std::string logPath = m_appRoot + DEFAULT_LOG_DIR + "/";
	sLog->initialize(sConfigMgr->getBoolDefault("Log.Async.Enable", false) ? &m_ioService : nullptr, logPath);

	uint16 worldPort = uint16(sConfigMgr->getIntDefault("WorldServerPort", 18402));
	std::string worldListener = sConfigMgr->getStringDefault("BindIP", "0.0.0.0");

	NS_LOG_INFO("server.worldserver", "WorldServer-Daemon (Game version %x.%x.%x, Build %d)", ((GAME_VERSION >> 16) & 0x000000FF), ((GAME_VERSION >> 8) & 0x000000FF), (GAME_VERSION & 0x000000FF), BUILD_NUMBER);
	NS_LOG_INFO("server.worldserver", "<Ctrl-C> to stop.\n");
	NS_LOG_INFO("server.worldserver", "#===============================================#");
	NS_LOG_INFO("server.worldserver", "#  ____ _  _ ____ _ _ _   ____ _ ____ _  _ ___  #");
	NS_LOG_INFO("server.worldserver", "#  [__  |\\ | |  | | | |   |___ | | __ |__|  |   #");
	NS_LOG_INFO("server.worldserver", "#  ___] | \\| |__| |_|_|   |    | |__] |  |  |   #");
	NS_LOG_INFO("server.worldserver", "#                                               #");
	NS_LOG_INFO("server.worldserver", "#                                   WorldServer #");
	NS_LOG_INFO("server.worldserver", "#===============================================#\n");
	NS_LOG_INFO("server.worldserver", "Running from path %s", m_appRoot.c_str());
	NS_LOG_INFO("server.worldserver", "Using configuration file %s.", configFile.c_str());
	NS_LOG_INFO("server.worldserver", "Using Boost version: %i.%i.%i", BOOST_VERSION / 100000, BOOST_VERSION / 100 % 1000, BOOST_VERSION % 100);
	NS_LOG_INFO("server.worldserver", "Bind Address: %s:%d", worldListener.c_str(), worldPort);

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

	// Create the PID file for worldserver
	if (!pidFile.empty())
	{
		if (uint32 pid = createPIDFile(m_appRoot + pidFile))
			NS_LOG_INFO("server.worldserver", "Daemon PID: %u", pid);
		else
			RETURN_FAILURE("Cannot create PID file %s.", pidFile.c_str());
	}

	// Set the priority of the process based on the configuration file
	setProcessPriority("server.worldserver");

	// Initialize the world
	if (!sWorld->initWorld())
	{
		RETURN_FAILURE("Failed to initialize world.");
	}

	int networkThreads = sConfigMgr->getIntDefault("Network.Threads", 1);
	if (networkThreads <= 0)
	{
		RETURN_FAILURE("Network.Threads must be greater than 0");
	}

	// Start listening socket for worldserver
	if (!sWorldSocketMgr->startNetwork(m_ioService, worldListener, worldPort, networkThreads))
	{
		RETURN_FAILURE("Failed to start network.");
	}

	sWorld->run();

	// Start shutting down the server

	shutdownThreadPool();

	sLog->setSynchronous();

	NS_LOG_INFO("server.worldserver", "Halting process...");


	sWorldSocketMgr->stopNetwork();

	return EXIT_SUCCESS;
}

// Returns true if the command option is processed, otherwise returns false
// The options contain unprocessed command options
bool Application::parseCommandLine(int argc, char** argv, std::string& configFile, variables_map& options)
{
	options_description desc("Allowed options");
	desc.add_options()
		("help,h", "print usage message")
		("version,v", "print version info")
		("config,c", value<std::string>(&configFile), StringUtil::format("use <arg> as configuration file(e.g. \"%s\").", DEFAULT_CONFIG_FILE).c_str());
	variables_map vm;
	store(command_line_parser(argc, argv).options(desc).run(), vm);
	notify(vm);

	if (vm.count("help"))
	{
		std::cout << desc << "\n";
	}
	else if (vm.count("version"))
	{
        std::cout << "WorldServer game version "
                << ((GAME_VERSION >> 16) & 0x000000FF)
                << "." << ((GAME_VERSION >> 8) & 0x000000FF)
                << "." << (GAME_VERSION & 0x000000FF)
                << " (Build "
                << BUILD_NUMBER
                << ")\n";
	}
	else
	{
		options = vm;
		return false;
	}

	return true;
}


void Application::signalHandler(const boost::system::error_code& error, int /*signalNumber*/)
{
	if (!error)
		sWorld->stopNow();
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
