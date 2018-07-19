#Descriptions for the head files in fs

## DFS.H
### Structures
Fields in Device file system (DFS)

| Byte Offset (in hex)| Field Length  | Sample Value | Meaning           |
|---------------------|---------------|--------------|-------------------|
| 04				  | 4 bytes		  | 			 | prev device       |
| 08				  | 4 bytes		  | 			 | calltable func ptr|
| 0C				  | 4 bytes		  | Dir1	     | filename			 |
| 0F				  | 4 bytes       | DIRECTORY    | type				 |

## FAT.H
### Structures

Partition <br />Boot <br /> Sector | FAT1 | FAT2 <br /> (duplicate) | Root <br /> folder | other folders and all files |
---|---|---|---|---	

Fields in FAT_BOOTSECTOR16


Fields in FAT_BOOTSECTOR32


Fields in FAT_BOOTSECTOR

Byte Offset (in hex) | Field Length | Sample Value | Meaning
---|---:|---:|---
00 | 3 bytes | EB 3C 90 | Jump instruction
03 | 8 bytes | MSDOS5.0 | OEM Name in text
0B | 25 bytes | | BIOS Parameter Block
24 | 26 bytes | | Extended BIOS Parameter Block
3E | 448 bytes | | Bootstrap code
1FE | 2 bytes | 0x55AA | End of sector marker

Specify BIOS Parameter Block

Byte Offset (in hex) | Field Length | Sample Value | Meaning
---|---:|---:|---
0B | WORD | 0x0002 | **Bytes per Sector**. The size of a hardware sector (default 512B)
0D | BYTE | 0x08 | **Sectors per Cluster**. The number of sectors in a cluster. depends on the volume size and the fs
0E | WORD | 0x0100 | **Reserved Sector**. From Partition Boot Sector(PBS) to the start of the first file allocation table, including the PBS
10 | BYTE | 0x02 | **Number of file allocation tables(FATs)**. The number of copies of FAT for this volume. (default 2)
11 | WORD | 0x0002 | **Root Entries**. The total number of the file name entries can be stored in the root folder of the volume. long name takes more entries.
13 | WORD | 0x0000 | **Small Sectors**, The number of sectors on the volume if the number fits in 16 bits. ELSE set to 0, and larger sectors field is used instead
15 | BYTE | 0xF8 | **Media Type**. Info about the media used. 0xF8 means hard disk.
16 | WORD | 0xC900 | **Sectors per FAT**. Number of sectors occupied by each FAT on this volume. with **Number of FAT** and **Reserved Sectors**, we can compute the start of root folder
18 | WORD | 0x3F00 | **Sectors per Track**. The apparent disk geometry in use when the disk was low-level formatted.
1A | WORD | 0x1000 | **Number of Heads**. The apparent disk geometry in use when the disk was low-level formatted.
1C | DWORD | 3F 00<br /> 00 00 | **Hidden Sectors**.  Same as the Relative Sector field in the Partition Table.
20 | DWORD | 51 42<br /> 06 00 | **Large Sectors**. if the small sectors isn't used.
24 | BYTE | 0x80 | **Physical Disk Number**. This is related to the BIOS physical disk number. Floppy drives are numbered starting with 0x00 for the A disk. Physical hard disks are numbered starting with 0x80. The value is typically 0x80 for hard disks, regardless of how many physical disk drives exist, because the value is only relevant if the device is the startup disk.
25 | BYTE | 0x00 | **Current Head**. Not used by the FAT file system.
26 | BYTE | 0x29 | **Signature**. Must be either 0x28 or 0x29 in order to be recognized by Windows NT.
27 | 4 bytes | CE 13<br /> 46 30 | **Volume Serial Number**. A unique number that is created when you format the volume.
2B | 11 bytes | NO NAME | **Volume Label**. This field was used to store the volume label, but the volume label is now stored as special file in the root directory.
36 | 8 bytes | FAT16 | **System ID**. Either FAT12 or FAT16, depending on the format of the disk.

Fields in FAT_ENTRY (FAT Folder Structure)
 * Name (eight-plus-three characters)
 * Attribute byte (8 bits worth of information, described later in this section)
 * Reserved (10 bytes)
 * Create time (24 bits)
 * Create date (16 bits)
 * Last access date (16 bits)
 * Last modified time (16 bits)
 * Last modified date (16 bits)
 * Starting cluster number in the file allocation table (16 bits)
 * File size (32 bits)

Fields in FAT_MOUNTPOINT

Fields in FAT_FILE



## VFS.H
###	Structures
Fields in VFS_HANDLE

Byte Offset (in hex) | Field Length | Sample Value | Meaning
---|---:|---:|---


Fields in VFS_FILESYSTEM_CALLTABLE
 * ptrs to operations on VFS

Fields in VFS_FILESYSTEM

Byte Offset (in hex) | Field Length | Sample Value | Meaning
---|---:|---:|---
04 | 4 bytes |  | file system type
3C | 56 bytes |  | pointers to operations on VFS
40 | 4 bytes |  | pointer to the next vfs in the list

Fields in VFS_MOUNTPOINT

Byte Offset (in hex) | Field Length | Sample Value | Meaning
---|---:|---:|---


Fields in VFS_DIRLIST_ENTRY

Byte Offset (in hex) | Field Length | Sample Value | Meaning
---|---:|---:|---
0 | 32 bytes | hello.txt | filename
20 | 4 bytes |  | attributes
24 | 4 bytes |  | file size 