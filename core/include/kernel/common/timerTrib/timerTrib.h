#ifndef _TIMER_TRIB_H
	#define _TIMER_TRIB_H

	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/clock_t.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>

#define TIMERTRIB_WATCHDOG_ALREADY_REGISTERED	(1)

#define TIMERTRIB_PER_CPU_CLOCK_EMU		(1<<0)

class timerTribC
:
public tributaryC
{
public:
	timerTribC(void);
	error_t initialize(void);
	~timerTribC(void);

public:
	status_t registerWatchdogIsr(status_t (*isr)(), uarch_t interval);
	void updateWatchdogIsr(uarch_t interval);
	void unregisterWatchdogIsr(void);

	void updateContinuousClock(void);
	void updateScheduledClock(uarch_t sourceId);

	// For MP systems with no per-cpu clock source. Slow. Non-deterministic.
	void enablePerCpuClockEmu(void);
	void disablePerCpuClockEmu(void);

private:
	// Arch-specific per-cpu scheduler call emulation.
	void invokeSchedulerEmu(void);

	// The watchdog timer for the chipset, if it exists.
	struct watchdogIsrS
	{
		status_t	(*isr)();
		clock_t		feedTime;
		uarch_t		interval;
	};

	uarch_t		flags;
	sharedResourceGroupC<waitLockC, watchdogIsrS>	watchdog;

	// Please LOOK at the source for clock_t in /core/include/__kclasses.
	sharedResourceGroupC<waitLockC, clock_t>	continuousClock;
};

extern timerTribC		timerTrib;

#endif
