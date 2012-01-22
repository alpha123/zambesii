
#include <__ksymbols.h>
#include <arch/arch.h>
#include <chipset/zkcm/memoryDetection.h>
#define __ASM__
#include <platform/memory.h>
#undef __ASM__
#include <firmware/ibmPcBios/ibmPcBios_coreFuncs.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <commonlibs/libacpi/libacpi.h>
#include "memoryDetection.h"


// E820 definitions for ibmPc_memoryMod_getMemoryMap().
#define E820_USABLE		0x1
#define E820_RECLAIMABLE	0x3

struct e820EntryS
{
	ubit32	baseLow;
	ubit32	baseHigh;
	ubit32	lengthLow;
	ubit32	lengthHigh;
	ubit32	type;
	ubit32	acpiExt;
};

static e820EntryS		*e820Ptr;
static zkcmMemMapS		*_mmap=__KNULL;
static zkcmMemConfigS		*_mcfg=__KNULL;

error_t ibmPc_memoryMod_initialize(void)
{
	return ibmPcBios::initialize();
}

error_t ibmPc_memoryMod_shutdown(void)
{
	return ibmPcBios::shutdown();
}

error_t ibmPc_memoryMod_suspend(void)
{
	return ERROR_SUCCESS;
}

error_t ibmPc_memoryMod_restore(void)
{
	return ERROR_SUCCESS;
}

static zkcmNumaMapS *ibmPc_mMod_gnm_rGnm(void)
{
	zkcmNumaMapS		*ret;
	acpi_rsdtS		*rsdt;
	acpi_rSratS		*srat;
	acpi_rSratMemS		*memEntry;
	void			*handle, *sratHandle, *context;
	ubit32			nEntries=0, currEntry=0;

	/**	EXPLANATION:
	 * Very simple: uses the kernel libACPI to get a list of NUMA memory
	 * nodes on the chipset, then transliterates them into a chipset-
	 * independent node map, and returns it to the caller.
	 *
	 * Only difference between this and ibmPc_mMod_gnm_xGnm() is that this
	 * function uses the RSDT and its accompanying tables, while the
	 * other function uses the XSDT and its accompanying tables.
	 **/

	// LibACPI has determined that there is an RSDT present.
	if (acpi::mapRsdt() != ERROR_SUCCESS) {	return __KNULL; };
	rsdt = acpi::getRsdt();

	// Now look for an SRAT entry in the RSDT.
	context = handle = __KNULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	// First run: find out how many entries exist.
	while (srat != __KNULL) 
	{
		sratHandle = __KNULL;
		memEntry = acpiRSrat::getNextMemEntry(srat, &sratHandle);
		for (; memEntry != __KNULL;
			memEntry = acpiRSrat::getNextMemEntry(
				srat, &sratHandle))
		{
			nEntries++;
		};

		acpiRsdt::destroySdt((acpi_sdtS *)srat);
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	};

	ret = new zkcmNumaMapS;
	if (ret == __KNULL) { return __KNULL; };
	ret->nCpuEntries = 0;
	ret->cpuEntries = __KNULL;

	ret->memEntries = new numaMemMapEntryS[nEntries];
	if (ret->memEntries == __KNULL)
	{
		delete ret;
		return __KNULL;
	};

	context = handle = __KNULL;
	srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);

	// Second run: Fill out kernel NUMA map.
	while (srat != __KNULL)
	{
		sratHandle = __KNULL;
		memEntry = acpiRSrat::getNextMemEntry(srat, &sratHandle);
		for (; memEntry != __KNULL && currEntry < nEntries;
			memEntry = acpiRSrat::getNextMemEntry(
				srat, &sratHandle))
		{
#ifdef __32_BIT__
			if (!(memEntry->baseHigh == 0
				&& memEntry->lengthHigh == 0))
			{
				continue;
			};
#endif
			ret->memEntries[currEntry].baseAddr
				= memEntry->baseLow
#ifndef __32_BIT__
				| (memEntry->baseHigh << 32)
#endif
				;
			ret->memEntries[currEntry].size
				= memEntry->lengthLow
#ifndef __32_BIT__
				| (memEntry->lengthHigh << 32)
#endif
				;

			ret->memEntries[currEntry].bankId =
				memEntry->domain0
				| (memEntry->domain1 << 16);

			if (__KFLAG_TEST(
				memEntry->flags,
				ACPI_SRAT_MEM_FLAGS_ENABLED))
			{
				__KFLAG_SET(
					ret->memEntries[currEntry].flags,
					NUMAMEMMAP_FLAGS_ONLINE);
			};

			if (__KFLAG_TEST(
				memEntry->flags,
				ACPI_SRAT_MEM_FLAGS_HOTPLUG))
			{
				__KFLAG_SET(
					ret->memEntries[currEntry].flags,
					NUMAMEMMAP_FLAGS_HOTPLUG);
			};

			currEntry++;
		};

		acpiRsdt::destroySdt((acpi_sdtS *)srat);
		srat = acpiRsdt::getNextSrat(rsdt, &context, &handle);
	};

	ret->nMemEntries = currEntry;
	return ret;
}

zkcmNumaMapS *ibmPc_memoryMod_getNumaMap(void)
{
	error_t		err;
	zkcmNumaMapS	*ret=0;

	// Get NUMA map from ACPI.
	acpi::initializeCache();
	err = acpi::findRsdp();
	if (err != ERROR_SUCCESS) { return __KNULL; };

// Only try to use XSDT for 64-bit and upwards.
#ifndef __32_BIT__
/*	if (acpi::testForXsdt())
	{
		__kprintf(NOTICE"getNumaMap: Using XSDT.\n");
		// Prefer XSDT to RSDT if found.
		ret = ibmPc_mMod_gnm_xGnm();
		if (ret != __KNULL) { return ret; };
	}; */
#endif
	// Else use RSDT.
	if (acpi::testForRsdt())
	{
		__kprintf(NOTICE"getNumaMap: No XSDT; falling back to RSDT.\n");
		ret = ibmPc_mMod_gnm_rGnm();
	};

	return ret;
}

#ifdef __32_BIT__
	#ifdef CONFIG_ARCH_x86_32_PAE
		// For PAE, baseHigh is allowed to have up to 4 bits set.
		#define IBMPCMMAP_ADDRHIGH_BADMASK	0xFFFFFFF0
	#else
		// Else, no bits in baseHigh should be set (==0) for 32-bit.
		#define IBMPCMMAP_ADDRHIGH_BADMASK	0xFFFFFFFF
	#endif
#else
	// For 64-bit, of course, baseHigh is completely fair game.
	#define IBMPCMMAP_ADDRHIGH_BADMASK	0
#endif

zkcmMemMapS *ibmPc_memoryMod_getMemoryMap(void)
{
	zkcmMemMapS		*ret;
	ubit32			nEntries=0, i, j;

	/**	EXPLANATION:
	 * Calls on the functions provided in the IBM-PC BIOS code to execute
	 * firmware interrupts on the board via the x86Emu library.
	 *
	 * This function executes INT 0x15(AH=0x0000E820).
	 *
	 * The function caches the result in a file-local variable. If the
	 * kernel asks for the memory map subsequently, it will get the old one
	 * back.
	 **/
	// If memory map was previously obtained, return the cached one.
	if (_mmap != __KNULL) { return _mmap; };

	ret = new zkcmMemMapS;
	if (ret == __KNULL)
	{
		__kprintf(ERROR"Failed to alloc memMap main structure.\n");	
		return __KNULL;
	};

	// Find out how many E820 entries there are.
	ibmPcBios::acquireLock();

	// Buffer is placed into 0x1000 in lowmem.
	e820Ptr = (struct e820EntryS *)(M.mem_base + 0x1000);
	ibmPcBios_regs::setEdi(0x00001000);
	ibmPcBios_regs::setEax(0x0000E820);
	ibmPcBios_regs::setEbx(0);
	ibmPcBios_regs::setEcx(24);
	ibmPcBios_regs::setEdx(0x534D4150);

	ibmPcBios::executeInterrupt(0x15);

	while ((ibmPcBios_regs::getEax() == 0x534D4150)
		&& !__KFLAG_TEST(ibmPcBios_regs::getEflags(), (1<<0)))
	{
		nEntries++;
		if (ibmPcBios_regs::getEbx() == 0) {
			break;
		};

		ibmPcBios_regs::setEax(0x0000E820);
		ibmPcBios_regs::setEcx(24);
		ibmPcBios_regs::setEdi(ibmPcBios_regs::getEdi() + 24);
		ibmPcBios::executeInterrupt(0x15);
	};

	ibmPcBios::releaseLock();

	// Allocate enough space to hold them all, plus the extra 3.
	ret->entries = new zkcmMemMapEntryS[nEntries + 3];
	if (ret->entries == __KNULL)
	{
		__kprintf(ERROR"Failed to alloc space for mem map entries.\n");
		delete ret;
		return __KNULL;
	};

	// Generate the kernel's generic map from the E820.
	// 'i' indexes into the E820 map, and 'j' indexes into the generic map.
	for (i=0, j=0; i<nEntries; i++)
	{
		if (e820Ptr[i].baseHigh & IBMPCMMAP_ADDRHIGH_BADMASK) {
			continue;
		};

		ret->entries[j].baseAddr = e820Ptr[i].baseLow
// Only add the high 32 bits for 64-bit and above, and for PAE x86-32.
#if defined(CONFIG_ARCH_x86_32_PAE) || (!defined(__32_BIT__))
			| (e820Ptr[i].baseHigh << 32)
#endif
			;
		ret->entries[j].size = e820Ptr[i].lengthLow
#if defined(CONFIG_ARCH_x86_32_PAE) || (!defined(__32_BIT__))
			| (e820Ptr[i].lengthHigh << 32)
#endif
			;

		switch (e820Ptr[i].type)
		{
		case E820_USABLE:
			ret->entries[j].memType = ZKCM_MMAP_TYPE_USABLE;
			break;

		case E820_RECLAIMABLE:
			ret->entries[j].memType = ZKCM_MMAP_TYPE_RECLAIMABLE;
			break;

		default:
			ret->entries[j].memType = ZKCM_MMAP_TYPE_RESERVED;
			break;
		};
		j++;
	};

	__kprintf(NOTICE"getMemoryMap(): %d entries in firmware map.\n",
		nEntries);

	// Hardcode in IVT + BDA
	ret->entries[j].baseAddr = 0x0;
	ret->entries[j].size = 0x4FF;
	ret->entries[j].memType = ZKCM_MMAP_TYPE_RESERVED;

	// Hardcode entry for EBDA + BIOS + VGA framebuffer + anything else.
	ret->entries[j+1].baseAddr = 0x80000;
	ret->entries[j+1].size = 0x80000;
	ret->entries[j+1].memType = ZKCM_MMAP_TYPE_RESERVED;

	// This entry sets the kernel image in pmem as reserved.
	ret->entries[j+2].baseAddr = CHIPSET_MEMORY___KLOAD_PADDR_BASE;
	ret->entries[j+2].size = PLATFORM_MEMORY_GET___KSYMBOL_PADDR(__kend)
		- CHIPSET_MEMORY___KLOAD_PADDR_BASE;

	ret->entries[j+2].memType = ZKCM_MMAP_TYPE_RESERVED;

	__kprintf(NOTICE"getMemoryMap(): Kernel phys: base 0x%P, size 0x%P.\n",
		ret->entries[j+2].baseAddr,
		ret->entries[j+2].size);

	ret->nEntries = j + 3;
	_mmap = ret;

	return ret;
}

zkcmMemConfigS *ibmPc_memoryMod_getMemoryConfig(void)
{
	zkcmMemConfigS		*ret;
	uarch_t			ax, bx, cx, dx;
	ubit32			highest=0;

	/**	EXPLANATION:
	 * Prefer to derive the size of memory from the E820 map. If you can't
	 * get an E820 map, then use the firmware's INT 0x15(AH=0x0000E801).
	 **/
	if (_mcfg != __KNULL) { return _mcfg; };
	ret = new zkcmMemConfigS;
	if (ret == __KNULL)
	{
		__kprintf(ERROR"getMemoryConfig: Failed to alloc config.\n");
		return __KNULL;
	};

	if (_mmap == __KNULL)
	{
		ibmPc_memoryMod_getMemoryMap();
		if (_mmap == __KNULL) { goto useE801; };
	};

	for (ubit32 i=0; i<_mmap->nEntries; i++)
	{
		if ((_mmap->entries[i].baseAddr
			> _mmap->entries[highest].baseAddr)
			&& _mmap->entries[i].memType
				== ZKCM_MMAP_TYPE_USABLE)
		{
			highest = i;
		};
	};

	if (_mmap->entries[highest].memType != ZKCM_MMAP_TYPE_USABLE)
	{
		goto useE801;
	};

	ret->memBase = 0x0;
	ret->memSize = _mmap->entries[highest].baseAddr
		+ _mmap->entries[highest].size;

	return ret;

useE801:

	ibmPcBios::acquireLock();

	ibmPcBios_regs::setEax(0x0000E801);
	ibmPcBios::executeInterrupt(0x15);

	ax = ibmPcBios_regs::getEax();
	bx = ibmPcBios_regs::getEbx();
	cx = ibmPcBios_regs::getEcx();
	dx = ibmPcBios_regs::getEdx();

	ibmPcBios::releaseLock();

	ret->memBase = 0x0;
	if (ax == 0)
	{
		ret->memSize = 0x100000 + (cx << 10);
		ret->memSize += dx << 16;
	}
	else
	{
		ret->memSize = 0x100000 + (ax << 10);
		ret->memSize += bx << 16;
	}

	return ret;
}
