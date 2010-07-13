#ifndef _TASK_H
	#define _TASK_H

	#include <arch/tlbContext.h>
	#include <__kstdlib/__ktypes.h>
	#include <__kclasses/bitmap.h>
	#include <kernel/common/smpConfig.h>
	#include <kernel/common/numaConfig.h>

#define TASK_FLAGS_SCHED_WAITING	(1<<0)

struct processS;

struct taskS
{
	taskS		*next;
	uarch_t		id;
	uarch_t		flags;
	ubit16		nLocksHeld;
	processS	*parent;
	smpConfigS	smpConfig;
	numaConfigS	numaConfig;
#ifdef CONFIG_PER_TASK_TLB_CONTEXT
	tlbContextS	*tlbContext;
#endif
	bitmapC		cpuTrace;
};

#endif
