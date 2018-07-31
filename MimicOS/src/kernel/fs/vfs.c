#include <kernel/fs/vfs.h>
#include <kernel/fs/fat.h>
#include <kernel/fs/dfs.h>
#include <kernel/mm/mm.h>
#include <kernel/pm/process.h>
#include <lib/libc/string.h>

struct VFS_FILESYSTEM * vfs_fsHead = NULL;

struct VFS_MOUNTPOINT * vfs_mpHead = NULL;
struct VFS_MOUNTPOINT * vfs_mpTail = NULL;

/* register a new file system with the VFS */
int vfs_register(struct VFS_FILESYSTEM * fs){
	// add the new file system to a linked list of file system drivers present in the system
	fs->prev = vfs_fsHead;
	vfs_fsHead = fs;
	// we can now mount volumes of this file system type
	return SUCCESS;
}

/* unregister a file system driver from the vfs */
int vfs_unregister(int fstype){
	return FAIL;	// TO-DO
}

/* find a file system with a specific type */
struct VFS_FILESYSTEM * vfs_find(int fstype){
	struct VFS_FILESYSTEM * fs;
	// search through the linked list of the file system drivers
	for (fs = vfs_fsHead; fs != NULL; fs = fs->prev){
		// if found
		if (fs->fstype == fstype)
			break;
	}
	return fs;
}

int vfs_mount(char * device, char * mountpoint, int fstype){
	struct VFS_MOUNTPOINT * mount;
	// create our mountpoint structure
	mount = (struct VFS_MOUNTPOINT *)mm_kmalloc(sizeof(struct VFS_MOUNTPOINT));
	if (mount == NULL)
		return FAIL;
	// find the correct file system driver for the mount command
	mount->fs = vfs_find(fstype);
	if (mount->fs == NULL){
		mm_kfree(mount);
		return FAIL;
	}
	mount->mountpoint = (char *)mm_kmalloc(strlen(mountpoint) + 1);
	strcpy(mount->mountpoint, mountpoint);
	mount->device = (char *)mm_kmalloc(strlen(device) + 1);
	strcpy(mount->device, device);

	// add the fs and the mountpoint to the linked list
	if (vfs_mpTail == NULL)
		vfs_mpTail = mount;
	else
		vfs_mpHead->next = mount;
	vfs_mpHead = mount;
	/*
	mount->next = NULL;
	if (vfs_mpTail != NULL)
		mount->next = vfs_mpTail;
	vfs_mpTail = mount;
	*/

	// call the file system driver to mount
	if (mount->fs->calltable.mount == NULL){
		mm_kfree(mount);
		return FAIL;
	}
	mount->data_ptr = mount->calltable.mount(device, mountpoint, fstype);
	if (mount->data_ptr == NULL){
		mm_kfree(mount);
		return FAIL;
	}
	return SUCCESS;
}

int vfs_unmount(char * mountpoint){
	
}