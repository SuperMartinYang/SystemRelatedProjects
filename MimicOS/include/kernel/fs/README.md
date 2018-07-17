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
Fields in FAT_BOOTSECTOR16


Fields in FAT_BOOTSECTOR32


Fields in FAT_BOOTSECTOR


Fields in FAT_ENTRY

Fields in FAT_MOUNTPOINT

Fields in FAT_FILE



## VFS.H
###	Structures
Fields in VFS_HANDLE


Fields in VFS_FILESYSTEM_CALLTABLE


Fields in VFS_FILESYSTEM


Fields in VFS_MOUNTPOINT


Fields in VFS_DIRLIST_ENTRY
