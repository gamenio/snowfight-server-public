#ifndef __THEATER_UPDATER_H__
#define __THEATER_UPDATER_H__

#include "Common.h"
#include "containers/BlockingQueue.h"
#include "Theater.h"

class TheaterUpdater;

class TheaterUpdateTask
{
public:
	TheaterUpdateTask();
	TheaterUpdateTask(Theater* theater, NSTime diff, TheaterUpdater* updater);

	void execute();
private:
	Theater* m_theater;
	NSTime m_diff;
	TheaterUpdater* m_updater;
};

class TheaterUpdater
{
public:
	TheaterUpdater();
	~TheaterUpdater();

	void start(uint32 numThreads);
	void stop();

	void waitUpdate();
	void scheduleUpdate(NSTime diff, Theater* theater);
	void updateFinished();

private:
	void workerThread();

	std::atomic<bool> m_isStopped;

	std::vector<std::thread> m_threadPool;
	BlockingQueue<TheaterUpdateTask> m_updateQueue;

	std::mutex m_mutex;
	std::condition_variable m_condition;
	uint32 m_pendingTasks;
};


#endif //__THEATER_UPDATER_H__