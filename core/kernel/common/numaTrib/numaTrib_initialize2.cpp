static void sortNumaMapByAddress(chipsetNumaMapS *map)
{
	numaMemMapEntryS	tmp;

	/** EXPLANATION:
	 * Simple one-pass swap sort algorithm. Recurses backward while sorting
	 * to avoid the need for multiple passes.
	 **/

	for (sarch_t i=0; i<static_cast<sarch_t>( map->nMemEntries - 1 ); )
	{		
		if (map->memEntries[i].baseAddr > map->memEntries[i+1].baseAddr)
		{
			memcpy(
				&tmp, &map->memEntries[i],
				sizeof(numaMemMapEntryS));

			memcpy(
				&map->memEntries[i], &map->memEntries[i+1],
				sizeof(numaMemMapEntryS));

			memcpy(
				&map->memEntries[i+1], &tmp,
				sizeof(numaMemMapEntryS));

			if (i != 0) { i--; };
			continue;
		};
		i++;
	};
}

/**	EXPLANATION:
 * Initialize2(): Detects physical memory on the chipset using firmware services
 * contained in the Firmware Tributary. If the person porting the kernel to
 * the current chipset did not provide a firmware interface, the kernel will
 * simply live on with only the __kspace bank; Of course, it should be obvious
 * that this is probably sub-optimal, but it can work.
 *
 * Generally, the kernel relies on the memInfoRiv to provide all memory
 * information. A prime example of all of this is the IBM-PC. For the IBM-PC,
 * the memInfoRiv is actually just a wrapper around the x86Emu library. x86Emu
 * Completely unbeknownst to the kernel, the IBM-PC support code initializes and
 * runs a full real mode emulator for memory detection. For NUMA detection, the
 * chipset firmware rivulet code will map low memory into the kernel's address
 * space and scan for the ACPI tables, trying to find NUMA information in the
 * SRAT/SLIT tables.
 *
 * To actually get the pmem information and numa memory layout information into
 * a usable state, the kernel must spawn a NUMA Stream for each detected bank,
 * and initialize it real-time.
 *
 * In Zambezii, a memory map is nothing more than something to overlay the
 * the real memory information. In other words, the last thing we logically
 * parse is a memory map. Our priority is:
 *	1. Find out about NUMA layout.
 *	2. Find out the total amount of memory.
 *
 *	   Take the following example: If a chipset is detailed to have 64MB of
 *	   RAM, yet the NUMA information describes only two nodes: (1) 0MB-8MB,
 *	   (2) 20MB-32MB, with a hole between 8MB and 20MB, and another hole
 *	   between 32MB and 64MB, we will spawn a third and fourth bank for the
 *	   two banks which were not described explicitly as NUMA banks, and
 *	   treat them as local memory to all NUMA banks. That is, we'll treat
 *	   those undescribed memory ranges as shared memory that is globally
 *	   the same distance from each node.
 *
 *	3. Find a memory map. When we know how much RAM there is, and also the
 *	   NUMA layout of this RAM, we can then pass through a memory map and
 *	   apply the information in the memory map to the NUMA banks. That is,
 *	   mark all reserved ranges as 'used' in the PMM info, and whatnot.
 *	   Note well that a memory map may be used as general memory information
 *	   suitable for use as requirement (1) above.
 *
 * ^ If NUMA information does not exist, and shared bank generation is not
 *   set in the config, Zambezii will assume no memory other than __kspace. If
 *   shbank is configured, then Zambezii will spawn a single bank for all of
 *   RAM as shared memory for all processes.
 *
 * ^ If a memory map is not found, and only a total figure for "amount of RAM"
 *   is given, Zambezii will assume that there is no reserved memory on the
 *   chipset and operate as if all RAM is available for use.
 *
 * ^ In the absence of a total figure for "amount of RAM", (where this figure
 *   may be provided explicitly, or derived from a memory map), Zambezii will
 *   simply assume that the only usable RAM is the __kspace RAM (bootmem), and
 *   continue to use that. When that runs out, that's that.
 **/
error_t numaTribC::initialize2(void)
{
	error_t			ret;
	chipsetMemConfigS	*memConfig=__KNULL;
	chipsetMemMapS		*memMap=__KNULL;
	chipsetNumaMapS		*numaMap=__KNULL;
	memInfoRivS		*memInfoRiv;
	numaStreamC		*ns;
	sarch_t			pos;
	paddr_t			tmpBase, tmpSize;
	status_t		nSet=0;

	/**	EXPLANATION:
	 * Now the NUMA Tributary is ready to check for new banks of memory,
	 * or for a non-NUMa build, generate a single "shared" NUMA bank as a
	 * fake bank 0 from which all threads will allocate.
	 *
	 * On both a NUMA and a non-NUMA build, the kernel will eradicate
	 * __kspace at this point. On a non-NUMA build however, if the kernel
	 * finds no extra memory, it will send out a nice, conspicuous warning
	 * and then assign __kspace to be the shared bank, and move on.
	 *
	 * On a NUMA build, if the kernel finds no NUMA memory config, then it
	 * will fall through to the check for shared memory. However, if the
	 * person porting the kernel to his or her chipset did not define
	 * CHIPSET_MEMORY_NUMA_GENERATE_SHBANK, then the kernel will just
	 * fall through, see that __kspace is the only remaining memory, and
	 * set that as the default bank, send out a nice highly conspicuous
	 * warning, and move on.
	 *
	 * In the event that CHIPSET_MEMORY_NUMA_GENERATE_SHBANK is defined,
	 * yet still no memory is found, the kernel will do the same as above.
	 *
	 * In the event that CHIPSET_MEMORY_NUMA_GENERATE_SHBANK is defined,
	 * and memory is found, the kernel will set that memory to be shbank,
	 * and the default bank, and then eradicate __kspace.
	 **/
	// Initialize both firmware streams.
	ret = (*chipsetFwStream.initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

	ret = (*firmwareFwStream.initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

	__kprintf(NOTICE NUMATRIB"Initialized Firmware and Chipset firmware "
		"streams.\n");

	// Fetch and initialize the Memory Info rivulet.
	memInfoRiv = firmwareTrib.getMemInfoRiv();
	assert_fatal(memInfoRiv != __KNULL);

	ret = (*memInfoRiv->initialize)();
	assert_fatal(ret == ERROR_SUCCESS);

	__kprintf(NOTICE NUMATRIB"Initialized Memory Information Rivulet.\n");

#if __SCALING__ >= SCALING_CC_NUMA
	numaMap = (*memInfoRiv->getNumaMap)();
	if (numaMap != __KNULL && numaMap->nMemEntries > 0)
	{
		__kprintf(NOTICE NUMATRIB"NUMA Map: %d entries.\n",
			numaMap->nMemEntries);

		for (uarch_t i=0; i<numaMap->nMemEntries; i++)
		{
			// If we've already spawned a stream for this bank:
			if (getStream(numaMap->memEntries[i].bankId)) {
				continue;
			};
			// Else allocate one.
			ret = spawnStream(numaMap->memEntries[i].bankId);
			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR NUMATRIB"Failed to spawn "
					"stream for detected bank %d.\n",
					numaMap->memEntries[i].bankId);

				continue;
			};
			__kprintf(NOTICE NUMATRIB"Spawned NUMA Stream for bank "
				"with ID %d.\n",
				numaMap->memEntries[i].bankId);
		};

		// Run through again, and this time spawn memory regions.
		for (uarch_t i=0; i<numaMap->nMemEntries; i++)
		{
			ns = getStream(numaMap->memEntries[i].bankId);
			if (ns == __KNULL)
			{
				__kprintf(WARNING NUMATRIB"Bank %d found in "
					"NUMA map, but it has no stream.\n",
					numaMap->memEntries[i].bankId);

				continue;
			};

			ret = ns->memoryBank.addMemoryRange(
				numaMap->memEntries[i].baseAddr,
				numaMap->memEntries[i].size);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR NUMATRIB"Failed to allocate "
					"memory range obj for range: base 0x%X "
					"size 0x%X on bank %d.\n",
					numaMap->memEntries[i].baseAddr,
					numaMap->memEntries[i].size,
					numaMap->memEntries[i].bankId);
			};
		};
	}
	else {
		__kprintf(WARNING NUMATRIB"getNumaMap(): no map.\n");
	};
#endif

#ifdef CHIPSET_MEMORY_NUMA_GENERATE_SHBANK
	memConfig = (*memInfoRiv->getMemoryConfig)();
	if (memConfig != __KNULL && memConfig->memSize > 0)
	{
		ret = spawnStream(CHIPSET_MEMORY_NUMA_SHBANKID);
		if (ret != ERROR_SUCCESS)
		{
			__kprintf(ERROR NUMATRIB"Failed to spawn shbank.\n");
			goto parseMemoryMap;
		};

		__kprintf(NOTICE NUMATRIB"Mem config: memsize 0x%X.\n",
			memConfig->memSize);

		if (numaMap != __KNULL && numaMap->nMemEntries-1 > 0)
		{
			// NUMA map exists: need to discover holes for shbank.
			sortNumaMapByAddress(numaMap);
			__kprintf(NOTICE NUMATRIB"Shbank: parsing NUMA map for "
				"holes.\n");

			for (sarch_t i=0;
				i<static_cast<sarch_t>( numaMap->nMemEntries )
					- 1;
				i++)
			{
				/* Shbank is only for where holes intersect with
				 * memoryConfig. i.e., if memSize is reported
				 * to be 256MB, and there are holes in the NUMA
				 * map higher up, those holes are ignored.
				 *
				 * Only holes that are below the memSize mark
				 * will get a shbank memory range.
				 **/
				if (numaMap->memEntries[i].baseAddr
					> memConfig->memSize)
				{
					break;
				};

				tmpBase = numaMap->memEntries[i].baseAddr
					+ numaMap->memEntries[i].size;

				tmpSize = numaMap->memEntries[i+1].baseAddr
					- tmpBase;

				if (tmpBase + tmpSize > memConfig->memSize) {
					tmpSize = memConfig->memSize - tmpBase;
				};

				if (tmpSize > 0)
				{
					__kprintf(NOTICE NUMATRIB
						"For memrange %d, on bank %d, "
						"base 0x%X, size 0x%X, "
						"next entry base 0x%X, shbank "
						"memory range with base 0x%X "
						"and size 0x%X is needed.\n",
						i,
						numaMap->memEntries[i].bankId,
						numaMap->memEntries[i].baseAddr,
						numaMap->memEntries[i].size,
						numaMap->memEntries[i+1]
							.baseAddr,
						tmpBase, tmpSize);

					ret = getStream(
						CHIPSET_MEMORY_NUMA_SHBANKID)
						->memoryBank.addMemoryRange(
							tmpBase, tmpSize);

					if (ret != ERROR_SUCCESS)
					{
						__kprintf(ERROR NUMATRIB
							"Shbank: Failed to "
							"spawn memrange for "
							"hole: base 0x%X size "
							"0x%X.\n",
							tmpBase, tmpSize);
					}
					else
					{
						__kprintf(NOTICE NUMATRIB
							"Shbank: New memrange "
							"base 0x%X, size "
							"0x%X.\n",
							tmpBase, tmpSize);
					};
				};
			};
		}
		else
		{

			ret = getStream(CHIPSET_MEMORY_NUMA_SHBANKID)
				->memoryBank.addMemoryRange(
					0x0, memConfig->memSize);

			if (ret != ERROR_SUCCESS)
			{
				__kprintf(ERROR NUMATRIB"Failed to add memory "
					"range for shbank memsize.\n");
			}
			else
			{
				__kprintf(NOTICE NUMATRIB"Shbank: no NUMA map. "
					"Spawn with total memsize 0x%X.\n",
					memConfig->memSize);
			};
		};
	}
	else {
		__kprintf(ERROR NUMATRIB"getMemoryConfig(): no config.\n");
	};
#endif

parseMemoryMap:
	memMap = (memInfoRiv->getMemoryMap)();
	if (memMap != __KNULL && memMap->nEntries > 0)
	{
		pos = numaStreams.prepareForLoop();
		ns = numaStreams.getLoopItem(&pos);
		for (; ns != __KNULL; ns = numaStreams.getLoopItem(&pos))
		{
			for (uarch_t i=0; i<memMap->nEntries; i++)
			{
				if (memMap->entries[i].memType !=
					CHIPSETMMAP_TYPE_USABLE)
				{
					ns->memoryBank.mapMemUsed(
						memMap->entries[i].baseAddr,
						PAGING_BYTES_TO_PAGES(
							memMap->entries[i]
								.size));
				};
			};
		};
	}
	else {
		__kprintf(WARNING NUMATRIB"getMemoryMap(): No mem map.\n");
	};

	// Next merge all banks with __kspace.
	pos = numaStreams.prepareForLoop();
	ns = numaStreams.getLoopItem(&pos);
	for (; ns != __KNULL; ns = numaStreams.getLoopItem(&pos)) {
		nSet = ns->memoryBank.merge(
			&getStream(CHIPSET_MEMORY_NUMA___KSPACE_BANKID)
				->memoryBank);
	};

	__kprintf(NOTICE NUMATRIB"%d frames were merged from __kspace into new "
		"PMM state.\n", nSet);

	// Then apply the Memory Tributary's Memory Regions to all banks.
	if (chipsetRegionMap != __KNULL) 
	{
		pos = numaStreams.prepareForLoop();
		ns = numaStreams.getLoopItem(&pos);
		for (; ns != __KNULL; ns = numaStreams.getLoopItem(&pos))
		{
			for (uarch_t i=0; i<chipsetRegionMap->nEntries; i++)
			{
				ns->memoryBank.mapMemUsed(
					chipsetRegionMap->entries[i].baseAddr,
					PAGING_BYTES_TO_PAGES(
						chipsetRegionMap
							->entries[i].size));
			};
		};
	};

	return ERROR_SUCCESS;
}
