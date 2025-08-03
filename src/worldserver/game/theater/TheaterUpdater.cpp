#include "TheaterUpdater.h"

#include "utilities/Util.h"

TheaterUpdateTask::TheaterUpdateTask() :
	m_theater(nullptr),
	m_diff(0),
	m_updater(nullptr)
{

}

TheaterUpdateTask::TheaterUpdateTask(Theater* theater, NSTime diff, TheaterUpdater* updater) :
	m_theater(theater),
	m_diff(diff),
	m_updater(updater)
{

}

void TheaterUpdateTask::execute()
{
	m_theater->update(m_diff);
	m_updater->updateFinished();
}



TheaterUpdater::TheaterUpdater() :
	m_isStopped(true),
	m_pendingTasks(0)
{

}

TheaterUpdater::~TheaterUpdater()
{
	this->stop();
}

void TheaterUpdater::start(uint32 numThreads)
{
	if (!m_isStopped.exchange(false))
		return;

	for (uint32 i = 0; i < numThreads; i++)
	{
		m_threadPool.push_back(std::thread(&TheaterUpdater::workerThread, this));
	}
}

void TheaterUpdater::waitUpdate()
{
	std::unique_lock<std::mutex> lock(m_mutex);

	while (m_pendingTasks > 0)
		m_condition.wait(lock);
}

void TheaterUpdater::scheduleUpdate(NSTime diff, Theater* theater)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	++m_pendingTasks;

	m_updateQueue.push(TheaterUpdateTask(theater, diff, this));
}

void TheaterUpdater::updateFinished()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	--m_pendingTasks;

	m_condition.notify_all();
}


void TheaterUpdater::stop()
{
	if (m_isStopped.exchange(true))
		return;

	NS_LOG_INFO("world.theater", "TheaterUpdater is stopping. PendingTasks: %d", m_pendingTasks);

	std::unique_lock<std::mutex> lock(m_mutex);
	m_pendingTasks = 0;
	m_condition.notify_all();

	lock.unlock();

	m_updateQueue.cancel();
	for (auto& thread : m_threadPool)
	{
		thread.join();
	}
	m_threadPool.clear();
}

void TheaterUpdater::workerThread()
{
	while (!m_isStopped)
	{
		TheaterUpdateTask task;

		if(m_updateQueue.waitAndPop(task))
			task.execute();
	}

	NS_LOG_DEBUG("world.theater", "TheaterUpdater worker thread(ID=%s) has exited. QueueSize: %zu", getCurrentThreadId().c_str(), m_updateQueue.size());
}
