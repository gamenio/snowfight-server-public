#ifndef __APPLICATION_H__
#define __APPLICATION_H__

#include <boost/asio/io_service.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/error_code.hpp>

#include "Common.h"

using namespace boost::program_options;

class Application
{
public:
	static Application* instance();

	int run(int argc, char** argv);
	// 获取应用程序的根目录
	// 格式: /home/.../{SERVER_APP_DIR}/
	std::string const& getRoot() const { return m_appRoot; }

private:
	Application();
	~Application();

	void shutdownThreadPool();
	void signalHandler(const boost::system::error_code& error, int signalNumber);
	bool parseCommandLine(int argc, char** argv, std::string& configFile, variables_map& options);

	boost::asio::io_service m_ioService;
	std::string m_appRoot;
	std::vector<std::thread> m_threadPool;
};

#define  sApplication Application::instance()

#endif //__APPLICATION_H__
