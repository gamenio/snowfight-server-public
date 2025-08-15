#ifndef __ASYNC_ACCEPTOR_H__
#define __ASYNC_ACCEPTOR_H__

#include <functional>
#include <atomic>

#include <boost/asio.hpp>

#include "logging/Log.h"



using boost::asio::ip::tcp;


class AsyncAcceptor
{
public:
	AsyncAcceptor(boost::asio::io_service& ioService) :
		m_tcpNoDelay(false),
		m_sendBufferSize(-1),
		m_acceptor(ioService),
		m_closed(true)
    {
    }

	~AsyncAcceptor()
	{
		this->close();
	}

	// Set the kernel send buffer size. -1 uses the system default value
	void setSendBufferSize(int32 size) { m_sendBufferSize = size; }

	void setTcpNoDelay(bool noDelay) { m_tcpNoDelay = noDelay; }

	bool setSocketOptions()
	{
		boost::system::error_code ec;

		//m_acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
		//if (ec)
		//{
		//	NS_LOG_ERROR("network.acceptor", "Acceptor set option(reuse_address) failed. err = %s", ec.message().c_str());
		//	return false;
		//}

		if (m_sendBufferSize >= 0)
		{
			m_acceptor.set_option(boost::asio::socket_base::send_buffer_size(m_sendBufferSize), ec);
			if (ec && ec != boost::system::errc::not_supported)
			{
				NS_LOG_ERROR("network.acceptor", "Acceptor set option(send_buffer_size) failed. err = %s", ec.message().c_str());
				return false;
			}
		}

		if (m_tcpNoDelay)
		{
			m_acceptor.set_option(boost::asio::ip::tcp::no_delay(true), ec);
			if (ec)
			{
				NS_LOG_ERROR("network.acceptor", "Acceptor set option(no_delay) failed. err = %s", ec.message().c_str());
				return false;
			}
		}

		return true;
	}

	bool listen(std::string const& bindIp, uint16 port)
	{
		boost::system::error_code ec;
		tcp::endpoint endpoint(boost::asio::ip::address::from_string(bindIp), port);

		m_acceptor.open(endpoint.protocol(), ec);
		if (ec)
		{
			NS_LOG_ERROR("network.acceptor", "Acceptor open failed. err = %s", ec.message().c_str());
			return false;
		}

		if (!this->setSocketOptions())
			return false;

		m_acceptor.bind(endpoint, ec);
		if (ec)
		{
			NS_LOG_ERROR("network.acceptor", "Acceptor bind failed. err = %s", ec.message().c_str());
			return false;
		}


		m_acceptor.listen(boost::asio::socket_base::max_connections, ec);
		if (ec)
		{
			NS_LOG_ERROR("network.acceptor", "Acceptor listen failed. err = %s", ec.message().c_str());
			return false;
		}

		m_closed = false;

		return true;
	}

	//
	// Asynchronously accept new connection
	// 
	// ACCEPT_CALLBACK, called when a new connection is established. Function prototype:
	// void(boost::asio::ip::tcp::socket&& newSocket, uint32 threadIndex)
	// The boost::system::system_error exception thrown by the callback function will be caught.
	//
	// SOCKET_FACTORY, receives new connection socket and network thread ID (NetworkThread), function prototype:
	// std::pair<tcp::socket*, uint32>()
	//
    template<typename ACCEPT_CALLBACK, typename SOCKET_FACTORY>
    void asyncAccept(ACCEPT_CALLBACK&& acceptCallback, SOCKET_FACTORY&& socketFactory)
    {
		m_acceptCallback = std::forward<ACCEPT_CALLBACK>(acceptCallback);
		m_socketFactory = std::forward<SOCKET_FACTORY>(socketFactory);

		this->accept();
    }

    void close()
    {
        if (m_closed.exchange(true))
            return;

        boost::system::error_code error;
        m_acceptor.close(error);
		if (error)
		{
			std::string emsg = error.message();
			NS_LOG_WARN("network.acceptor", "close() error(%d):%s", error.value(), emsg.c_str());
		}
    }

private:
	void accept()
	{
		tcp::socket* socket;
		uint32 threadIndex;
		std::tie(socket, threadIndex) = m_socketFactory();
		m_acceptor.async_accept(*socket, [this, socket, threadIndex](boost::system::error_code error)
		{
			if (!error)
			{
				try
				{
					m_acceptCallback(std::move(*socket), threadIndex);
				}
				catch (boost::system::system_error const& err)
				{
					NS_LOG_ERROR("network.acceptor", "Failed to initialize client's socket %s", err.what());
				}
			}

			if (!m_closed)
				this->accept();
		});
	}


	bool m_tcpNoDelay;
	int32 m_sendBufferSize;

    tcp::acceptor m_acceptor;
    std::atomic<bool> m_closed;

	std::function<void(tcp::socket&&, uint32)> m_acceptCallback;
    std::function<std::pair<tcp::socket*, uint32>()> m_socketFactory;
};


#endif //__ASYNC_ACCEPTOR_H__
