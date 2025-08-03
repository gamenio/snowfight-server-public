#ifndef __TIMER_H__
#define __TIMER_H__

#include "Common.h"

class IntervalTimer
{
public:

	explicit IntervalTimer(bool makeUpForLostTime = false):
		m_interval(0), 
		m_current(0), 
		m_remainder(0),
		m_makeUpForLostTime(makeUpForLostTime)
	{
	}

	void update(NSTime diff)
	{
		if (m_interval <= 0)
			return;

		m_current += diff;
	}

	bool passed() const
	{
		if (m_interval <= 0)
			return false;

		return m_current + m_remainder >= m_interval;
	}

	void setPassed()
	{
		m_current = m_interval;
		m_remainder = 0;
	}

	void reset()
	{
		if (m_makeUpForLostTime)
		{
			NSTime t = m_current + m_remainder;
			if (t > m_interval)
				m_remainder = t - m_interval;
			else
				m_remainder = 0;
		}
		
		m_current = 0;
	}

	void setInterval(NSTime interval)
	{
		m_remainder = 0;
		m_current = 0;
		m_interval = interval;
	}

	NSTime getInterval() const { return m_interval; }
	NSTime getCurrent() const { return m_current; }
	NSTime getRemainder() const { return m_remainder; }

private:
	NSTime m_interval;
	NSTime m_current;
	NSTime m_remainder;
	bool m_makeUpForLostTime;
};


class DelayTimer
{
public:
	DelayTimer() :
		m_duration(0),
		m_current(0)
	{
	}

	void update(NSTime diff)
	{
		if (m_current <= 0)
			return;

		m_current -= diff;
	}

	bool passed() const
	{
		return m_current <= 0;
	}

	void setPassed()
	{
		m_current = 0;
	}

	void reset()
	{
		m_current = m_duration;
	}

	void setDuration(NSTime duration)
	{
		m_current = duration;
		m_duration = duration;
	}
	NSTime getDuration() const { return m_duration; }

	NSTime getCurrent() const { return m_current; }
	NSTime getRemainder() const { return std::max(0, m_current); }
	NSTime getElapsed() const { return m_duration - this->getRemainder(); }

private:
	NSTime m_duration;
	NSTime m_current;
};


#endif //__TIMER_H__