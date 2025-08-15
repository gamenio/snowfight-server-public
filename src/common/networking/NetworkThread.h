#ifndef __NETWORK_THREAD_H__
#define __NETWORK_THREAD_H__

#include <atomic>
#include <chrono>
#include <mutex>


#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/deadline_timer.hpp>

#include "Common.h"
#include "utilities/Util.h"
#include "debugging/Errors.h"
#include "logging/Log.h"
#include "containers/ConcurrentQueue.h"

#define UPDATE_TIMER_INTERVAL				10 // Milliseconds

using boost::asio::ip::tcp;

template<class SOCKET_TYPE>
class NetworkThread
{
public:
    NetworkThread() : 
		m_connections(0), 
		m_isStopped(false), 
		m_thread(nullptr),
        m_acceptSocket(m_ioService),
		m_updateTimer(m_ioService)
    {
    }

    virtual ~NetworkThread()
    {
        stop();
    }

	bool start()
	{
		if (m_thread)
			return false;

		m_thread = new std::thread(&NetworkThread::run, this);
		return true;
	}

	// Stop dispatching network IO events and wait for the thread to finish
    void stop()
    {
		if (m_isStopped.exchange(true))
			return;

        m_ioService.stop();

		if (m_thread)
		{
			m_thread->join();
			delete m_thread;
			m_thread = nullptr;
		}
    }

    virtual void addSocket(std::shared_ptr<SOCKET_TYPE> socket)
    {
		++m_connections;
		m_pendingSockets.add(socket);
    }

	int32 getConnectionCount() const { return m_connections; }
    tcp::socket* getSocketForAccept() { return &m_acceptSocket; }

protected:
    virtual void onSocketAdded(std::shared_ptr<SOCKET_TYPE> socket) { }
    virtual void onSocketRemoved(std::shared_ptr<SOCKET_TYPE> socket) { }

private:
    void processPendingSockets()
    {
		std::shared_ptr<SOCKET_TYPE> socket;
		while (m_pendingSockets.next(socket))
		{
			if (!socket->isOpen())
			{
				onSocketRemoved(socket);
				--m_connections;
			}
			else
			{
				m_sockets.push_back(socket);
				onSocketAdded(socket);

				socket->start();
			}
		}
    }

    void run()
    {
        NS_LOG_INFO("network.thread", "Network Thread(ID=%s) starting.", getCurrentThreadId().c_str());

		this->scheduleUpdateTimer();

        m_ioService.run();

        m_pendingSockets.clear();
        m_sockets.clear();

		NS_LOG_INFO("network.thread", "Network Thread(ID=%s) exited.", getCurrentThreadId().c_str());
    }

	void scheduleUpdateTimer()
	{
		boost::system::error_code ec;
		m_updateTimer.expires_from_now(boost::posix_time::milliseconds(UPDATE_TIMER_INTERVAL), ec);
		if (!ec)
			m_updateTimer.async_wait(std::bind(&NetworkThread<SOCKET_TYPE>::update, this, std::placeholders::_1));
		else
			NS_LOG_ERROR("network.thread", "Schedule update timer failed. msg(%d): %s", ec.value(), ec.message().c_str());
	}

    void update(boost::system::error_code const& error)
    {
		if (error)
		{
			NS_LOG_ERROR("network.thread", "Update timer error. msg(%d): %s", error.value(), error.message().c_str());
			return;
		}

        if (m_isStopped)
            return;

		this->scheduleUpdateTimer();
        this->processPendingSockets();

		// Remove closed sockets, or update them if they are not closed
		m_sockets.erase(std::remove_if(m_sockets.begin(), m_sockets.end(), [this](std::shared_ptr<SOCKET_TYPE> const& socket)
		{
			if (socket->isOpen())
			{
				socket->update();
				return false;
			}
			else
			{
				this->onSocketRemoved(socket);
				--this->m_connections;
				return true;
			}

		}), m_sockets.end());
    }

    typedef std::vector<std::shared_ptr<SOCKET_TYPE>> SocketContainer;

    std::atomic<int32> m_connections;
    std::atomic<bool> m_isStopped;

    std::thread* m_thread;

    SocketContainer m_sockets;

	ConcurrentQueue<std::shared_ptr<SOCKET_TYPE>> m_pendingSockets;

    boost::asio::io_service m_ioService;
    tcp::socket m_acceptSocket;
    boost::asio::deadline_timer m_updateTimer;
};

#endif // __NETWORK_THREAD_H__
