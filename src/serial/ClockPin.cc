// $Id$

#include <cassert>
#include "ClockPin.hh"

using std::string;

namespace openmsx {

ClockPin::ClockPin(Scheduler& scheduler, ClockPinListener* listener_)
	: Schedulable(scheduler), listener(listener_), periodic(false)
	, status(false), signalEdge(false)
{
}

void ClockPin::setState(bool newStatus, const EmuTime& time)
{
	periodic = false;
	if (signalEdge) {
		unschedule();
	}
	if (signalEdge && !status && newStatus) {
		// pos edge
		status = newStatus;
		if (listener) {
			listener->signalPosEdge(*this, time);
		}
	} else {
		status = newStatus;
	}
	if (listener) {
		listener->signal(*this, time);
	}
}

void ClockPin::setPeriodicState(const EmuDuration& total,
	const EmuDuration& hi, const EmuTime& time)
{
	referenceTime = time;
	totalDur = total;
	hiDur = hi;

	if (listener) {
		if (periodic) {
			unschedule();
		}
		periodic = true;
		if (signalEdge) {
			executeUntil(time, 0);
		}
		listener->signal(*this, time);
	} else {
		periodic = true;
	}
}


bool ClockPin::getState(const EmuTime& time) const
{
	if (!periodic) {
		return status;
	} else {
		return ((time - referenceTime) % totalDur) < hiDur;
	}
}

bool ClockPin::isPeriodic() const
{
	return periodic;
}

const EmuDuration& ClockPin::getTotalDuration() const
{
	assert(periodic);
	return totalDur;
}

const EmuDuration& ClockPin::getHighDuration() const
{
	assert(periodic);
	return hiDur;
}

int ClockPin::getTicksBetween(const EmuTime& begin, const EmuTime& end) const
{
	assert(begin <= end);
	if (!periodic) {
		return 0;
	}
	if (totalDur > EmuDuration::zero) {
		int a = (begin < referenceTime) ?
		        0 :
		        (begin - referenceTime) / totalDur;
		int b = (end   - referenceTime) / totalDur;
		return b - a;
	} else {
		return 0;
	}
}


void ClockPin::generateEdgeSignals(bool wanted, const EmuTime& time)
{
	if (signalEdge != wanted) {
		signalEdge = wanted;
		if (periodic) {
			if (signalEdge) {
				EmuTime tmp(referenceTime);
				while (tmp < time) {
					tmp += totalDur;
				}
				if (listener) {
					schedule(tmp);
				}
			} else {
				unschedule();
			}
		}
	}
}

void ClockPin::unschedule()
{
	removeSyncPoint();
}

void ClockPin::schedule(const EmuTime& time)
{
	assert(signalEdge && periodic && listener);
	setSyncPoint(time);
}

void ClockPin::executeUntil(const EmuTime& time, int /*userdata*/)
{
	assert(signalEdge && periodic && listener);
	listener->signalPosEdge(*this, time);
	if (signalEdge && (totalDur > EmuDuration::zero)) {
		schedule(time + totalDur);
	}
}

const string& ClockPin::schedName() const
{
	static const string name("ClockPin");
	return name;
}

} // namespace openmsx
