#ifndef _KERNEL_MULTIBOOT_H_
#define _KERNEL_MULTIBOOT_H_

#include <sys/types.h>
#include <kernel/pm/elf.h>

#define MULTIBOOT_FLAG_MEM			1
#define MULTIBOOT_FLAG_MMAP			32

#define MULTIBOOT_MMAP_AVAILABLE	1

struct MULTIBOOT_INFO
{
	DWORD flags;
	DWORD mem_lower;
	DWORD mem_upper;
	BYTE boot_device[4];
	DWORD cmdline;
	DWORD mods_count;
	DWORD mods_addr;
	struct ELF_SECTION_HDR_TABLE elf_sec;
	DWORD mmap_length;
	struct MULTIBOOT_MEMORY_MAP * map;
	DWORD drives_length;
	DWORD drives_addr;
	DWORD config_table;
	DWORD boot_loader_name;
	DWORD apm_table;
	DWORD vbe_control_info;
	DWORD vbe_mode_info;
	DWORD vbe_mode;
	DWORD vbe_interface_seg;
	DWORD vbe_interface_off;
	DWORD vbe_interface_len;
};

struct MULTIBOOT_MEMORY_MAP
{
	DWORD size;
	DWORD base_low;
	DWORD base_high;
	DWORD length_low;
	DWORD length_high;
	DWORD type;
};

#endif