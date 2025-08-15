#ifndef __PRODUCER_CONSUMER_QUEUE_H__
#define __PRODUCER_CONSUMER_QUEUE_H__

#include <condition_variable>
#include <mutex>
#include <queue>
#include <atomic>
#include <type_traits>

template <typename T>
class BlockingQueue
{
public:

	BlockingQueue() : m_shutdown(false) { }

	void push(T const& value)
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_queue.push(value);

		lock.unlock();

		m_condition.notify_one();
	}

	void push(T&& value)
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);
		m_queue.push(std::forward<T>(value));

		lock.unlock();

		m_condition.notify_one();
	}

	bool empty()
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);

		return m_queue.empty();
	}

	size_t size()
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);

		return m_queue.size();
	}

	bool pop(T& value)
	{
		std::lock_guard<std::mutex> lock(m_queueMutex);

		if (m_queue.empty() || m_shutdown)
			return false;

		value = std::move(m_queue.front());

		m_queue.pop();

		return true;
	}


	// If there is a new element in the queue, it is popped and stored in the value. Otherwise, 
	// the thread waits until there is a new element in the queue or cancel() is called.
	// If true is returned, a new element is stored in the value
	bool waitAndPop(T& value)
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);

		// we could be using .wait(lock, predicate) overload here but it is broken
		// https://connect.microsoft.com/VisualStudio/feedback/details/1098841
		while (m_queue.empty() && !m_shutdown)
			m_condition.wait(lock);

		if (m_queue.empty() || m_shutdown)
			return false;

		value = std::move(m_queue.front());
		m_queue.pop();

		return true;
	}

	// Wake up all waiting threads and remove elements from the queue
	void cancel()
	{
		std::unique_lock<std::mutex> lock(m_queueMutex);

		while (!m_queue.empty())
		{
			T& value = m_queue.front();
			deleteElement(value);
			m_queue.pop();
		}

		m_shutdown = true;
		m_condition.notify_all();
	}

private:
	template<typename E = T>
	typename std::enable_if<std::is_pointer<E>::value>::type deleteElement(E& obj) { delete obj; }

	template<typename E = T>
	typename std::enable_if<!std::is_pointer<E>::value>::type deleteElement(E const&) { }

	std::mutex m_queueMutex;
	std::queue<T> m_queue;
	std::condition_variable m_condition;
	bool m_shutdown;

};

#endif //__PRODUCER_CONSUMER_QUEUE_H__
