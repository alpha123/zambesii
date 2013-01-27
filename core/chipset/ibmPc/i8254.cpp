
#include <arch/io.h>
#include <arch/atomic.h>
#include <arch/cpuControl.h>
#include <chipset/zkcm/zkcmCore.h>
#include <__kstdlib/__kbitManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/interruptTrib/interruptTrib.h>

#include "i8254.h"


/**	EXPLANATION:
 * While we define constants for the IO locations used by channels 2 and 3, we
 * do not have any code in this file that manipulates those channels. This file
 * is solely concerned with channel 0, the only channel that has its CLK-OUT
 * signal attached directly to an IRQ pin signal.
 *
 * Channel 2's CLK-OUT signal goes to the speaker subsystem, and channel 3's
 * CLK-OUT signal goes to the NMI logic. While it could have been feasible to
 * possibly use channel 3 as a timer source, sadly, it will only generates an
 * NMI when channel 0's IRQ is raised for too long without being acknowledged.
 * In other words, it cannot actually generate an NMI signal independently of
 * channel 0, since its ability to generate the NMI depends on channel 0's
 * IRQ being raised.
 *
 *	OPERATION:
 * We drive the i8254 in mode-0 for oneshot operation, and mode-2 for periodic
 * operation.
 *
 * When we require the i8254 to be "shut off" and to cease sending
 * IRQs, we program it to operate in mode-0 for one time-out, then EOI it. The
 * reason for this is that I do not believe that Linux uses mode 0
 **/

i8254PitC		i8254Pit(0);

// Chan 0 and 2 counters are 16bit, chan 3 counter is 8bit.
#define i8254_CHAN0_IO_COUNTER		(0x40)
#define i8254_CHAN2_IO_COUNTER		(0x42)
#define i8254_CHAN3_IO_COUNTER		(0x44)
// All control regs are 8bit and write-only.
#define i8254_CHAN0_IO_CONTROL		(0x43)
#define i8254_CHAN2_IO_CONTROL		(0x43)
#define i8254_CHAN3_IO_CONTROL		(0x47)

#define i8254_CHAN0_CONTROL_SELECT_COUNTER		(0x0<<6)
#define i8254_CHAN0_CONTROL_COUNTER_WRITE_LOW		(0x1<<4)
#define i8254_CHAN0_CONTROL_COUNTER_WRITE_HIGH		(0x2<<4)
#define i8254_CHAN0_CONTROL_COUNTER_WRITE_LOWHIGH	(0x3<<4)
// Must set bits 4-5 to 0 when attempting to read the counter regs.
#define i8254_CHAN0_CONTROL_COUNTER_LATCH		(0x0<<4)
// All modes which are invalid for channel 0 are commented out.
#define i8254_CHAN0_CONTROL_MODE0_ONESHOT		(0x0<<1)
// #define i8254_CHAN0_CONTROL_MODE1_HRETRIGGER_ONESHOT	(0x1<<1) INVALID chan0.
#define i8254_CHAN0_CONTROL_MODE2_RATEGEN		(0x2<<1)
#define i8254_CHAN0_CONTROL_MODE3_SQUARE_WAVE		(0x3<<1)
#define i8254_CHAN0_CONTROL_MODE4_STROBE		(0x4<<1)
//#define i8254_CHAN0_CONTROL_MODE5_HARDWARE_STROBE	(0x5<<1) INVALID chan0.

#define i8254_CHAN2_CONTROL_SELECT_COUNTER		(0x2<<6)
#define i8254_CHAN2_CONTROL_COUNTER_RWLOW		(0x1<<4)
#define i8254_CHAN2_CONTROL_COUNTER_RWHIGH		(0x2<<4)
// Must set bits 4-6 to 0 when attempting to read the counter regs.
#define i8254_CHAN2_CONTROL_COUNTER_LATCH		(0x0<<4)
// All modes which are invalid for channel 2 are commented out.
#define i8254_CHAN2_CONTROL_MODE0_ONESHOT		(0x0<<1)
#define i8254_CHAN2_CONTROL_MODE1_HARDWARE_PERIODIC	(0x1<<1)
#define i8254_CHAN2_CONTROL_MODE2_RATEGEN		(0x2<<1)
#define i8254_CHAN2_CONTROL_MODE3_PERIODIC		(0x3<<1)
#define i8254_CHAN2_CONTROL_MODE4_STROBE		(0x4<<1)
#define i8254_CHAN2_CONTROL_MODE5_HARDWARE_STROBE	(0x5<<1)

#define i8254_CHAN3_CONTROL_SELECT_COUNTER	(0x0<<6)
#define i8254_CHAN3_CONTROL_COUNTER_RWLOW	(0x1<<4)


error_t i8254PitC::initialize(void)
{
	error_t		ret;

	// Disable timer source and EOI it to ensure it's not asserting its IRQ.
	disable();
	sendEoi();

	// Expose the i8254 channel 0 timer source to the Timer Control mod.
	ret = zkcmCore.timerControl.registerNewTimerDevice(this);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(WARNING i8254"Failed to register i8254 with Timer "
			"Control mod.\n");

		return ret;
	};

	return ERROR_SUCCESS;
}

error_t i8254PitC::shutdown(void)
{
	error_t		ret;

	ret = zkcmCore.timerControl.unregisterTimerDevice(this, 0);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(WARNING i8254"Unable to unregister %s; latched.\n",
			getBaseDevice()->shortName);

		return ret;
	};

	return ERROR_SUCCESS;
}

void i8254PitC::sendEoi(void)
{
	uarch_t		flags;
	ubit8		val;

	cpuControl::safeDisableInterrupts(&flags);
	val = io::read8(0x61);
	__KBIT_UNSET(val, 7);
	io::write8(0x61, val);
	cpuControl::safeEnableInterrupts(flags);

	state.lock.acquire();

	if (state.rsrc.mode == ONESHOT)
	{
		__KFLAG_UNSET(
			state.rsrc.flags,
			ZKCM_TIMERDEV_STATE_FLAGS_ENABLED);
	};

	state.lock.release();
}

status_t i8254PitC::isr(zkcmDeviceBaseC *self, ubit32 flags)
{
	(void)		flags;

	/**	EXPLANATION:
	 * Queue a notification on the waitqueue of the process that is
	 * latched to us.
	 **/
	static_cast<i8254PitC *>( self )->sendEoi();
	return ERROR_SUCCESS;
}

error_t i8254PitC::enable(void)
{
	error_t		ret;

	state.lock.acquire();
	if (state.rsrc.mode == UNINITIALIZED)
	{
		state.lock.release();
		return ERROR_UNINITIALIZED;
	};
	state.lock.release();

	if (!validateCallerIsLatched()) { return ERROR_RESOURCE_BUSY; };

	/**	EXPLANATION:
	 * 1. Lookup the correct __kpin for our IRQ (ISA IRQ 0).
	 * 2. Register our ISR with the kernel on the correct __kpin list.
	 * 3. Ask the kernel to enable our __kpin.
	 * 4. Program the i8254 to begin sending in IRQs.
	 **/
	if (!isrRegistered)
	{
		ret = zkcmCore.irqControl.bpm.get__kpinFor(
			CC"isa", 0, &__kpinId);

		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR i8254"enable: BPM was unable to map "
				"ISA IRQ 0 to a __kpin.\n");

			return ret;
		};

		__kprintf(NOTICE i8254"enable: BPM reports ISA IRQ 0 __kpin is "
			"%d.\n",
			__kpinId);

		ret = interruptTrib.zkcm.registerPinIsr(
			__kpinId, this, &isr, 0);

		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR i8254"enable: Failed to register ISR "
				"on __kpin %d.\n",
				__kpinId);

			return ret;
		};

		isrRegistered = 1;
	};

	// Now program the i8254 to begin interrupting.
	if (state.rsrc.mode == ONESHOT) {
		writeOneshotIo();
	} else {
		writePeriodicIo();
	};

	ret = interruptTrib.__kpinEnable(__kpinId);
	if (ret != ERROR_SUCCESS)
	{
		__kprintf(ERROR i8254"enable: Interrupt Trib failed to "
			"enable IRQ for __kpin %d.\n",
			__kpinId);

		goto failOut;
	};

	return ERROR_SUCCESS;

failOut:
	interruptTrib.zkcm.retirePinIsr(__kpinId, &isr);
	return ret;
}

void i8254PitC::disable(void)
{
	/**	EXPLANATION:
	 * 1. Program the i8254 to stop interrupting.
	 * 2. Unregister our ISR from Interrupt Trib.
	 *	InterruptTrib will automatically mask the IRQ pin off if
	 *	it determines that there are no more devices actively signaling
	 *	on the pin.
	 *
	 * Linux sets the i8254 to mode 0, with a 0 CLK pulse timeout when it
	 * wants to disable it.
	 **/

	state.lock.acquire();

	// Write out the oneshot mode (mode 0) control byte.
	io::write8(
		i8254_CHAN0_IO_CONTROL,
		i8254_CHAN0_CONTROL_SELECT_COUNTER
		| i8254_CHAN0_CONTROL_MODE0_ONESHOT
		| i8254_CHAN0_CONTROL_COUNTER_WRITE_LOWHIGH);

	atomicAsm::memoryBarrier();
	io::write8(i8254_CHAN0_IO_COUNTER, 0);
	io::write8(i8254_CHAN0_IO_COUNTER, 0);
	__KFLAG_UNSET(state.rsrc.flags, ZKCM_TIMERDEV_STATE_FLAGS_ENABLED);

	state.lock.release();

	if (isrRegistered) {
		interruptTrib.zkcm.retirePinIsr(__kpinId, &isr);
	};
}

static inline sarch_t validateTimevalLimit(timeS time)
{
	/**	EXPLANATION:
	 * The i8254's input frequency is a 1,193,180Hz CLK source. This means
	 * that not every period requirement can be translated exactly. For
	 * example, if the user asked for 20 nanoseconds to be timed, we would
	 * actually program the PIT to interrupt after 24 CLK pulses and not 20,
	 * because the input CLK source is faster than 1,000,000Hz.
	 *
	 * The exact conversion table being used is as follows:
	 *	1ns: 1,193,180ps: 1.193 CLKs -> 1 CLK = 1ns.
	 *	10ns: 11.93 CLKs -> 12 CLKs = 10ns.
	 *	100ns: 119.3 CLKs -> 119 CLKs = 100ns.
	 *	1ms: 1,193.180 CLKs -> 1,193 CLKs = 1ms.
	 *	10ms: 11,931.8 CLKs -> 11,932 CLKs = 10ms
	 *
	 * Even these values are rounded off, and do not guarantee perfect
	 * precision. However, they are as precise as it gets.
	 *
	 * The user can specify any number of nanoseconds in the timeS structure
	 * passed as an argument to setOneshotMode(). However, the i8254 cannot
	 * physically time any number of nanoseconds, because the COUNTER reg
	 * takes a 16 bit value, which limits the number of CLKs that can be
	 * counted down in one i8254 oneshot run.
	 *
	 * Furthermore, since the i8254's input CLK source runs FASTER than
	 * a power-of-ten value, it also means that 20 ns is NOT EQUAL to 20 CLK
	 * pulses; consequentially, 65,535 ns is NOT EQUAL to 65,535 CLK pulses
	 * either. While we can program the i8254 to count down a maximum of
	 * 65535 CLK pulses before interrupting, we cannot give the user an
	 * accurate guarantee of 65535 ns. In reality the cap on the value
	 * accepted is approximately 54,928 ns based on the following table:
	 *		10000 * 6 = 11,932 * 5
	 *		1000 * 5 = 1193 * 4
	 *		100 * 5 = 119 * 9
	 *		10 * 3 = 12 * 2
	 *		1 * 6 = 1 * 8
	 *
	 * Basically, 54,928 nanoseconds would take approx 65536 CLK pulses
	 * to timeout. So we define the maximum number of nanoseconds the i8254
	 * will accept as an argument to be 54928, (i8254_ONESHOT_MAX_NS) and
	 * NOT 65535 (which is the max timeout value for the 16 bit counter).
	 **/
	if (time.seconds > 0 || time.nseconds > i8254_ONESHOT_MAX_NS) {
		return 0;
	};

	return 1;
}

inline static ubit16 nanosecondsToClks(ubit32 ns)
{
	ubit32		ret;

	ret = (ns / 10000) * i8254_ONESHOT_NS2CLK_10K;
	ns -= (ns / 10000) * 10000;

	ret += (ns / 1000) * i8254_ONESHOT_NS2CLK_1K;
	ns -= (ns / 1000) * 1000;

	ret += (ns / 100) * i8254_ONESHOT_NS2CLK_100;
	ns -= (ns / 100) * 100;

	ret += (ns / 10) * i8254_ONESHOT_NS2CLK_10;
	ns -= (ns / 10) * 10;

	ret += ns * i8254_ONESHOT_NS2CLK_1;
	return ret;
}

status_t i8254PitC::setPeriodicMode(struct timeS interval)
{
	if (!validateCallerIsLatched()) { return ERROR_RESOURCE_BUSY; };
	// Make sure the requested periodic interval is supported by the i8254.
	if (!validateTimevalLimit(interval)) { return ERROR_UNSUPPORTED; };

	state.lock.acquire();

	state.rsrc.currentInterval.nseconds = interval.nseconds;
	state.rsrc.mode = PERIODIC;
	// Convert the periodic interval into i8254 CLK pulse equivalent.
	currentIntervalClks = nanosecondsToClks(
		state.rsrc.currentInterval.nseconds);
	
	state.lock.release();
	return ERROR_SUCCESS;
}

status_t i8254PitC::setOneshotMode(struct timeS timeout)
{
	if (!validateCallerIsLatched()) { return ERROR_RESOURCE_BUSY; };
	// Make sure the requested timeout is supported by the i8254.
	if (!validateTimevalLimit(timeout)) { return ERROR_UNSUPPORTED; };

	state.lock.acquire();

	state.rsrc.currentTimeout.nseconds = timeout.nseconds;
	state.rsrc.mode = ONESHOT;
	// Convert the currentTimeout nanosecond value into i8254 CLK pulses.
	currentTimeoutClks = nanosecondsToClks(
		state.rsrc.currentTimeout.nseconds);

	state.lock.release();
	return ERROR_SUCCESS;
}

void i8254PitC::writeOneshotIo(void)
{
	/**	NOTE:
	 * Interesting note here, Linux seems to use mode 4 for the oneshot
	 * timer (Software retriggerable strobe).
	 **/
	state.lock.acquire();

	// Write out the oneshot mode (mode 0) control byte.
	io::write8(
		i8254_CHAN0_IO_CONTROL,
		i8254_CHAN0_CONTROL_SELECT_COUNTER
		| i8254_CHAN0_CONTROL_MODE0_ONESHOT
		| i8254_CHAN0_CONTROL_COUNTER_WRITE_LOWHIGH);

	atomicAsm::memoryBarrier();

	// Write out the number of ns converted into i8254 CLK pulse equiv time.
	io::write8(
		i8254_CHAN0_IO_COUNTER,
		currentTimeoutClks & 0xFF);

	io::write8(
		i8254_CHAN0_IO_COUNTER,
		currentTimeoutClks >> 8);

	__KFLAG_SET(state.rsrc.flags, ZKCM_TIMERDEV_STATE_FLAGS_ENABLED);

	state.lock.release();
}

void i8254PitC::writePeriodicIo(void)
{
	state.lock.acquire();

	io::write8(
		i8254_CHAN0_IO_CONTROL,
		i8254_CHAN0_CONTROL_SELECT_COUNTER
		| i8254_CHAN0_CONTROL_MODE2_RATEGEN
		| i8254_CHAN0_CONTROL_COUNTER_WRITE_LOWHIGH);

	atomicAsm::memoryBarrier();

	// Write out the currently set periodic rate.
	io::write8(
		i8254_CHAN0_IO_COUNTER,
		currentIntervalClks & 0xFF);

	io::write8(
		i8254_CHAN0_IO_COUNTER,
		currentIntervalClks >> 8);

	__KFLAG_SET(state.rsrc.flags, ZKCM_TIMERDEV_STATE_FLAGS_ENABLED);

	state.lock.release();
}

/*uarch_t i8254PitC::getPrecisionDiscrepancyForPeriod(ubit32 period) { return 0; }*/

