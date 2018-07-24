#ifndef _KERNEL_IO_FS_FAT_H_
#define _KERNEL_IO_FS_FAT_H_

#include <sys/types.h>

#define FAT_TYPE	1

#define FAT_READ	1
#define FAT_WRITE	2

#define FAT_PROCESS_BREAK		-1
#define FAT_PROCESS_CONTINUE	-2
#define FAT_PROCESS_SUCCESS		0

#define FAT_CLUSTER12(c)	(c & 0x00000FFF)	// get the fat_data ptr
#define FAT_CLUSTER16(c)	(c & 0x0000FFFF)
#define FAT_CLUSTER31(c)	(c & 0x0FFFFFFF)

#define FAT_FREECLUSTER		0x00000000
#define FAT_ENDOFCLUSTER	0xFFFFFFFF
#define FAT_BADCLUSTER		0x0FFFFFF7
#define FAT_RESERVERCLUSTER	0x0FFFFFF8

#define FAT_ENTRY_DELETED	0xE5

#define FAT_PADBYTE			0X20

#define FAT_12 	12
#define FAT_16 	16
#define FAT_32  32

#define FAT_MAGIC	0xAA55

// FAT 16
struct FAT_BOOTSETOR16
{
	BYTE BS_DrvNUM;
	BYTE BS_Reserved1;
	BYTE BS_BootSig;
	DWORD BS_VolID;
	BYTE BS_VolLab[11];
	BYTE BS_FilSysType[8];
} PACKED;

struct FAT_BOOTSECTOR32
{
	// File System Specification
	DWORD BPB_FATSz32;
	WORD BPB_ExtFlags;
	WORD BPB_FSVer;
	DWORD BPB_RootClus;
	WORD BPB_FSInfo;
	// Boot Sector and Bootstrap Modification
	WORD BPB_BKBootSec;
	BYTE BPB_Reserved[12];
	BYTE BPB_DrvNum;
	BYTE BS_Reserved1;
	BYTE BS_BootSig;
	DWORD BS_VolID;
	BYTE BS_VolLab[11];
	BYTE BS_FileSysType[8];
} PACKED;

struct FAT_BOOTSECTOR
{
	BYTE jmp[3];
	BYTE oem_id[8];
	// BIOS Parameter Block
	WORD bytes_per_sector;
	BYTE sectors_per_cluster;
	WORD reserved_sectors;
	BYTE num_fats;
	WORD num_root_dir_ents;
	WORD total_sectors;
	BYTE media_id_byte;
	WORD sectors_per_fat;
	WORD sectors_per_track;
	WORD heads;
	DWORD hidden_sectors;
	DWORD total_sectors_large;
	/**
	BYTE physical_disk_num;
	BYTE current_head;
	BYTE signature;
	BYTE vol_serial_num[4];
	BYTE vol_label[11];
	BYTE sys_id[8];
	*/
	union
	{
		struct FAT_BOOTSECTOR16 bs16;
		struct FAT_BOOTSECTOR32 bs32;
		BYTE boot_code[474];
	};
	WORD magicl
} PACKED;

struct FAT_DOSTIME
{
	unsigned int twosecs:5;
	unsigned int minutes:6;
	unsigned int hours:5;
} PACKED;

struct FAT_DOSDATE
{
	unsigned int date:5;
	unsigned int month:4;
	unsigned int year:7;
} PACKED;

struct FAT_ATTRIBUTE
{
	int readonly:1;
	int hidden:1;
	int system:1;
	int volumelabel:1;
	int directory:1;		// indicate this file is a directory
	int archive:1;
	int reserved:2;
} PACKED;

#define FAT_NAMESIZE		8
#define FAT_EXTENSIONSIZE	3

struct FAT_ENTRY   		// store file specify info, like data
{
	BYTE name[FAT_NAMESIZE];
	BYTE extension[FAT_EXTENSIONSIZE];
	struct FAT_ATTRIBUTE attribute;
	BYTE reserved[10];
	struct FAT_DOSTIME time;
	struct FAT_DOSDATE date;
	WORD start_cluster;
	DWORD file_size;	
} PACKED;

struct FAT_MOUNTPOINT
{
	struct VFS_HANDLE * device;
	struct FAT_BOOTSECTOR bootsector;
	struct FAT_ENTRY * rootdir;
	BYTE type;
	BYTE * fat_data;		// ptr to fat data (clusters)
	int fat_size;			// the number of cluster in each fat
	int cluster_size;		// the size of each cluster
	int total_clusters;		// total cluster available
} PACKED;

struct FAT_FILE			// file info
{
	struct FAT_MOUNTPOINT * mount;
	struct FAT_ENTRY entry;	 // the real content of file 
	int dir_cluster;
	int dir_index;
	int current_pos;		// used as a cursor when read or write to the file
} PACKED;

int fat_init(void);

#endif