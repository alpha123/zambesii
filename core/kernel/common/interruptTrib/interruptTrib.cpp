
#include <__kclasses/debugPipe.h>
#include <kernel/common/interruptTrib/interruptTrib.h>
#include <kernel/common/cpuTrib/cpuTrib.h>

interruptTribC::interruptTribC(void)
{
	memset(
		isrTable,
		0,
		sizeof(interruptTribC::vectorDescriptorS) * ARCH_IRQ_NVECTORS);
}

void interruptTribC::irqMain(taskContextS *regs)
{
	__kprintf(NOTICE"Interrupt Trib: CPU %d: Entry from vector %d.\n",
		regs->vectorNo, cpuTrib.getCurrentCpuStream()->cpuId);
	// Calls ISRs, then exit.
}

