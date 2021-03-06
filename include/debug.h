#ifndef _DEBUG_HELPER_H
	#define _DEBUG_HELPER_H

#define DEBUG_ON(__con)				\
	if (__con) \
	{ \
		asm volatile("" ::: "memory"); \
		asm volatile ( \
			"pushl	%%ecx \n\t " \
			"movl	$0, %%ecx \n\t" \
			"1: \n\t" \
			"cmp	$1, %%ecx \n\t" \
			"jne	1b \n\t" \
			"popl	%%ecx \n\t" \
			::: "memory"); \
	}

extern int oo, pp, qq, rr;

int testFunction(...);

// GCC likes to warn me about this, and it gets really irritating.
// #define PRINTFON(__cond, __str, ...)		{if (__cond) { printf(__str, __VA_ARGS__); };}

#endif

