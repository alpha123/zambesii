
#include <arch/cpuControl.h>
#include <arch/tlbControl.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

#if __SCALING__ >= SCALING_SMP
cpuStreamC::interCpuMessagerC::interCpuMessagerC(cpuStreamC *parent)
:
parent(parent)
{
	cache = __KNULL;
	statusFlag.rsrc = CPUMSGR_STATUS_NORMAL;
}

error_t cpuStreamC::interCpuMessagerC::initialize(void)
{
	messageQueue.initialize();
	statusFlag.rsrc = CPUMSGR_STATUS_NORMAL;

	// Create an object cache for the messages.
	cache = cachePool.getCache(sizeof(messageS));
	if (cache == __KNULL)
	{
		cache = cachePool.createCache(sizeof(messageS));
		if (cache == __KNULL)
		{
			__kprintf(ERROR CPUMSG"%d: initialize(): Failed "
				"to create an object cache for messages.\n",
				parent->cpuId);

			return ERROR_CRITICAL;
		};
	};

	return ERROR_SUCCESS;
}

error_t cpuStreamC::interCpuMessagerC::bind(void)
{
	/*	NOTE:
	 * Signifies that this CPU is now "in synch" with the kernel and usable.
	 * The reason this must only be set after the CPU is ready to receive
	 * IPIs is because if it is set any earlier, there is a likelihood that
	 * since the inter-cpu-messager lock is initialized in the "locked"
	 * state, any CPU which allocates memory and needs to broadcast a
	 * "TLB flush" message will deadlock on this CPU's message lock.
	 *
	 * If however, this CPU's message lock isn't set in the global
	 * "availableCpus" BMP, that likelihood is erased, since no CPU will
	 * try to send messages to this CPU if its bit isn't set.
	 **/
	__kprintf(NOTICE CPUMSG"%d: Binding.\n", parent->cpuId);
	cpuControl::enableInterrupts();
	cpuTrib.availableCpus.setSingle(parent->cpuId);

	return ERROR_SUCCESS;
}

void cpuStreamC::interCpuMessagerC::cut(void)
{
	// TODO: May need to process all pending messages.
	__kprintf(NOTICE CPUMSG"%d: Cut.\n", parent->cpuId);
	cpuTrib.availableCpus.unsetSingle(parent->cpuId);
	cpuControl::disableInterrupts();
}

void cpuStreamC::interCpuMessagerC::set(
	messageS *msg, ubit8 type,
	uarch_t val0, uarch_t val1, uarch_t val2, uarch_t val3
	)
{
	msg->type = type;
	msg->val0 = val0;
	msg->val1 = val1;
	msg->val2 = val2;
	msg->val3 = val3;
}

error_t cpuStreamC::interCpuMessagerC::dispatch(void)
{
	messageS	*msg;

	msg = messageQueue.pop();
	while (msg != __KNULL)
	{
		switch (msg->type)
		{
		case CPUMESSAGE_TYPE_TLBFLUSH:
			tlbControl::flushEntryRange(
				(void *)msg->val0,
				msg->val1);

			break;

		default:
			__kprintf(ERROR CPUMSG"%d: Unknown message type %d.\n",
				parent->cpuId, msg->type);
		};

		cache->free(msg);
		msg = messageQueue.pop();
	};

	return ERROR_SUCCESS;
}

void cpuStreamC::interCpuMessagerC::setReceiveStateReady(void)
{
	statusFlag.lock.acquire();
	statusFlag.rsrc = CPUMSGR_STATUS_NORMAL;
	statusFlag.lock.release();
}
#endif /* if __SCALING__ >= SCALING_SMP */

