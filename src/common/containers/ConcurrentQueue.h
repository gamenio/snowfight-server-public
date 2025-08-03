#ifndef __LOCKED_QUEUE_H__
#define __LOCKED_QUEUE_H__

#include <deque>
#include <mutex>


template<typename T, typename ContainerType = std::deque<T>>
class ConcurrentQueue
{
public:
	bool next(T& result)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (m_container.empty())
			return false;

		result = std::move(m_container.front());
		m_container.pop_front();

		return true;
	}

	void add(T&& element)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_container.push_back(std::move(element));
	}

	void add(T const& element)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_container.push_back(element);
	}

	void clear()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_container.clear();
	}

	void lock()
	{
		this->m_mutex.lock();
	}

	void unlock()
	{
		this->m_mutex.unlock();
	}


	bool empty()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_container.empty();
	}

	size_t size()
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		return m_container.size();
	}

private:
	std::mutex m_mutex;
	ContainerType m_container;
};


#endif //__LOCKED_QUEUE_H__