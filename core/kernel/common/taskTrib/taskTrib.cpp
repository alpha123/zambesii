
#include <scaling.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kcxxlib/new>
#include <kernel/common/taskTrib/taskTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>


taskTribC::taskTribC(void)
{
	capacity = 0;
	load = 0;

	quantumClass[QUANTUMCLASS_NORMAL] = QUANTUMCLASS_NORMAL_INITVAL;
	quantumClass[QUANTUMCLASS_DRIVER] = QUANTUMCLASS_DRIVER_INITVAL;
	custQuantumClass.rsrc.arr = __KNULL;
	custQuantumClass.rsrc.nClasses = 0;
	deadQ.rsrc = __KNULL;
}

#if __SCALING__ >= SCALING_CC_NUMA
error_t taskTribC::schedule(taskS *task)
{
	ubit32		lowestLoad=0xFFFFFFFF;
	numaCpuBankC	*bestBank=__KNULL, curBank;

	// Go according to NUMA affinity.
	for (uarch_t i=0; i<task->localAffinity.cpuBanks.getNBits(); i++)
	{
		if (task->localAffinity.cpuBanks.testSingle(i))
		{
			curBank = numaTrib.getStream(i);
			if (curBank == __KNULL) { continue; };
			if (curBank->cpuBank.getLoad() < lowestLoad)
			{
				bestBank = curBank;
				lowestLoad = curBank->cpuBank.getLoad();
			};
		};
	};

	if (bestBank == __KNULL) {
		return ERROR_UNKNOWN;
	};

	return bestBank->cpuBank.schedule(task);
}
#endif

void taskTribC::updateCapacity(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		capacity += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		capacity -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		capacity = val;
		return;

	default: return;
	};
}

void taskTribC::updateLoad(ubit8 action, uarch_t val)
{
	switch (action)
	{
	case PROCESSTRIB_UPDATE_ADD:
		load += val;
		return;

	case PROCESSTRIB_UPDATE_SUBTRACT:
		load -= val;
		return;

	case PROCESSTRIB_UPDATE_SET:
		load = val;
		return;

	default: return;
	};
}

status_t taskTribC::createQuantumClass(utf8Char *name, prio_t prio)
{
	sarch_t		pos=-1;
	uarch_t		j;
	quantumClassS	*mem, *old;

	custQuantumClass.lock.acquire();

	for (uarch_t i=0; i<custQuantumClass.rsrc.nClasses; i++)
	{
		if (custQuantumClass.rsrc.arr[i].name == __KNULL) {
			pos = static_cast<sarch_t>( i );
		};
	};

	if (pos == -1)
	{
		mem = new quantumClassS[custQuantumClass.rsrc.nClasses + 1];
		if (mem == __KNULL)
		{
			custQuantumClass.lock.release();
			return ERROR_MEMORY_NOMEM;
		};

		memcpy(
			mem, custQuantumClass.rsrc.arr,
			sizeof(quantumClassS) * custQuantumClass.rsrc.nClasses);

		old = custQuantumClass.rsrc.arr;
		delete old;
		custQuantumClass.rsrc.arr = mem;
		pos = custQuantumClass.rsrc.nClasses;
		custQuantumClass.rsrc.nClasses++;
	};

	// FIXME: use soft->hard conversion here.
	custQuantumClass.rsrc.arr[pos].prio = prio;
	for (j=0; j<127 && *name; j++) {
		custQuantumClass.rsrc.arr[pos].name[j] = name[j];
	};
	custQuantumClass.rsrc.arr[pos].name[((j < 127) ? j:127)] = '\0';

	return pos;
}

void taskTribC::setClassQuantum(sarch_t qc, prio_t prio)
{
	custQuantumClass.lock.acquire();

	if (qc >= static_cast<sarch_t>( custQuantumClass.rsrc.nClasses ))
	{
		custQuantumClass.lock.release();
		return;
	};

	// FIXME: Use soft->hard prio conversion here.
	custQuantumClass.rsrc.arr[qc].prio = prio;

	custQuantumClass.lock.release();
}

void taskTribC::setTaskQuantumClass(processId_t id, sarch_t qc)
{
	custQuantumClass.lock.acquire();

	if (qc >= static_cast<sarch_t>( custQuantumClass.rsrc.nClasses ))
	{
		custQuantumClass.lock.release();
		return;
	};

	processTrib.getTask(id)->prio = &custQuantumClass.rsrc.arr[qc].prio;

	custQuantumClass.lock.release();
}


