#ifndef __SOCKET_H__
#define __SOCKET_H__


#include <atomic>

#include <boost/asio.hpp>

#include "Common.h"
#include "utilities/Util.h"
#include "logging/Log.h"
#include "debugging/Errors.h"
#include "containers/ConcurrentQueue.h"
#include "MessageBuffer.h"
#include "BasicPacket.h"

using boost::asio::ip::tcp;

template<typename SOCKET_TYPE, typename PACKET_TYPE>
class Socket: public std::enable_shared_from_this<SOCKET_TYPE>
{
public:
	enum
	{
		// The default size of the message buffer
		MESSAGE_BUFFER_SIZE = 4096,
		// Unlimited send queue size
		SEND_QUEUE_UNLIMITED = 0

	};

	// Construct a Socket object of the specified type
	// The readBufferSize is used to set the size of the receive buffer in user space
    Socket(tcp::socket&& socket) try:
		m_readBuffer(MESSAGE_BUFFER_SIZE),
		m_sendQueueLimit(SEND_QUEUE_UNLIMITED),
		m_writeQueueSize(0),
		m_isWritingAsync(false),
		m_isClosed(false),
		m_socket(std::move(socket)),
		m_remoteEndpoint(m_socket.remote_endpoint())
	{ 
	}
	catch (boost::system::system_error const& error) 
	{
		NS_LOG_DEBUG("network.socket", "Failed to construct socket. msg(%d): %s", error.code().value(), error.what());
	}

	virtual ~Socket() 
	{
		m_isClosed = true;
		boost::system::error_code error;
		m_socket.close(error);
		if (error)
		{
			std::string emsg = getUtf8ErrorMsg(error);
			NS_LOG_DEBUG("network.socket", "Close socket error. address: %s:%d msg(%d): %s", this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), error.value(), emsg.c_str());
		}
	}

	void setSendQueueLimit(int32 size) { m_sendQueueLimit = size; }
	int32 getSendQueueLimit() const { return m_sendQueueLimit; }

	tcp::endpoint const& getRemoteEndpoint() const { return m_remoteEndpoint;  }
	boost::asio::ip::address getRemoteAddress() const { return m_remoteEndpoint.address(); }
	uint16 getRemotePort() const { return m_remoteEndpoint.port(); }

	// Returns the open status of the socket. The function is thread-safe
	bool isOpen() { return !m_isClosed; }

	void asyncRead() { this->readHeader(); }

	// Actively closes the socket. The function is thread-safe
	void closeSocket()
	{
		if (m_isClosed.exchange(true))
			return;

		boost::system::error_code error;
		m_socket.shutdown(boost::asio::socket_base::shutdown_send, error);
		if (error)
		{
			std::string emsg = getUtf8ErrorMsg(error);
			NS_LOG_DEBUG("network.socket", "Shutdown socket error. address: %s:%d msg(%d): %s", this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), error.value(), emsg.c_str());
		}

		this->onSocketClosed();
	}


	// Add data packet to queue. Function is thread-safe.
	void queuePacket(PACKET_TYPE&& packet)
	{
		if (m_isClosed)
			return;

		m_packetQueue.add(std::move(packet));
	}

	// Called after the socket is added to the network thread queue
	virtual void start() = 0;

	// Called when the socket receives data
	virtual void onReceivedData(PACKET_TYPE&& packet) { }

	// Called when the socket is closed
	// This function may be called in a non-network thread, which is related to the location where the closeSocket() function is called
	virtual void onSocketClosed() { }

	virtual void update()
	{
		if (m_packetQueue.empty())
			return;

		MessageBuffer buff(MESSAGE_BUFFER_SIZE);
		PACKET_TYPE packet;

		// Merge data packets into a single MessageBuffer as much as possible before sending
		while (m_packetQueue.next(packet))
		{
			if (m_isClosed)
				return;

			if (buff.getRemainingSpace() < packet.getByteSize() 
				&& buff.getActiveSize() > 0)
			{
				addToWriteQueue(std::move(buff));
				buff.resize(MESSAGE_BUFFER_SIZE);
			}

			if (buff.getRemainingSpace() >= packet.getByteSize())
			{
				packet.write(buff);
			}
			// A single packet is larger than SEND_BUFFER_SIZE
			else
			{
				MessageBuffer largeBuf(packet.getByteSize());
				packet.write(largeBuf);
				addToWriteQueue(std::move(largeBuf));
			}
		}
		if (buff.getActiveSize() > 0)
		{
			addToWriteQueue(std::move(buff));
		}
	}

private:
	void readHeader()
	{
		m_readBuffer.reset();
		auto self(this->shared_from_this());
		boost::asio::async_read(m_socket, boost::asio::buffer(m_readBuffer.getWritePointer(), PACKET_TYPE::HEADER_BYTE_SIZE),
			[this, self](boost::system::error_code const& error, std::size_t bytes_transferred)
		{
			if (!error)
			{
				NS_ASSERT(bytes_transferred == PACKET_TYPE::HEADER_BYTE_SIZE);

				m_readBuffer.writeCompleted(static_cast<uint16>(bytes_transferred));
				try 
				{
					m_readPacket.decodeHeader(m_readBuffer);
					if (m_readPacket.hasBody())
						this->readBody();
					else
					{
						this->onReceivedData(std::move(m_readPacket));
						this->readHeader();
					}
				}
				catch (PacketException const& ex)
				{
					this->closeSocket();
					NS_LOG_ERROR("network.socket", "Decode header of packet failed from %s:%d msg: %s", this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), ex.what());
				}
			} 
			else
			{
				this->closeSocket();
				std::string emsg = getUtf8ErrorMsg(error);
				NS_LOG_DEBUG("network.socket", "Read data header failed. address: %s:%d msg(%d): %s", this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), error.value(), emsg.c_str());
			}

		});
	}

	void readBody()
	{
		m_readBuffer.ensureFreeSpace(m_readPacket.getBodyBytes());
		auto self(this->shared_from_this());
		boost::asio::async_read(m_socket, boost::asio::buffer(m_readBuffer.getWritePointer(), m_readPacket.getBodyBytes()),
			[this, self](boost::system::error_code const& error, std::size_t bytes_transferred)
		{
			if (!error)
			{
				NS_ASSERT(bytes_transferred == m_readPacket.getBodyBytes());

				m_readBuffer.writeCompleted(static_cast<uint16>(bytes_transferred));
				m_readPacket.readBody(m_readBuffer);
				this->onReceivedData(std::move(m_readPacket));

				this->readHeader();
			}
			else
			{
				this->closeSocket();
				std::string emsg = getUtf8ErrorMsg(error);
				NS_LOG_DEBUG("network.socket", "Read data body failed.  address: %s:%d msg(%d): %s", this->getRemoteAddress().to_string().c_str(), this->getRemotePort(), error.value(), emsg.c_str());
			}

		});
	}

	void addToWriteQueue(MessageBuffer&& buff)
	{
		if (m_sendQueueLimit != SEND_QUEUE_UNLIMITED && m_writeQueueSize >= m_sendQueueLimit)
		{
			NS_LOG_WARN("network.socket", "Send queue is full (%d >= %d bytes). address: %s:%d", m_writeQueueSize, m_sendQueueLimit, this->getRemoteAddress().to_string().c_str(), this->getRemotePort());
			this->closeSocket();
		}
		else
		{
			m_writeQueueSize += buff.getBufferSize();
			m_writeQueue.push(std::move(buff));
			this->processWriteQueue();
		}

	}

	void processWriteQueue()
	{
		if (m_isWritingAsync)
			return;

		m_isWritingAsync = true;

		MessageBuffer& buff = m_writeQueue.front();
		auto self(this->shared_from_this());
		boost::asio::async_write(m_socket, boost::asio::buffer(buff.getReadPointer(), buff.getActiveSize()),
			[this, self, &buff](boost::system::error_code const& error, std::size_t bytes_transferred)
		{
			if (!error)
			{
				m_isWritingAsync = false;

				NS_ASSERT(bytes_transferred == buff.getActiveSize());
				buff.readCompleted(static_cast<uint16>(bytes_transferred));

				m_writeQueueSize -= buff.getBufferSize();
				m_writeQueue.pop();

				if(!m_writeQueue.empty())
					this->processWriteQueue();
			}
			else
			{
				this->closeSocket();
			}
		});
	}

	PACKET_TYPE m_readPacket;
	MessageBuffer m_readBuffer;

	int32 m_sendQueueLimit;
	int32 m_writeQueueSize;
	ConcurrentQueue<PACKET_TYPE> m_packetQueue;
	std::queue<MessageBuffer> m_writeQueue;
	bool m_isWritingAsync;

	std::atomic<bool> m_isClosed;
	tcp::socket m_socket;

	tcp::endpoint m_remoteEndpoint;
};


#endif //__SOCKET_H__
