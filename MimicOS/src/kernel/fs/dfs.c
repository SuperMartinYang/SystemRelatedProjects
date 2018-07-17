/**
 * Usage:  
 *
 */

#include <kernel/fs/dfs.h>
#include <kernel/fs/vfs.h>
#include <kernel/mm/mm.h>
#include <kernel/io/io.h>
#include <lib/libc/string.h>

struct DFS_ENTRY * dfs_deviceHead = NULL;

struct DFS_ENTRY * dfs_add(char * name, struct IO_CALLTABLE * calltable, int type){
	struct DFS_ENTRY * device;
	device = (struct DFS_ENTRY *)mm_kmalloc(sizeof(struct DFS_ENTRY));
	device->prev = dfs_deviceHead;
	dfs_deviceHead = device;
	device->calltable = calltable;
	device->name = name;
	device->type = type;
	return device;
}

struct DFS_ENTRY * dfs_find(char * name){
	struct DFS_ENTRY * device;
	for (device = dfs_deviceHead; device != NULL; device = device->prev){
		if (strcmp(device-> name, name) == 0)
			break;
	}
	return device;
}

int dfs_remove(char * name){
	struct DFS_ENTRY * device, *d;
	// fine the device
	device = dfs_find(name);
	if (device == NULL)
		return FAIL;
	// TO-DO: test for any open file handles of the requested device

	// remove from linked list
	if (device == dfs_deviceHead)
		dfs_deviceHead = device->prev;
	else{
		for (d = dfs_deviceHead; d != NULL; d = d->prev){
			if (d -> prev == device){
				d ->prev = device->prev;
				break;
			}
		}
	}
	mm_kfree(device->calltable);
	mm_kfree(device->name);
	mm_kfree(device);

	return SUCCESS;
}

void * dfs_mount(char * device, char * mountpoint, int fstype){
	if (fstype == DFS_TYPE)
		return ((void *)!NULL);
	return NULL;
}

int dfs_unmount(struct VFS_MOUNTPOINT * mount, char * mountpoint){
	return FAIL;
}

struct VFS_HANDLE * dfs_open(struct VFS_HANDLE * handle, char * filename){
	struct DFS_ENTRY * device;
	device = dfs_find(filename);
	if (device == NULL) return NULL;

	handle->data_ptr = (void *)io_open(device);
	if (handle->data_ptr == NULL) return NULL;

	return handle;
}

int dfs_close(struct VFS_HANDLE * handle){
	return io_close((struct IO_HANDLE *)handle->data_ptr);
}

int dfs_clone(struct VFS_HANDLE * handle, struct VFS_HANDLE * clone){
	return io_clone((struct IO_HANDLE *) handle->data_ptr, (struct IO_HANDLE **)&clone->data_ptr);
}

int dfs_read(struct VFS_HANDLE * handle, BYTE * buffer, DWORD size){
	return io_read((struct IO_HANDLE *)handle->data_ptr, buffer, size);
}

int dfs_write(struct VFS_HANDLE * handle, BYTE * buffer, DWORD size){
	return io_write((struct IO_HANDLE *)handle->data_ptr, buffer, size);
}

int dfs_seek(struct VFS_HANDLE * handle, DWORD offset, BYTE origin){
	return io_seek((struct IO_HANDLE *)handle->data_ptr, offset, origin);
}

int dfs_control(struct VFS_HANDLE * handle, DWORD request, DWORD arg){
	return io_control((struct IO_HANDLE *)handle->data_ptr, request, arg);
}

int dfs_create(struct VFS_MOUNTPOINT * mount, char * filename){
	// We can't create a new file here, new devices are to be loaded
	// into the system with the IO Subsystem load() system call
	return FAIL;
}

int dfs_delete(struct VFS_MOUNTPOINT * mount, char * filename){
	return dfs_remove(filename);
}

int dfs_copy(struct VFS_MOUNTPOINT * mount, char * src, char * dst){
	struct DFS_ENTRY * device;
	char * name;
	struct IO_CALLTABLE * calltable;
	// find the source device
	device = dfs_find(src);
	if (device == NULL) return FAIL;

	calltable = (struct IO_CALLTABLE *)mm_kmalloc(sizeof(struct IO_CALLTABLE));
	memcpy(calltable, device->calltable, sizeof(struct IO_CALLTABLE));

	name = (char *)mm_kmalloc(strlen(dst));
	strcpy(name, dst);

	dfs_add(name, calltable, device->type);
}

int dfs_rename(struct VFS_MOUNTPOINT * mount, char * src, char * dst){
	// rename <=> move from src to dst
	if (dfs_copy(mount, src, dst) == FAIL) return FAIL;
	return dfs_delete(mount, src);
}

struct VFS_DIRLIST_ENTRY * dfs_list(struct VFS_MOUNTPOINT * mount, char * dir){
	struct DFS_ENTRY * device;
	struct VFS_DIRLIST_ENTRY * entry;
	int i = 0, count = 0;
	for (device = dfs_deviceHead; device != NULL; device = device->prev) count ++;

	if (count == 0) return NULL;

	entry = (struct VFS_DIRLIST_ENTRY *)mm_kmalloc((sizeof(struct VFS_DIRLIST_ENTRY) * 32));

	memset(entry, 0x00, (sizeof(struct VFS_DIRLIST_ENTRY) * 32));

	strncpy(entry[0].name, ".", 2);
	entry[0].attributes = VFS_DIRECTORY;
	entry[0].size = 0;

	strncpy(entry[1].name, "..", 2);
	entry[1].attributes = VFS_DIRECTORY;
	entry[1].size = 0;

	// add each device in
	for (device = dfs_deviceHead, i = 2; device != NULL || count > 0; device = device->prev, i ++, count --){
		// fill in the name
		strncpy(entry[i].name, device->name, VFS_NAMESIZE);
		// fill in the attributes
		entry[i].attributes = VFS_DEVICE;
		// fill in the size
		entry[i].size = 0;
	}
	return entry;
}

int dfs_init(void){
	struct VFS_FILESYSTEM * fs;
	fs = (struct VFS_FILESYSTEM *)mm_kmalloc(sizeof(struct VFS_FILESYSTEM));
	// set the file system type
	fs->fstype = DFS_TYPE;
	// setup the file system calltable
	fs->calltable.open 		= dfs_open;
	fs->calltable.close 	= dfs_close;
	fs->calltable.clone 	= dfs_clone;
	fs->calltable.read 		= dfs_read;
	fs->calltable.write  	= dfs_write;
	fs->calltable.seek 		= dfs_seek;
	fs->calltable.control 	= dfs_control;
	fs->calltable.create 	= dfs_create;
	fs->calltable.delete 	= dfs_delete;
	fs->calltable.rename 	= dfs_reanme;
	fs->calltable.copy 		= dfs_copy;
	fs->calltable.list 		= dfs_list;
	fs->calltable.mount 	= dfs_mount;
	fs->calltable.unmount 	= dfs_unmount;
	// register the file system with the vfs
	return vfs_register(fs);
}