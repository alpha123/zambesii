
#include <__ksymbols.h>
#include <arch/paging.h>
#include <arch/paddr_t.h>
#include <chipset/pkg/chipsetPackage.h>
#include <__kstdlib/__ktypes.h>
#include <__kstdlib/compiler/cxxrtl.h>
#include <__kstdlib/__kflagManipulation.h>
#include <__kstdlib/__kclib/string.h>
#include <__kstdlib/__kclib/assert.h>
#include <__kstdlib/__kcxxlib/new>
#include <__kclasses/debugPipe.h>
#include <__kclasses/memReservoir.h>
#include <__kclasses/prioQueue.h>
#include <__kthreads/__korientation.h>
#include <kernel/common/__koptimizationHacks.h>
#include <kernel/common/timerTrib/timerTrib.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/numaTrib/numaTrib.h>
#include <kernel/common/memoryTrib/memoryTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>
#include <kernel/common/processTrib/processTrib.h>
#include <kernel/common/execTrib/execTrib.h>
#include <kernel/common/vfsTrib/vfsTrib.h>


int oo=0, pp=0, qq=0, rr=0;

int ghfoo(void)
{
	__kprintf(NOTICE"This is a thread.\n");
	return 0;
}

extern "C" void __korientationMain(ubit32, multibootDataS *)
{
	error_t		ret;
	uarch_t		devMask;

	__koptimizationHacks();

	// Prepare the kernel by zeroing .BSS and calling constructors.
	memset(&__kbssStart, 0, &__kbssEnd - &__kbssStart);

	// Initialize the chipset's module package.
	DO_OR_DIE(chipsetPkg, initialize(), ret);
	// processTrib initializes __kprocess & __korientation.
	DO_OR_DIE(processTrib, initialize(), ret);
	DO_OR_DIE(cpuTrib, initialize(), ret);

	cxxrtl::callGlobalConstructors();

	DO_OR_DIE(interruptTrib, initialize(), ret);
	DO_OR_DIE(timerTrib, initialize(), ret);

	// Initialize the kernel swamp.
	DO_OR_DIE(
		memoryTrib,
		__kstreamInit(
			reinterpret_cast<void *>( 0xC0000000 + 0x400000 ),
			0x3FB00000, __KNULL),
		ret);

	DO_OR_DIE(memoryTrib, __kspaceInit(), ret);
	DO_OR_DIE(__kdebug, initialize(), ret);

	devMask = __kdebug.tieTo(DEBUGPIPE_DEVICE_BUFFER | DEBUGPIPE_DEVICE1);
	if (!__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE_BUFFER)
		|| !__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1))
	{
		if (__KFLAG_TEST(devMask, DEBUGPIPE_DEVICE1)) {
			__kprintf(WARNING ORIENT"No debug buffer allocated.\n");
		};
	};

	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Kernel debug output tied to devices BUFFER and "
		"DEVICE1.\n");

	DO_OR_DIE(memReservoir, initialize(), ret);
	DO_OR_DIE(cachePool, initialize(), ret);
	DO_OR_DIE(memoryTrib, pmemInit(), ret);

	DO_OR_DIE(processTrib, initialize2(), ret);
	DO_OR_DIE(cpuTrib, initialize2(), ret);
	DO_OR_DIE(execTrib, initialize(), ret);
	DO_OR_DIE(vfsTrib, initialize(), ret);

	ubit8		t;
	void		*r;
	status_t	st;

	vfsTrib.createTree((utf8Char *)":ekfs");
	vfsTrib.setDefaultTree((utf8Char *)":ekfs");
	vfsTrib.createTree((utf8Char *)":sapphire");
	vfsTrib.createTree((utf8Char *)":ftp");
	__kprintf(NOTICE"Created trees :ekfs, :sapphire, :ftp, :ekfs is the "
		"default tree.\n");

	st = vfsTrib.getPath((utf8Char *)":ekfs", &t, &r);
	__kprintf(NOTICE"GetPath on :ekfs: %d.\n", st);

	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Result of createFolder: %d.\n",
		vfsTrib.createFolder(static_cast<vfsDirC *>( r ), (utf8Char *)"zambezii"));

	__kprintf(NOTICE ORIENT"Result of createFile: %d.\n",
		vfsTrib.createFile(static_cast<vfsDirC *>( r ), (utf8Char *)"file1"));

	__kprintf(NOTICE ORIENT"result of getPath on :ekfs/zambezii: %d.\n",
		vfsTrib.getPath((utf8Char *)":ekfs/zambezii", &t, &r));

	__kprintf(NOTICE ORIENT"result of getPath on :ekfs/file1: %d.\n",
		vfsTrib.getPath((utf8Char *)":ekfs/file1", &t, &r));

	vfsTrib.getDefaultTree()->desc->dumpSubDirs();
	vfsTrib.getDefaultTree()->desc->dumpFiles();

	ret = processTrib.__kgetProcess()->spawnThread(
		reinterpret_cast<void *>( &ghfoo ), SCHEDPOLICY_ROUND_ROBIN,
		13, SPAWNTHREAD_FLAGS_SCHEDPOLICY_SET
		| SPAWNTHREAD_FLAGS_SCHEDPRIO_SET);

	__kprintf(NOTICE"Result of spawnThread: %d.\n", ret);

	__kdebug.refresh();
	__kprintf(NOTICE ORIENT"Successful!\n");
}

