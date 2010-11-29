
#include <kernel/common/process.h>

error_t processStreamC::initializeFirstThread(
	taskC *newTask, taskC *spawningTask,
	ubit8 schedPolicy, ubit8 prio, uarch_t flags
	)
{
	error_t		ret;

	// Affinity unconditionally inherited from containing process.
	ret = affinity::copyLocal(
		&newTask->localAffinity, localAffinity);

	if (ret != ERROR_SUCCESS) { return ret; };

	// Now deal with scheduling policy.
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET))
	{
		// Caller wants new process to use specific policy.
		newTask->schedPolicy = schedPolicy;
	}
	else if (__KFLAG_TEST(
		flags, SPAWNTHREAD_FLAGS_SCHEDPOLICY_STINHERIT))
	{
		// Caller wants new process to inherit from spawner.
		newTask->schedPolicy = spawningTask->schedPolicy;
	}
	else
	{
		// Set new process policy to system default.
		newTask->schedPolicy = SCHEDPOLICY_ROUND_ROBIN;
	};

	// And now for the scheduling priority.
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_SET))
	{
		// Caller wants new process to use specific priority.
		*newTask->schedPrio = prio;
	}
	else if (__KFLAG_TEST(
		flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_STINHERIT))
	{
		// Caller wants new process to inherit from spawner.
		if (spawningTask->schedPrio
			== &spawningTask->internalPrio)
		{
			*newTask->schedPrio = *spawningTask->schedPrio;
		}
		else {
			newTask->schedPrio = spawningTask->schedPrio;
		};
	}
	else
	{
		// New process defaults to system default priority.
		*newTask->schedPrio = SCHEDPRIO_DEFAULT;
	};

	return ERROR_SUCCESS;
}

error_t processStreamC::initializeChildThread(
	taskC *newTask, taskC *spawningTask,
	ubit8 schedPolicy, ubit8 prio, uarch_t flags
	)
{
	error_t		ret;

	// Deal with affinity inheritance first.
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT))
	{
		// Inherit from containing process.
		ret = affinity::copyLocal(
			&newTask->localAffinity, localAffinity);

	}
	else
	{
		// Inherit from spawning thread.
		ret = affinity::copyLocal(
			&newTask->localAffinity,
			&spawningTask->localAffinity);
	};
	if (ret != ERROR_SUCCESS) { return ret; };

	// Now deal with scheduling policy inheritance.
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET))
	{
		// Caller has passed specific scheduling policy.
		newTask->schedPolicy = schedPolicy;
	}
	else if (__KFLAG_TEST(
		flags, SPAWNTHREAD_FLAGS_SCHEDPOLICY_DEFAULT))
	{
		// Caller wants thread to have default policy.
		newTask->schedPolicy = SCHEDPOLICY_ROUND_ROBIN;
	}
	else
	{
		// Thread inherits spawner's policy by default.
		newTask->schedPolicy = spawningTask->schedPolicy;
	};

	// And now scheduling priority inheritance.
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_SET))
	{
		// Caller wants specific priority.
		*newTask->schedPrio = prio;
	}
	else if (__KFLAG_TEST(
		flags, SPAWNTHREAD_FLAGS_SCHEDPRIO_DEFAULT))
	{
		// Caller wants thread to have default priority.
		*newTask->schedPrio = SCHEDPRIO_DEFAULT;
	}
	else
	{
		// Thread inherits spawner's priority by default.
		if (spawningTask->schedPrio
			== &spawningTask->internalPrio)
		{
			*newTask->schedPrio = *spawningTask->schedPrio;
		}
		else {
			newTask->schedPrio = spawningTask->schedPrio;
		};
	};

	return ERROR_SUCCESS;
}
