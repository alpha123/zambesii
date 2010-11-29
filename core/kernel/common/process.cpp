
#include <chipset/chipset.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string8.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>
#include <kernel/common/taskTrib/taskTrib.h>


processStreamC::processStreamC(processId_t processId, processId_t parentProcId)
:
id(processId), parentId(parentProcId), nextTaskId(CHIPSET_MEMORY_MAX_NTASKS - 1)
{
	if (initMagic == PROCESS_INIT_MAGIC) {
		return;
	};

	memset(tasks, 0, sizeof(tasks));

	absName = argString = env = __KNULL;
	localAffinity = __KNULL;
	memoryStream = __KNULL;
}

error_t processStreamC::initialize(utf8Char *fileAbsName)
{
	ubit32		nameLen;
	error_t		ret;

	nameLen = strlen8(fileAbsName);
	absName = new utf8Char[nameLen + 1];
	if (absName == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	// Initialize cpuTrace to the number of bits in Memory Trib bmp.
	ret = cpuTrace.initialize(cpuTrib.onlineCpus.getNBits());
	if (ret != ERROR_SUCCESS) {
		return ret;
	};

	// Allocate Memory Stream.
	memoryStream = new memoryStreamC(
		id,
		reinterpret_cast<void *>( 0x1000 ), 0x80000000, __KNULL,
		__KNULL, static_cast<paddr_t>( 0 ));

	if (memoryStream == __KNULL) {
		return ERROR_MEMORY_NOMEM;
	};

	return ERROR_SUCCESS;
}

processStreamC::~processStreamC(void)
{
	if (absName != __KNULL) {
		delete absName;
	};
	if (memoryStream != __KNULL) {
		delete memoryStream;
	};
}

error_t processStreamC::initializeChild(processStreamC *child)
{
	error_t		ret;
	ubit32		len;

	len = strlen8(env);
	child->env = new utf8Char[len + 1];
	if (child->env == __KNULL) { return ERROR_MEMORY_NOMEM; };
	strcpy8(child->env, env);

	ret = affinity::copyMachine(&child->affinity, &affinity);
	if (ret != ERROR_SUCCESS) { return ret; };
	// child->localAffinity = affinity::findLocal(&child->affinity);

	child->execDomain = execDomain;
	return ERROR_SUCCESS;
}

error_t processStreamC::spawnThread(
	void *entryPoint, ubit8 schedPolicy, ubit8 prio, uarch_t flags
	)
{
	sarch_t		threadId;
	taskC		*newTask, *spawningTask;
	error_t		ret;

	// Check for a free thread ID in this process's thread ID space.
	threadId = getNextThreadId();
	if (threadId == -1) { return SPAWNTHREAD_STATUS_NO_PIDS; };

	// Allocate new thread if ID was available.
	newTask = allocateNewThread(this->id | threadId);
	if (newTask == __KNULL) { return ERROR_MEMORY_NOMEM; };

__kprintf(NOTICE"New thread id 0x%x, v 0x%p.\n", newTask->id, newTask);

	// Allocate internal sub-structures (context, etc.).
	ret = newTask->initialize();
	if (ret != ERROR_SUCCESS) { return ret; };
__kprintf(NOTICE"Initialize() on new thread was successful.\n");

	/**	EXPLANATION:
	 * For any thread other than the first, the kernel has a fixed attribute
	 * inheritance model.
	 *
	 * Threads inherit affinity from the thread that spawned them, unless
	 * the spawner passed SPAWNTHREAD_FLAGS_AFFINITY_PINHERIT. That flag
	 * tells the kernel to give the new thread its process global affinity
	 * instead of the affinity of its spawning thread.
	 *
	 * Threads inherit scheduling parameters from the the thread that
	 * spawned them, unless the spawner passes
	 * SPAWNTHREAD_FLAGS_SCHEDPARAMS_SET, which tells the kernel to set the
	 * new thread's scheduling parameters to those passed in the syscall.
	 *
	 * When the thread being spawned is the first in the process, the kernel
	 * uses a different inheritance model.
	 *
	 * The first thread in a process inherits its affinity attributes from
	 * its process stream unconditionally. There is no modifier to change
	 * the way that the fist thread in a process inherits affinity.
	 * This is because in the syscall for process spawning, there are
	 * arguments which allow for inheritance modification of the spawned
	 * process. Therefore the first thread in a process must follow those
	 * passed modifiers.
	 *
	 * The first thread in a process will be set to the scheduling 
	 * parameters passed in the syscall that created the process. If none
	 * were passed, the scheduling parameters will default to system
	 * defaults.
	 **/

	spawningTask = cpuTrib.getCurrentCpuStream()->currentTask;
	if (__KFLAG_TEST(flags, SPAWNTHREAD_FLAGS_FIRST_THREAD))
	{
	__kprintf(NOTICE"Branch taken for firstThread.\n");
		ret = initializeFirstThread(
			newTask, spawningTask, schedPolicy, prio, flags);
	}
	else
	{
	__kprintf(NOTICE"Branch taken for childThread.\n");
		ret = initializeChildThread(
			newTask, spawningTask, schedPolicy, prio, flags);
	};
	if (ret != ERROR_SUCCESS) { return ret; };
__kprintf(NOTICE"Initialize[First|Child]Thread called successfully.\n");

	// New task has inherited as needed. Initialize register context.
	taskContext::initialize(newTask->context, newTask, entryPoint);
	ret = taskTrib.schedule(newTask);
__kprintf(NOTICE"Ret for schedule. %d.\n", ret);
	return ret;
}

extern int oo;

taskC *processStreamC::allocateNewThread(processId_t id)
{
	taskC		*ret;
 
	ret = new taskC(id, this);
	if (ret == __KNULL) { return __KNULL; };

	// Add the new thread to the process's task array.
	tasks[PROCID_THREAD(id)] = ret;
	return ret;
}

void processStreamC::removeThread(processId_t id)
{
	if (tasks[PROCID_THREAD(id)] != __KNULL)
	{
		delete tasks[PROCID_THREAD(id)];
		tasks[PROCID_THREAD(id)] = __KNULL;
	};
}

