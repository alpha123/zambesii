
#include <arch/x8632/mp.h>

/**	EXPLANATION:
 * Hardcoded tables for the default configurations on Intel MP.
 **/

struct x86_mpCfg1S
{
	struct x86_mpCfgS	cfg;
	struct x86_mpCfgCpuS	cpus[2];
	struct x86_mpCfgIoApicS	ioApic;
	// We won't care about the IRQ Source entries anytime soon.
};

extern struct x86_mpCfg1S	x86_mpCfg1, x86_mpCfg2, x86_mpCfg3, x86_mpCfg4,
				x86_mpCfg5, x86_mpCfg6, x86_mpCfg7;

struct x86_mpCfgS		*x86_mpCfgDefaults[7] =
{
	(struct x86_mpCfgS *)&x86_mpCfg1,
	(struct x86_mpCfgS *)&x86_mpCfg2,
	(struct x86_mpCfgS *)&x86_mpCfg3,
	(struct x86_mpCfgS *)&x86_mpCfg4,
	(struct x86_mpCfgS *)&x86_mpCfg5,
	(struct x86_mpCfgS *)&x86_mpCfg6,
	(struct x86_mpCfgS *)&x86_mpCfg7
};

static struct x86_mpCfg1S	x86_mpCfg1 =
{
	{
		{'P', 'C', 'M', 'P'},
		sizeof(x86MpConfig1),
		0x01,
		// Checksum.
		0,
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i'},
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i', ' ', ' ', ' ', ' '},
		0x0,
		0x0,
		// nEntries.
		0x0,
		0xFEC00000,
		0x0,
		0x0
	},
	{
		{
			x86_MPCFG_TYPE_CPU,
			0x0,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		},
		{
			x86_MPCFG_TYPE_CPU,
			0x1,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		}
	},
	{
		x86_MPCFG_TYPE_IOAPIC,
		0x2,
		// FIXME: I/O APIC Version. Check over values.	
		0x1,
		x86_MPCFG_IOAPIC_FLAGS_ENABLED,
		0xFEE00000
	}
	
};

static struct x86_mpCfg1S	x86_mpCfg2 =
{
	{
		{'P', 'C', 'M', 'P'},
		sizeof(x86MpConfig1),
		0x01,
		// Checksum.
		0,
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i'},
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i', ' ', ' ', ' ', ' '},
		0x0,
		0x0,
		// nEntries.
		0x0,
		0xFEC00000,
		0x0,
		0x0
	},
	{
		{
			x86_MPCFG_TYPE_CPU,
			0x0,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		},
		{
			x86_MPCFG_TYPE_CPU,
			0x1,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		}
	},
	{
		x86_MPCFG_TYPE_IOAPIC,
		0x2,
		// FIXME: I/O APIC Version. Check over values.	
		0x1,
		x86_MPCFG_IOAPIC_FLAGS_ENABLED,
		0xFEE00000
	}
	
};

static struct x86_mpCfg1S	x86_mpCfg3 =
{
	{
		{'P', 'C', 'M', 'P'},
		sizeof(x86MpConfig1),
		0x01,
		// Checksum.
		0,
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i'},
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i', ' ', ' ', ' ', ' '},
		0x0,
		0x0,
		// nEntries.
		0x0,
		0xFEC00000,
		0x0,
		0x0
	},
	{
		{
			x86_MPCFG_TYPE_CPU,
			0x0,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		},
		{
			x86_MPCFG_TYPE_CPU,
			0x1,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		}
	},
	{
		x86_MPCFG_TYPE_IOAPIC,
		0x2,
		// FIXME: I/O APIC Version. Check over values.	
		0x1,
		x86_MPCFG_IOAPIC_FLAGS_ENABLED,
		0xFEE00000
	}
	
};

static struct x86_mpCfg1S	x86_mpCfg4 =
{
	{
		{'P', 'C', 'M', 'P'},
		sizeof(x86MpConfig1),
		0x01,
		// Checksum.
		0,
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i'},
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i', ' ', ' ', ' ', ' '},
		0x0,
		0x0,
		// nEntries.
		0x0,
		0xFEC00000,
		0x0,
		0x0
	},
	{
		{
			x86_MPCFG_TYPE_CPU,
			0x0,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		},
		{
			x86_MPCFG_TYPE_CPU,
			0x1,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		}
	},
	{
		x86_MPCFG_TYPE_IOAPIC,
		0x2,
		// FIXME: I/O APIC Version. Check over values.	
		0x1,
		x86_MPCFG_IOAPIC_FLAGS_ENABLED,
		0xFEE00000
	}
	
};

static struct x86_mpCfg1S	x86_mpCfg5 =
{
	{
		{'P', 'C', 'M', 'P'},
		sizeof(x86MpConfig1),
		0x01,
		// Checksum.
		0,
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i'},
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i', ' ', ' ', ' ', ' '},
		0x0,
		0x0,
		// nEntries.
		0x0,
		0xFEC00000,
		0x0,
		0x0
	},
	{
		{
			x86_MPCFG_TYPE_CPU,
			0x0,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		},
		{
			x86_MPCFG_TYPE_CPU,
			0x1,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		}
	},
	{
		x86_MPCFG_TYPE_IOAPIC,
		0x2,
		// FIXME: I/O APIC Version. Check over values.	
		0x1,
		x86_MPCFG_IOAPIC_FLAGS_ENABLED,
		0xFEE00000
	}
	
};

static struct x86_mpCfg1S	x86_mpCfg6 =
{
	{
		{'P', 'C', 'M', 'P'},
		sizeof(x86MpConfig1),
		0x01,
		// Checksum.
		0,
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i'},
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i', ' ', ' ', ' ', ' '},
		0x0,
		0x0,
		// nEntries.
		0x0,
		0xFEC00000,
		0x0,
		0x0
	},
	{
		{
			x86_MPCFG_TYPE_CPU,
			0x0,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		},
		{
			x86_MPCFG_TYPE_CPU,
			0x1,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		}
	},
	{
		x86_MPCFG_TYPE_IOAPIC,
		0x2,
		// FIXME: I/O APIC Version. Check over values.	
		0x1,
		x86_MPCFG_IOAPIC_FLAGS_ENABLED,
		0xFEE00000
	}
	
};

static struct x86_mpCfg1S	x86_mpCfg7 =
{
	{
		{'P', 'C', 'M', 'P'},
		sizeof(x86MpConfig1),
		0x01,
		// Checksum.
		0,
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i'},
		{'Z', 'a', 'm', 'b', 'e', 'z', 'i', 'i', ' ', ' ', ' ', ' '},
		0x0,
		0x0,
		// nEntries.
		0x0,
		0xFEC00000,
		0x0,
		0x0
	},
	{
		{
			x86_MPCFG_TYPE_CPU,
			0x0,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_BSP | x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		},
		{
			x86_MPCFG_TYPE_CPU,
			0x1,
			// FIXME: LAPIC Version. Check over.
			0x01,
			x86_MPCFG_CPU_FLAGS_ENABLED,
			0x0
		}
	},
	{
		x86_MPCFG_TYPE_IOAPIC,
		0x2,
		// FIXME: I/O APIC Version. Check over values.	
		0x1,
		x86_MPCFG_IOAPIC_FLAGS_ENABLED,
		0xFEE00000
	}
	
};

