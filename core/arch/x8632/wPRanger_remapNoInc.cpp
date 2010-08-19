
#include <arch/tlbControl.h>
#include <arch/paging.h>
#include <arch/walkerPageRanger.h>
#include <arch/x8632/wPRanger_accessors.h>
#include <arch/x8632/wPRanger_getLevelRanges.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/process.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

void walkerPageRanger::remapNoInc(
	vaddrSpaceC *vaddrSpace,
	void *vaddr, paddr_t paddr, uarch_t nPages,
	ubit8 op, uarch_t __kflags
	)
{
	uarch_t		l0Start, l0Current, l0End;
	uarch_t		l1Start, l1Current, l1Limit, l1End;
	paddr_t		l0Entry, l1Entry;
#ifdef CONFIG_ARCH_x86_32_PAE
	uarch_t		l2Start, l2Current, l2Limit, l2End;
	paddr_t		l2Entry;
#endif
	uarch_t		archFlags;

	if (nPages == 0) { return; };

	archFlags = walkerPageRanger::encodeFlags(__kflags);
	getLevelRanges(
		vaddr, nPages,
		&l0Start, &l0End,
		&l1Start, &l1End
#ifdef CONFIG_ARCH_x86_32_PAE
		,&l2Start, &l2End
#endif
	);

	// This is SRS BSNS. Lock off both address spaces.
	vaddrSpace->level0Accessor.lock.acquire();
	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.acquire();

__kprintf(NOTICE"WPRr: l0s %d l0e %d l1s %d l1e %d v %X np %X op %d __kf %X.\n",
	l0Start, l0End, l1Start, l1End, vaddr, nPages, op, __kflags);

	l0Current = l0Start;
	for (; l0Current <= l0End; l0Current++)
	{
		l0Entry = vaddrSpace->level0Accessor.rsrc->entries[l0Current];
		*level1Modifier = l0Entry;
__kprintf(NOTICE"WPRr: l1Mod %X.\n", *level1Modifier);
		tlbControl::flushSingleEntry((void *)level1Accessor);

		l1Current = ((l0Current == l0Start) ? l1Start : 0);
		l1Limit = ((l0Current == l0End)
			? l1End : (PAGING_L1_NENTRIES - 1));

		for (; l1Current <= l1Limit; l1Current++)
		{
#ifdef CONFIG_ARCH_x86_32_PAE
			l1Entry = level1Accessor->entries[l1Current];
			*level2Modifier = l1Entry;
			tlbControl::flushSingleEntry((void *)level2Accessor);

			l2Current = (((l0Current == l0Start)
				&& (l1Current == l1Start)) ? l2Start : 0);

			l2Limit = (((l0Current == l0End)
				&& (l1Current == l1End))
				? l2End : (PAGING_L2_NENTRIES - 1))

			for (; l2Current < l2Limit; l2Current++)
			{
				l2Entry = level2Accessor->entries[l2Current]
					& 0xFFF;

				level2Accessor->entries[l2Current] = 0;
				level2Accessor->entries[l2Current] |= l2Entry;

				switch (op)
				{
				case WPRANGER_OP_SET:
				{
					__KFLAG_SET(
						level2Accessor
							->entries[l2Current],
						archFlags);

					break;
				};
				case WPRANGER_OP_CLEAR:
				{
					__KFLAG_UNSET(
						level2Accessor
							->entries[l2Current],
						archFlags);

					break;
				};
				case WPRANGER_OP_SET_PRESENT:
				{
					__KFLAG_SET(
						level2Accessor
							->entries[l2Current],
						PAGING_L2_PRESENT);

					break;
				};
				case WPRANGER_OP_CLEAR_PRESENT:
				{
					__KFLAG_UNSET(
						level2Accessor
							->entries[l2Current],
						PAGING_L2_PRESENT);

					break;
				};
				case WPRANGER_OP_SET_WRITE:
				{
					__KFLAG_SET(
						level2Accessor
							->entries[l2Current],
						PAGING_L2_WRITE);

					break;
				};
				case WPRANGER_OP_CLEAR_WRITE:
				{
					__KFLAG_UNSET(
						level2Accessor
							->entries[l2Current],
						PAGING_L2_WRITE);

					break;
				};
				default: break;
				};

				level2Accessor->entries[l2Current] |= paddr;
			};
#else
			l1Entry = level1Accessor->entries[l1Current] & 0xFFF;
			level1Accessor->entries[l1Current] = 0;
			level1Accessor->entries[l1Current] |= l1Entry;

			switch (op)
			{
			case WPRANGER_OP_SET:
			{
				__KFLAG_SET(
					level1Accessor->entries[l1Current],
					archFlags);

				break;
			};
			case WPRANGER_OP_CLEAR:
			{
				__KFLAG_UNSET(
					level1Accessor->entries[l1Current],
					archFlags);

				break;
			};
			case WPRANGER_OP_SET_PRESENT:
			{
				__KFLAG_SET(
					level1Accessor
						->entries[l1Current],
					PAGING_L1_PRESENT);

				break;
			};
			case WPRANGER_OP_CLEAR_PRESENT:
			{
				__KFLAG_UNSET(
					level1Accessor
						->entries[l1Current],
					PAGING_L1_PRESENT);

				break;
			};
			case WPRANGER_OP_SET_WRITE:
			{
				__KFLAG_SET(
					level1Accessor
						->entries[l1Current],
					PAGING_L1_WRITE);

				break;
			};
			case WPRANGER_OP_CLEAR_WRITE:
			{
				__KFLAG_UNSET(
					level1Accessor
						->entries[l1Current],
					PAGING_L1_WRITE);

				break;
			};
			default:
				break;
			};

			level1Accessor->entries[l1Current] |= paddr;
#endif
		};
	};

	vaddrSpace->level0Accessor.lock.release();
	cpuTrib.getCurrentCpuStream()->currentTask->parent->memoryStream
		->vaddrSpaceStream.vaddrSpace.level0Accessor.lock.release();

	// Flush all the addresses in the range from the TLB.
	tlbControl::flushEntryRange(vaddr, nPages);

	/*	FIXME:
	 * Insert code here to propagate possible changes to the kernel
	 * address space into the other process address spaces.
	 **/

	// We make the assumption that this did not fail.
}

