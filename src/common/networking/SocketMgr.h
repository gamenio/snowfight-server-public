#ifndef __SOCKET_MGR_H__
#define __SOCKET_MGR_H__

#include <memory>

#include <boost/asio/ip/tcp.hpp>

#include "debugging/Errors.h"
#include "configuration/Config.h"
#include "AsyncAcceptor.h"
#include "NetworkThread.h"
#include "MessageBuffer.h"

using boost::asio::ip::tcp;

template<class SOCKET_TYPE>
class SocketMgr
{
public:
    virtual ~SocketMgr()
    {
		this->stopNetwork();
    }

	// 监听指定IP和端口上的连接请求，并启动网络I/O事件处理线程
	// 新的连接将被分派给网络线程处理
    virtual bool startNetwork(boost::asio::io_service& service, std::string const& bindIp, uint16 port, uint32 threadCount)
    {
        NS_ASSERT(threadCount > 0);

		if (!this->loadConfig())
			return false;

		NS_ASSERT(m_acceptor == nullptr, "Acceptor has been initialized.");
		m_acceptor = new AsyncAcceptor(service);
		m_acceptor->setSendBufferSize(m_sockOutKBuff);
		m_acceptor->setTcpNoDelay(m_tcpNoDelay);

		if (!m_acceptor->listen(bindIp, port))
			return false;

		NS_ASSERT(m_threads == nullptr, "Threads has been created.");
        m_threadCount = threadCount;
        m_threads = createThreads(threadCount);

        NS_ASSERT(m_threads);

		for (uint32 i = 0; i < m_threadCount; ++i)
			m_threads[i].start();
 
		m_acceptor->asyncAccept(std::bind(&SocketMgr::onSocketOpen, this, std::placeholders::_1, std::placeholders::_2), 
								std::bind(&SocketMgr::getSocketForAccept, this));

        return true;
    }

	// 停止接收新的连接并终止网络线程，已被接受的连接将被关闭
    virtual void stopNetwork()
    {
		this->closeAcceptor();
		this->stopThreads();
    }

	uint32 getNetworkThreadCount() const { return m_threadCount; }

protected:
    SocketMgr() : 
		m_acceptor(nullptr), 
		m_threads(nullptr), 
		m_threadCount(0),
		m_tcpNoDelay(false),
		m_sockOutKBuff(-1),
		m_sendQueueLimit(0)
    {
    }

	bool loadConfig()
	{
		m_tcpNoDelay = sConfigMgr->getBoolDefault("Network.TcpNoDelay", false);

		int const max_connections = boost::asio::socket_base::max_connections;
		NS_LOG_INFO("network.socket", "Max allowed socket connections %d", max_connections);

		m_sockOutKBuff = sConfigMgr->getIntDefault("Network.OutKBuff", -1);
		m_sendQueueLimit = sConfigMgr->getIntDefault("Network.SendQueueLimit", 0);

		return true;
	}

	virtual void onSocketOpen(tcp::socket&& socket, uint32 threadIndex)
	{
		try
		{
			std::shared_ptr<SOCKET_TYPE> newSocket = std::make_shared<SOCKET_TYPE>(std::move(socket));
			newSocket->setSendQueueLimit(m_sendQueueLimit);
			m_threads[threadIndex].addSocket(newSocket);
		}
		catch (boost::system::system_error const& error)
		{
			NS_LOG_ERROR("network.socket", "Failed to retrieve client's remote address. %s", error.what());
		}
	}

	uint32 selectThreadWithMinConnections() const
	{
		uint32 min = 0;

		for (uint32 i = 1; i < m_threadCount; ++i)
			if (m_threads[i].getConnectionCount() < m_threads[min].getConnectionCount())
				min = i;

		return min;
	}

	std::pair<tcp::socket*, uint32> getSocketForAccept()
	{
		uint32 threadIndex = selectThreadWithMinConnections();
		return std::make_pair(m_threads[threadIndex].getSocketForAccept(), threadIndex);
	}

	virtual NetworkThread<SOCKET_TYPE>* createThreads(uint32 count) const
	{
		return new NetworkThread<SOCKET_TYPE>[count]();
	}

	void closeAcceptor()
	{
		if (m_acceptor)
		{
			m_acceptor->close();
			delete m_acceptor;
			m_acceptor = nullptr;
		}
	}

	void stopThreads()
	{
		if (m_threads)
		{
			for (uint32 i = 0; i < m_threadCount; ++i)
				m_threads[i].stop();

			delete[] m_threads;
			m_threads = nullptr;

			m_threadCount = 0;
		}
	}

    AsyncAcceptor* m_acceptor;
    NetworkThread<SOCKET_TYPE>* m_threads;
    uint32 m_threadCount;

	bool m_tcpNoDelay;
	int32 m_sockOutKBuff;
	int32 m_sendQueueLimit;
};

#endif // __SOCKET_MGR_H__
