#An incomplete list of ideal goals include:

> 1. Reliable and robust (even with hardware failures or incomplete writes due to power loss)
> 2. Access (security) controls
> 3. Accounting and quotas
> 4. Indexing and search
> 5. Versioning and backup capabilities
> 6. Encryption
> 7. Automatic compression
> 8. High performance (e.g. Caching in-memory)
> 9. Efficient use of storage de-duplication

## What is a file system
![file system](https://camo.githubusercontent.com/6a8f4e9abf2da15ef961f958cfb01a0b219cb4db/687474703a2f2f74696e66322e7675622e61632e62652f253745647665726d6569722f6d616e75616c2f75696e74726f2f6469736b2e676966)​

## inode format
![inode](https://camo.githubusercontent.com/ce1dba530e9217c0b8fa9fca4285fae8fa412da9/68747470733a2f2f636c61737365732e736f652e756373632e6564752f636d70733131312f46616c6c30382f696e6f64655f776974685f7369676e6174757265732e6a7067)

## info store in file
> 1. Filename
> 2. File size
> 3. Time created, last modified, last accessed
> 4. Permissions "type of file" + "user" + "group" + "other" <- "rwx", may has mask "s" - set-uid 
> 5. Filepath
> 6. Checksum
> 7. File data (inode)

**file and directory are inode, directory maps filename to inode**
**"ln" (symbolic link) is just set a name->inode pair in directory file, hard like will copy the contain**
**"rm" just remove the pair**
**script start with #!/usr/bin/env python for portability**
**hidden file start with "." will not be listed by "ls **

## copy bytes from A to B
```sh
dd if=/dev/urandom of=/dev/null bs=1k count=1024
dd if=/dev/zero of=/dev/null bs=1M count=1024
```

#Virtual file system
several virtual filesystems that are mounted (available) as part of the file-system. Files inside these virtual filesystems do not exist on the disk; they are generated dynamically by the kernel when a process requests a directory listing. Linux provides 3 main virtual filesystems

> /dev  - A list of physical and virtual devices (for example network card, cdrom, random number generator) e.g. /dev/urandom or /dev/random (high entropy)
> /proc - A list of resources used by each process and (by tradition) set of system information
> /sys - An organized list of internal kernel entities

```sh
# mount the file as a filesystem and explore its contents
$ wget ***.iso
$ mkdir arch
$ sudo mount -o loop ***.iso ./arch
$ cd arch
```

# Scalability and Reliability 
RAID (redundant array of inexpensive disks)
	RAID-1: a file has 2 copies among 2 disks
	RAID-0: a file is split up among 2 disks
	RAID-10: a file is split up among 2 disks AND copies among another 2 disks
	RAID-3: use a parity disk
![RAID-3](https://camo.githubusercontent.com/501cb4f1927ab1488191e981c5da663324e30280/687474703a2f2f6465766e756c6c2e747970657061642e636f6d2f2e612f366130306535353163333965316338383334303133336564313865643636393730622d7069)
	RAID-5: check-block
![RAID-5](https://camo.githubusercontent.com/4dfc52945c01b224ac5fbe8ea4d79e00f636d4e7/687474703a2f2f7777772e736561676174652e636f6d2f66696c65732f7777772d636f6e74656e742f6d616e75616c732f627573696e6573732d73746f726167652d6e61732d6f732d6d616e75616c2f5f7368617265642f696d616765732f313138615f696c6c5f726169645f352e706e67)

# Build up a virtual disk
How to use inode to access data:
![inode organization](https://camo.githubusercontent.com/4d1c4743712adf919887ff3813122e57781dcdbb/687474703a2f2f7577373134646f632e73636f2e636f6d2f656e2f46535f61646d696e2f67726170686963732f7335636861696e2e676966)
```c
// Disk size:
#define MAX_INODE (1024)
#define MAX_BLOCK (1024*1024)

// Each block is 4096 bytes:
typedef char[4096] block_t;

// A disk is an array of inodes and an array of disk blocks:
struct inode[MAX_INODE] inodes;
block[MAX_BLOCK] blocks;

struct inode {
 int[10] directblocks; // indices for the block array i.e. where to the find the file's content
 int indirectblock;
 long size;
 // ... standard inode meta-data e.g.
 int mode, userid, groupid;
 time_t ctime, atime, mtime;
}

char readbyte(inode*inode,long position) {
  if(position <0 || position >= inode->size) return -1; // invalid offset

  int block_count = position / 4096,offset = position % 4096;
  
  // block count better be 0..9 !
  int physical_idx = lookup_physical_block_index(inode, block_count );

  // sanity check that the disk block index is reasonable...
  assert(physical_idx >=0 && physical_idx < MAX_BLOCK);


  // read the disk block from our virtual disk 'blocks' and return the specific byte
  return blocks[physical_idx][offset];
}
/* This is only using double-indirected */ 
int lookup_physical_block_index(inode*inode, int block_count) {
  assert(sizeof(int)==4); // Warning this code assumes an index is 4 bytes!
  assert(block_count>=0 && block_count < 1024 + 10); // 0 <= block_count< 1034

  if( block_count < 10)
     return inode->directblocks[ block_count ];
  
  // read the indirect block from disk:
  block_t* oneblock = & blocks[ inode->indirectblock ];

  // Treat the 4KB as an array of 1024 pointers to other disk blocks
  int* table = (int*) oneblock;
  
 // Look up the correct entry in the table
 // Offset by 10 because the first 10 blocks of data are already 
 // accounted for
  return table[ block_count - 10 ];
}
```

