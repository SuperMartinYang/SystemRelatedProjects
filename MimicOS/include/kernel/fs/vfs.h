#ifndef _KERNEL_FS_VFS_H
#define _KERNEL_FS_VFS_H

#include <sys/types.h>

#define VFS_MAXFILENAME 	256

#define VFS_FILE 			0
#define VFS_DIRECTORY		1
#define VFS_DEVICE			2

#define VFS_SEEK_START		0
#define VFS_SEEK_CURRENT	1
#define VFS_SEEK_END		2

#define VFS_MODE_READ		1
#define VFS_MODE_WRITE		2
#define VFS_MODE_READWRITE	(VFS_MODE_READ | VFS_MODE_WRITE)
#define VFS_MODE_CREATE		4
#define VFS_MODE_TRUNCATE	8
#define VFS_MODE_APPEND		9

struct VFS_HANDLE		// used to operate on data_ptr
{
	struct VFS_MOUNTPOINT * mount;
	void * data_ptr;	// always point to struct FAT_FILE
	int mode;
};

typedef int (*rw)(struct VFS_HANDLE *, BYTE *, DWORD);

struct VFS_FILESYSTEM_CALLTABLE
{
	struct VFS_HANDLE * (*open)(struct VFS_HANDLE *, char *);
	int (*close)(struct VFS_HANDLE *);
	int (*clone)(struct VFS_HANDLE *, struct VFS_HANDLE *);
	rw read;
	rw write;
	int (*seek)(struct VFS_HANDLE *, DWORD, BYTE);
	int (*control)(struct VFS_HANDLE *, DWORD, DWORD);
	int (*create)(struct VFS_MOUNTPOINT *, char *);
	int (*delete)(struct VFS_MOUNTPOINT *, char *);
	int (*rename)(struct VFS_MOUNTPOINT *, char *, char *);
	int (*copy)(struct VFS_MOUNTPOINT *, char *, char *);
	struct VFS_DIRLIST_ENTRY * (*list)(struct VFS_MOUNTPOINT *, char *);
	void * (*mount)(char *, char *, int);
	int (*unmount)(struct VFS_MOUNTPOINT *, char *);
};

struct VFS_FILESYSTEM  	// file system struction, linked list
{
	int fstype;
	struct VFS_FILESYSTEM_CALLTABLE calltable;
	struct VFS_FILESYSTEM * prev;
};

struct VFS_MOUNTPOINT		// virtual fs content, linked list
{
	struct VFS_FILESYSTEM * fs;
	void * data_ptr;		// FAT_MOUNTPOINT, stores fs content
	char * mountpoint;
	char * device;
	struct VFS_MOUNTPOINT * next;
};

#define VFS_NAMESIZE	32

struct VFS_DIRLIST_ENTRY	// used to store file item tiny info
{
	char name[VFS_NAMESIZE];
	int attributes;
	int size;
};

int vfs_init(void);

int vfs_register(struct VFS_FILESYSTEM *);

int vfs_unregister(int);

// volume operations
int vfs_mount(char *, char *, int);

int vfs_unmount(char *);

// file operations
struct VFS_HANDLE * vfs_open(char *, int);

int vfs_close(struct VFS_HANDLE *);

struct VFS_HANDLE * vfs_clone(struct VFS_HANDLE *);

int vfs_read(struct VFS_HANDLE *, BYTE *, DWORD);

int vfs_write(struct VFS_HANDLE *, BYTE *, DWORD);

int vfs_seek(struct VFS_HANDLE *, DWORD, BYTE);

int vfs_control(struct VFS_HANDLE *, DWORD, DWORD);

int vfs_create(char *);

int vfs_delete(char *);

int vfs_rename(char *);

int vfs_copy(char *, char *);

struct VFS_DIRLIST_ENTRY * vfs_list(char *);

#endif