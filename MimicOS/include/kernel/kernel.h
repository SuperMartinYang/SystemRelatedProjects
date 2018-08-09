#ifndef _KERNEL_KERNEL_H_
#define _KERNEL_KERNEL_H_

#include <sys/types.h>
#include <kernel/pm/process.h>

#define MIMICOS_VERSION_MAJOR       0
#define MIMICOS_VERSION_MINOR       1
#define MIMICOS_VERSION_PATCH       0

#define MIMICOS_VERSION_STRING      "MimicOS 0.1.0"

#define KERNEL_FILENAME             "kernel.elf"

#define KERNEL_PID                  0

#define KERNEL_QUICKMAP_VADDRESS    (void *)0xC0000000

#define KERNEL_CODE_VADDRESS        (void *)0xC0001000

#define KERNEL_HEAP_VADDRESS        (void *)0xD0000000

// for graphic present
#define KERNEL_VGA_VADDRESS         (void *)0x000B8000

#define KERNEL_VGA_PADDRESS         (void *)0x000B8000

void kernel_printInfo(void);

void kernel_printf(char *, ...);

void kernel_panic(struct PROCESS_stack *, char *);

#endif