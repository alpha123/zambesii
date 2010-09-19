#ifndef _PROCESS_TRIBUTARY_H
	#define _PROCESS_TRIBUTARY_H

	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/tributary.h>
	#include <kernel/common/sharedResourceGroup.h>
	#include <kernel/common/multipleReaderLock.h>
	#include <kernel/common/processId.h>

class processTribC
:
public tributaryC
{
public:
	// Init __kprocess, __korientation & boot CPU Stream.
	error_t initialize(void);

public:
	processS *__kgetProcess(void) { return &__kprocess; };
	processS *getProcess(processId_t id);
	processS *getProcess(const utf16Char *absName);

	processS *spawnProcess(const utf16Char *absName);
	error_t destroyProcess(void);

private:
	processS	__kprocess;
	sharedResourceGroupC<multipleReaderLockC, processS *>	processes;
};

#endif

