#ifndef ___KORIENTATION_H
	#define ___KORIENTATION_H

	#include <multiboot.h>
	#include <__kstdlib/__ktypes.h>
	#include <kernel/common/task.h>

extern taskS		__korientationThread;
extern "C" void __korientationMain(ubit32 mbMagic, multibootDataS *mbData);

#endif
