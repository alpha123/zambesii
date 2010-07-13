
#include <asm/cpuControl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <kernel/common/multipleReaderLock.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

void multipleReaderLockC::readAcquire(uarch_t *_flags)
{
	readerCount.lock.acquire();

	// Save the thread's current IRQ state.
	*_flags = readerCount.lock.flags;
#if __SCALING__ >= SCALING_SMP
	readerCount.rsrc++;
#endif

	readerCount.lock.releaseNoIrqs();

	cpuTrib.getCurrentCpuStream()->currentTask->nLocksHeld++;
}

void multipleReaderLockC::readRelease(uarch_t _flags)
{
#if __SCALING__ >= SCALING_SMP
	// See if there is a writer waiting on the lock.
	if (!__KFLAG_TEST(lockC::flags, MRLOCK_FLAGS_WRITE_REQUEST))
	{
		/* If there is no writer waiting, acquire the lock and then
		 * decrement. Else just decrement.
		 **/
		readerCount.lock.acquire();
		readerCount.rsrc--;
		readerCount.lock.releaseNoIrqs();
	}
	else
	{
		// If there is a writer, don't acquire the lock. Just decrement.
		readerCount.rsrc--;
	};
#endif

	cpuTrib.getCurrentCpuStream()->currentTask->nLocksHeld--;

	// Test the flags and see whether or not to enable IRQs.
	if (__KFLAG_TEST(_flags, LOCK_FLAGS_IRQS_WERE_ENABLED)) {
		cpuControl::enableInterrupts();
	};
}

void multipleReaderLockC::writeAcquire(void)
{
	// Acquire the readerCount lock.
	readerCount.lock.acquire();

#if __SCALING__ >= SCALING_SMP
	__KFLAG_SET(flags, MRLOCK_FLAGS_WRITE_REQUEST);

	/* Spin on reader count until there are no readers left on the
	 * shared resource.
	 **/
	while (readerCount.rsrc != 0) {
		cpuControl::subZero();
	};

	/* Since we don't release the lock before returning, the waitLock holds
	 * IRQ state for us already.
	 **/
#endif
}

void multipleReaderLockC::writeRelease(void)
{
	/* Do not attempt to acquire the lock. We already hold it. Just unset
	 * the write request flag and release the lock.
	 **/
#if __SCALING__ >= SCALING_SMP
	__KFLAG_UNSET(flags, MRLOCK_FLAGS_WRITE_REQUEST);
#endif

	readerCount.lock.release();
}
