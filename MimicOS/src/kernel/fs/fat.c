#include <kernel/fs/fat.h>
#include <kernel/fs/vfs.h>
#include <kernel/io/io.h>
#include <kernel/mm/mm.h>
#include <kernel/kernel.h>
#include <lib/libc/ctype.h>
#include <lib/libc/string.h>

#define DEBUG FALSE

/* set the fat_data to given value */
void fat_setFATCluster(struct FAT_MOUNTPOINT * mount, int cluster, int value, int commit){
	switch(mount->type){
		case FAT_12:
			mount->fat_data[(cluster * 3) / 2] = FAT_CLUSTER12(value);
			break;
		case FAT_16:
			mount->fat_data[cluster] = FAT_CLUSTER16(value);
			break;r
		case FAT_32:
			mount->fat_data[cluster] = FAT_CLUSTER32(value);
			break;
		default:
			return;
	}

	if (commit){
		vfs_seek(mount->device, sizeof(struct FAT_BOOTSECTOR) + 1, VFS_SEEK_START);
		vfs_write(mount->device, (void *)mount->fat_data, mount->fat_size);
	} 
}
 
/* Get next cluster */
int fat_getFATCluster(struct FAT_MOUNTPOINT * mount, int cluster){
	int next_cluster;
	switch(mount->type){
		case FAT_12:
			next_cluster = *(WORD *)((BYTE *)&mount->fat_data[(cluster * 3) / 2]);
			// if cluster is odd
			if (cluster & 0x1)
				next_cluster >>= 4;
			else
				next_cluster = FAT_CLUSTER12(next_cluster);
			if (next_cluster == FAT_CLUSTER12(FAT_ENDOFCLUSTER))
				return FAIL;
			break;
		case FAT_16:
			next_cluster = ((WORD *)mount->fat_data)[cluster];
			if (next_cluster == FAT_CLUSTER16(FAT_ENDOFCLUSTER))
				return FAIL;
			break;
		case FAT_32:
			next_cluster = ((DWORD *)mount->fat_data)[cluster];
			if (next_cluster == FAT_CLUSTER32(FAT_ENDOFCLUSTER))
				return FAIL;
			break;
		default:
			return FAIL;
	}
	return next_cluster;
}

int fat_getFreeCluster(struct FAT_MOUNTPOINT * mount){
	int cluster, index;
	for (index = 0; index < mount->total_clusters; index ++){
		cluster = fat_getFATCluster(mount, index);
		if (cluster == FAT_FREECLUSTER)	// free cluster is just start with 4 bytes 0x00
			return index;
	}
	return FAIL;
}

int fat_determineType(struct FAT_MOUNTPOINT * mount){
	int root_dir_sectors;
	int total_sectors;
	int fats;
	int data_sectors;

	root_dir_sectors = (((mount->bootsector.num_root_dir_ents * 32) + (mount->bootsector.bytes_per_sector - 1)) / mount->bootsector.bytes_per_sector);

	if (mount->bootsector.sectors_per_fat != 0)
		fats = mount->bootsector.sectors_per_fat;
	else
		fats = mount->bootsector.bs32.BPB_FATSz32;

	if (mount->bootsector.total_sectors != 0)
		total_sectors = mount->bootsector.total_sectors;
	else
		total_sectors = mount->bootsector.total_sectors_large;

	// | root_dir_sectors | num_fats * sectors_per_fat | reserved_sectors | data_sectors | ---> total_sectors
	data_sectors = total_sectors - (mount->bootsector.reserved_sectors + (mount->bootsector.num_fats * fats) + root_dir_sectors);
	
	// | data_sectors | -> | data_cluster (total_clusters) |
	mount->total_clusters = data_sectors / mount->bootsector.sectors_per_cluster;

	if (mount->total_clusters < 4085)
		mount->type = FAT_12;
	else if (mount->total_clusters < 65525)
		mount->type = FAT_16;
	else
		mount->type = FAT_32;

	return mount->type;

}


int fat_cluster2block(struct FAT_MOUNTPOINT * mount, int cluster){
	// total sectors 
	return cluster * mount->bootsector.sectors_per_cluster
		+ mount->bootsector.hidden_sectors
		+ mount->bootsector.num_fats * mount->bootsector.sectors_per_fat
		+ mount->bootsector.num_root_dir_ents / (mount->bootsector.bytes_per_sector / sizeof(struct FAT_ENTRY)) - 1;

}

/* read / write data from / to cluster */
int fat_rwCluster(struct FAT_MOUNTPOINT * mount, int cluster, BYTE * clusterBuffer, int mode){
	int i, block;
	rw vfs_rw;	// function ptr

	if (mode == FAT_READ)
		vfs_rw = vfs_read;
	else if (mode == FAT_WRITE)
		vfs_rw = vfs_write;
	else 
		return FAIL;
	// convert cluster to a logical block number
	block = fat_cluster2block(mount, cluster);
	// seek to the correct offset
	if (vfs_seek(mount->device, (block * mount->bootsector.bytes_per_sector) + 1, VFS_SEEK_START) == FAIL)
		return FAIL;
	// loop through the blocks
	for (i = 0; i < mount->bootsector.sectors_per_cluster; i++){
		clusterBuffer += mount->bootsector.bytes_per_sector * i;
		// perform the read or write
		if (vfs_rw(mount->device, (void *)clusterBuffer, mount->bootsector.bytes_per_sector) == FAIL)
			return FAIL;
	}
	return SUCCESS;
}

int fat_compareName(struct FAT_ENTRY * entry, char * name){
	int i, x;
	for (i = 0; i < strlen(name); i ++)
		name[i] = toupper(name[i]);

	for (i = 0; i < FAT_NAMESIZE; i ++){
		if (entry->name[i] == FAT_PADBYTE)
			break;
		if (name[i] != entry->name[i])
			return FAIL;
	}
	// file1.txt
	if (name[i] == '.' || entry->extension[0] != FAT_PADBYTE){
		i ++;
		for (x = 0; x < FAT_EXTENSIONSIZE; x ++){
			if (entry->extension[x] == FAT_PADBYTE)
				break;
			if (name[i + x] != entry->extension[x])
				return FAIL;
		}
	}
	return SUCCESS;
}

int fat_getIndex(struct FAT_ENTRY * dir, char * name){
	int i;
	for (i = 0; i < 32; i ++){
		if (dir[i].name[0] == 0x00)
			break;

		if (dir[i].name[0] == FAT_ENTRY_DELETED)
			continue;

		if (dir[i].start_cluster == FAT_FREECLUSTER && !dir[i].attribute.archive)
			continue;

		if (fat_compareName(&dir[i], name) == SUCCESS)
			return i;
	}
	return FAIL;
}
/* read the file info into the last parameter -> FAT_FILE * file and entry */
int fat_file2entry(struct FAT_MOUNTPOINT * mount, char * filename, struct FAT_ENTRY * entry, struct FAT_FILE * file){
	int i, dir_index = FAIL, length, dir_cluster = FAIL;
	char * curr_name;
	struct FAT_ENTRY * curr_dir, prevEntry;
	BYTE * clusterBuffer;
	/* Parse the filename */
	// advance past the first forward slash
	if (filename[0] == '/')
		filename ++;
	length = strlen(filename);
	for (i = 0; i < length; i ++)
		filename[i] = toupper(filename[i]);
	// allocate a buffer of memory the same size of the cluster
	clusterBuffer = (BYTE *)mm_kmalloc(mount->cluster_size);
	curr_name = filename;
	curr_dir = mount->rootdir;
	// loop through the filename to find the directory it belong, and the cluster pos it should be
	// searching by decomposing the filename into its component parts of
	// directorys and optional ending file
	for (i = 0; i <= length; i ++){
		if (filename[i] == '/' || filename[i] == '\0'){
			// set the forward '/' to '\0'
			filename[i] = '\0';
			// get the index in the entryy to the next part of the filename
			if ((dir_index = fat_getIndex(curr_dir, curr_name)) == FAIL)
				break;
			else {
				// save the cluster number, so we can set the file->dir_cluster if needed
				dir_cluster = prevEntry.start_cluster;
				// copy the current entry into the previous entry buffer
				memcpy(&prevEntry, (struct FAT_ENTRY *)&curr_dir[dir_idx], sizeof(struct FAT_ENTRY));
				// load the next cluster to check
				if (fat_rwCluster(mount, curr_dir[dir_index].start_cluster, clusterBuffer, FAT_READ) == FAIL){
					mm_kfree(clusterBuffer);
					return FAIL;
				}
				// associate the current directory with the newly loaded cluster
				curr_dir = (struct FAT_ENTRY *)clusterBuffer;
			}
			curr_name = &filename[i] + 1;
		}
	}
	// if we didn't find the file/directory
	if (dir_index == FAIL && strlen(curr_name) != 0){
		// free our cluster buffer
		mm_kfree(clusterBuffer);
		return FAIL;
	}
	// copy the file entry into the entry struct to return to the caller
	if (entry != NULL)
		memcpy(entry, &prevEntry, sizeof(struct FAT_ENTRY));
	// copy the files directory cluster number and index if needed
	if (file != NULL){
		file->dir_cluster = dir_cluster;
		file->dir_index = dir_index;
		memcpy(&file->entry, &prevEntry, sizeof(struct FAT_ENTRY));
	}
	// free cluster buffer
	mm_kfree(clusterBuffer);
	return SUCCESS;
}

// set the file with name
int fat_setFileName(struct FAT_FILE * file, char * name){
	int i, x;
	// these are defined in the official FAT spec
	// BYTE illegalBytes[16] = { 0x22, 0x2A, 0x2B, 0x2C, 0x2E, 0x2F, 0x3A, 0x3B, 
	//						  0x3C, 0x3D, 0x3E, 0x3F, 0x5B, 0x5C, 0x5D, 0x7C };
	// convert the name to UPPERCASE and test for illegal bytes
	for (i = 0; i < strlen(name); i++){
		/*if( name[i] < FAT_PADBYTE )
			return FAIL;
		for( x=0 ; x<16 ; x++ )
		{
			if( (BYTE)name[i] == illegalBytes[x] )
				return FAIL;
		}*/
		name[i] = toupper(name[i]);
	}

	// clear the entry's name and extension
	memset(&file->entry.name, FAT_PADBYTE, FAT_NAMESIZE);
	memset(&file->entry.extension, FAT_PADBYTE, FAT_EXTENSIONSIZE);

	// set the name
	for (i = 0; i < FAT_NAMESIZE; i ++){
		if (name[i] == 0x00 || name[i] == '.')
			break;
		file->entry.name[i] = name[i];
	}
	// if there's an extension
	if (name[i ++] == '.'){
		for (x = 0; x < FAT_EXTENSIONSIZE; i ++){
			if (name[i] == 0x00)
				break;
			file->entry.extension[x] = name[i];
		}
	}
	return SUCCESS;
}

/* update the file entry to the disk */
int fat_updateFileEntry(struct FAT_FILE * file){
	int ret = FAIL;
	struct FAT_ENTRY * dir;
	// allocate the directory structure
	dir = (struct FAT_ENTRY *)mm_kmalloc(file->mount->cluster_size);
	// read in the files directory data
	if (fat_rwCluster(file->mount, file->dir_cluster, (BYTE *)dir, FAT_READ) == SUCCESS){
		// if we don't have a directory index, we mush find one to use
		if (file->dir_index == FAIL){
			for (file->dir_index = 0; file->dir_index < (file->mount->cluster_size / sizeof(struct FAT_ENTRY)); file->dir_index ++){
				// skip it if it has negative size
				if (dir[file->dir_index].file_size == -1)
					continue;
				if (dir[file->dir_index].name[0] == 0x00){
					// make sure we don't go to far
					if (file->dir_index + 1 >= (file->mount->cluster_size / sizeof(struct FAT_ENTRY))){
						mm_kfree(dir);
						return FAIL;
					}
					dir[file->dir_index + 1].name[0] = 0x00;
					break;					
				}
			}
		}
		// write file data to dir cluster
		memcpy(&dir[file->dir_index], &file->entry, sizeof(struct FAT_ENTRY));
		ret = fat_rwCluster(file->mount, file->dir_cluster, (BYTE *)dir, FAT_WRITE);	
	}
	mm_kfree(dir);
	return res;
}

int fat_setFileSize(struct FAT_FILE * file, int size){
	// don't do anything if trying to set the same size
	if (file->entry.file_size == size)
		return SUCCESS;
	if (size < file->entry.file_size){
		/**
		// shrink
		// TO-DO: traverse the cluster chain(backwards) and mark them all free
		int cluster = entry[index].start_cluster;
		// set the first cluster to be free
		fat_setFATCluster( mount, cluster, FAT_FREECLUSTER, FALSE );
		while( cluster != FAT_FREECLUSTER )
		{
			// get the next cluster
			cluster = fat_getFATCluster( mount, cluster );
			if( cluster == FAT_FREECLUSTER || cluster == FAIL )
				break;
			// set the next cluster to be free
			fat_setFATCluster( mount, cluster, FAT_FREECLUSTER, FALSE );
		}
		// commit the FAT to disk
		fat_setFATCluster( mount, cluster, FAT_FREECLUSTER, TRUE );
		*/
	}
	// set the new size
	file->entry.file_size = size;
	return fat_updateFileEntry(file);  // write it back to cluster
}

void * fat_mount(char * device, char * mountpoint, int fstype){
	int root_dir_offset;
	struct FAT_MOUNTPOINT * mount = (struct FAT_MOUNTPOINT *)mm_kmalloc(sizeof(struct FAT_MOUNTPOINT));
	if (mount == NULL)
		return NULL;
	// open the device we wish to mount
	mount->device = vfs_open(device, VFS_MODE_READWRITE);
	if (mount->device == NULL){
		mm_kfree(mount);
		return NULL;
	}
	// read in the bootsector
	vfs_read(mount->device, (void *)&mount->bootsector, sizeof(struct FAT_BOOTSECTOR));
	// make sure we have a valid bootsector
	if (mount->bootsector.magic != FAT_MAGIC){
		vfs_close(mount->device);
		mm_kfree(mount);
		return NULL;
	}
	// determine if we have a FAT 12, 16, or 32 filesystem
	fat_determineType(mount);
	mount->cluster_size = mount->bootsector.bytes_per_sector * mount->bootsector.sectors_per_cluster;
	mount->fat_size = mount->bootsector.sectors_per_fat * mount->bootsector.bytes_per_sector;
	mount->fat_data = (BYTE *)mm_kmalloc(mount->fat_size);
	memset(mount->fat_data, 0x00, mount->fat_size);
	// read device info to fat
	vfs_read(mount->device, (void *)mount->fat_data, mount->fat_size);
	
	mount->rootdir = (struct FAT_ENTRY *)mm_kmalloc(mount->bootsector.num_root_dir_ents * sizeof(struct FAT_ENTRY));
	memset(mount->rootdir, 0x00, mount->bootsector.num_root_dir_ents * sizeof(struct FAT_ENTRY));
	// find and read in the root directory
	// | FAT_BOOTSECTOR | FAT0 | FAT1 | ROOT_DIRS (32)
	root_dir_offset = (mount->bootsector.num_fats * mount->fat_size) + sizeof(struct FAT_BOOTSECTOR) + 1
	vfs_seek(mount->device, root_dir_offset, VFS_SEEK_START);
	vfs_read(mount->device, (void *)mount->rootdir, mount->bootsector.num_root_dir_ents * sizeof(struct FAT_ENTRY));
	// successfully return the new FAT mount
	return mount;
}

int fat_unmount(struct VFS_MOUNTPOINT * mount, char * mountpoint){
	struct FAT_MOUNTPOINT * fat_mount;
	if ((fat_mount = (struct FAT_MOUNTPOINT *)mount->data_ptr) == NULL)
		return FAIL;
	vfs_close(fat_mount->device);
	mm_kfree(fat_mount->root_dir);	// bot-up free
	mm_kfree(fat_mount)
	return SUCCESS;
}

/* find the file and assign the FAT_FILE to handle, for vfs to operate on it */
struct VFS_HANDLE * fat_open(struct VFS_HANDLE * handle, char * filename){
	struct FAT_MOUNTPOINT * fat_mount;
	struct FAT_FILE * file;
	if ((fat_mount = (struct FAT_MOUNTPOINT *)handle->mount->data_ptr) == NULL)
		return FAIL;
	file = (struct FAT_FILE *)mm_kmalloc(sizeof(struct FAT_FILE));
	// try to find the file
	if (fat_file2entry(fat_mount, filename, NULL, file) == NULL){
		mm_kfree(file);
		return NULL;
	}
	// find out, set the mountpoint this file is on
	file->mount = fat_mount;
	// set the current file position to zero
	file->current_pos = 0;
	// associate the handle with the file entry
	handle->data_ptr = file;
	// if we opened the file in truncate mode we need to set the size to be zero
	if (handle->mode & VFS_MODE_TRUNCATE == VFS_MODE_TRUNCATE){
		if (fat_setFileSize(file, 0) == FAIL){
			mm_kfree(file);
			return NULL;
		}
	}
	return handle;
}

/* after vfs want to close the file. free the allocated file */
int fat_close(struct VFS_HANDLE * handle){
	// check we have a fat entry associate with this handle
	if (handle->data_ptr == NULL)
		return FAIL;
	mm_kfree(handle->data_ptr);
	return SUCCESS;
}

int fat_clone(struct VFS_HANDLE * handle, struct VFS_HANDLE * clone){
	// CURRENTLY NOT SUPPORTED
	return FAIL;
}

int fat_rw(struct VFS_HANDLE * handle, BYTE * buffer, DWORD size, int mode){
	int bytes_to_rw = 0, bytes_rw = 0, cluster_offset = 0;
	int cluster, i;
	struct FAT_FILE * file;
	BYTE * clusterBuffer;
	if ((file = (struct FAT_FILE *)handle->data_ptr) == NULL)
		return FAIL;
	// initially set the cluster number to the first cluster as specified in the file entry
	cluster = file->entry.start_cluster;
	// get the correct cluster to begin reading / writing from
	i = file->current_pos / file->mount->cluster_size;
	// we traverse the cluster chain i times
	while (i --){
		// get the next cluster in the file
		cluster = fat_getFATCluster(file->mount, cluster);
		// fail if we have gone beyond the files cluster chain
		if (cluster == FAT_FREECLUSTER || cluster == FAIL)
			return FAIL;
	}

	// reduce size if we are trying to read past the end of the file
	if (file->current_pos + size > file->entry.file_size){
		// but if we are writing we will need to expand the file size
		if (mode == FAT_WRITE){
			// add enough new clusters for writing
			int new_clusters = (size / file->mount->cluster_size) + 1;
			int prev_cluster = cluster, next_cluster;
			// alloc more clusters
			while (new_clusters --){
				// get a free cluster
				next_cluster = fat_getFreeCluster(file->mount);
				if (next_cluster == FAIL)
					return FAIL;
				// add it on the cluster chain of this file
				fat_setFATCluster(file->mount, prev_cluster, next_cluster, FALSE);
				// update our previous cluster number
				prev_cluster = new_cluster;
			}
			//terminate the cluster chain and commit the FAT to the disk
			fat_setFATCluster(file->mount, prev_cluster, FAT_ENDOFCLUSTER, TRUE);
		}else 
			size = file->entry.file_size - file->current_pos;
	}
	// handle reads / writes that begin from some point inside a cluster
	cluster_offset = file->current_pos % file->mount->cluster_size;
	//allocate a buffer to read / write data into
	clusterBuffer = (BYTE *)mm_kmalloc(file->mount->cluster_size);
	// read / write data
	while (TRUE){
		// set the amount of data we want to read / write in this loop iteration
		if (size > file->mount->cluster_size)
			bytes_to_rw = file->mount->cluster_size;
		else
			bytes_to_rw = size;
		// test if we are reading / writing across 2 cluster. if we are, we can only read / write up to the end of the first cluster
		// in this iteration of the loop, the next iteration will take care of the rest
		// this solution is ugly, more than likely a much cleaner way of checking for this
		// TO-DO --> make the code into cleaner way
		if ((cluster_offset + bytes_to_rw) > (((file->current_pos / file->mount->cluster_size) + 1) * file->mount->cluster_size)){
			bytes_to_rw = (cluster_offset + bytes_to_rw) - (((file->current_pos / file->mount->cluster_size) + 1) * file->mount->cluster_size);
			bytes_to_rw = size - bytes_to_rw;
		}
		// set up the clusterBuffer if we are going to perform a write operation
		if (mode == FAT_WRITE){
			// if we are writing from a point inside a cluster (as opposed to an entire clsuter) we will need to 
			// read in the original cluster to preserve the data before the cluster offset
			if (cluster_offset > 0 || bytes_to_rw < file->mount->cluster_size){
				if (fat_rwCluster(file->mount, cluster, clusterBuffer, FAT_READ) == FAIL)
					break;
			}else {
				memset(clusterBuffer, 0x00, file->mount->cluster_size);
			}
			// copy the data from buffer to clusterBuffer
			memcpy((clusterBuffer + cluster_offset), buffer, bytes_to_rw);
		}
		// read / write the next cluster of data. if we fail should we reset the file offset position if it's changed?
		if (fat_rwCluster(file->mount, cluster, clusterBuffer, mode) == FAIL)
			break;
		// if we performed a read operation, copy the cluster data into buffer
		if (mode == FAT_READ)
			memcpy(buffer, (clusterBuffer + cluster_offset), bytes_to_rw)
		// move the buffer pointer back
		buffer += bytes_to_rw;
		// increase the bytes read / written
		bytes_rw += bytes_to_rw;
		// reduce the size
		size -= bytes_to_rw;
		// test if we have read / written enough
		if (size <= 0)
			break;
		// get the next cluster to read / write
		cluster = fat_getFATCluster(file->mount, cluster);
		// we've reached the end of cluster chain
		if (the cluster == FAT_FREECLUSTER || cluster == FAIL)
			// for write oper, new cluster need to be allocated
			break;
		cluster_offset = 0;
	}
	// free the clusterBuffer
	mm_kfree(clusterBuffer);
	// update the file pos
	file->current_pos += bytes_rw;
	// return the counts read / written
	return bytes_rw;	
}

int fat_read(struct VFS_HANDLE * handle, BYTE * buffer, DWORD size){
	return fat_rw(handle, buffer, size, FAT_READ);
}

// write data and update the file entry
int fat_write(struct VFS_HANDLE * handle, BYTE * buffer, DWORD size){
	struct FAT_FILE * file;
	int bytes_written, orig_position;
	// retrieve the file structure
	if ((file = (struct FAT_FILE *)handle->data_ptr) == NULL)
		return FAIL;
	orig_position = file->current_pos;
	bytes_written = fat_rw(handle, buffer, size, FAT_WRITE);
	if (bytes_written >= 0){
		if ((orig_position + bytes_written) > file->entry.file_size)
			return fat_setFileSize(file, (orig_position + bytes_written));
		return SUCCESS;
	}
	return FAIL;
}

// set new file position
int fat_seek(struct VFS_HANDLE * handle, DWORD offset, BYTE origin){
	struct FAT_FILE * file;
	int saved_pos;
	// retrieve the file structure
	if ((file = (struct FAT_FILE *)handle->data_ptr) == NULL)
		return FAIL;
	// save the original position in case we need to roll back
	saved_pos = file->current_pos;
	// set the new position, 3 mode, set/add/minus
	if (origin == VFS_SEEK_START)
		file->current_pos = offset;
	else if (origin == VFS_SEEK_CURRENT)
		file->current_pos += offset;
	else if (origin == VFS_SEEK_END)
		file->current_pos = file->entry.file_size - offset;
	else
		return FAIL;
	// reset if we have gone over the file size
	if (file->current_pos > file->entry.file_size || file->current_pos < 0)
		file->current_pos = saved_pos;
	return file->current_pos;
}

int fat_control(struct VFS_HANDLE * handle, DWORD request, DWORD arg){
	// don't need to support and control calls
	return FAIL;
}

// create a file, like "/dir0/dir1/dir2/file1.txt"
int fat_create(struct VFS_MOUNTPOINT * mount, char * filename){
	int ret = FAIL;
	BOOL create_dir = FALSE;
	char * dirname;
	struct FAT_FILE * file;
	struct FAT_DOSTIME time;
	struct FAT_DOSDATE data;
	struct FAT_MOUNTPOINT * fat_mount;
	// retrieve the fat mount structure;
	if ((fat_mount = (struct FAT_MOUNTPOINT *)mount->data_ptr) == NULL)
		return FAIL;
	// create a new file, alloc first
	file = (struct FAT_FILE *)mm_kmalloc(struct FAT_FILE);
	dirname = (char *)mm_kmalloc(strlen(filename) + 1);	// 1 for '\0'
	strcpy(dirname, filename);

#ifdef DEBUG
	kernel_printf("[1] dirname(%d) %s filename(%d) %s\n",strlen(dirname),dirname,strlen(filename),filename);
#endif

	// check if we are trying to create a file which already exists
	if (fat_file2entry(fat_mount, filename, NULL, NULL) == FAIL){
		// not exist
		char * tmp;
		// decompose the filename into its directory portion and the filename portion
		// strcpy(filename, dirname);
		// filename = '/dir1/dir2/dir3/', or filename = '/', or filename = '/dir1/file.txt'
		if (filename[strlen(filename) - 1] == '/'){
			create_dir = TRUE;
			filename[strlen(filename) - 1] = 0x00;
			strcpy(dirname, filename);
		}
		// dirname == ''
		tmp = strrchr(dirname, '/');	
		if (tmp == NULL){
			strcpy(dirname, '/');
			// memcpy(&file->entry, &fat_mount->rootdir, sizeof(struct FAT_ENTRY));
			// is_root = TRUE;
		}else {
			// filename = '/dir1/dir2/dir3'
			//                      tmp -> 0x00  filename = dir3
			// dirname = '/dir1/dir2'
			tmp[1] = 0x00;
			filename = strrchr(filename, '/') + 1;
		}

#ifdef DEBUG
	kernel_printf("[1] dirname(%d) %s filename(%d) %s\n",strlen(dirname),dirname,strlen(filename),filename);
#endif
		// TO-DO
		// try to find the directory we wish to create the file in
		if (fat_file2entry(fat_mount, dirname, NULL, file) == SUCCESS){
			// set the FAT_MOUNT it exists on
			file->mount = fat_mount;
			// we don't have a directory index yet
			file->dir_index = FAIL;
			// set the new files direcotry cluster
			file->dir_cluster = file->entry.start_cluster;
			// set the current position to zero
			file->current_pos = 0;
			// clear the entry structure so we can fill in the new files details
			memset(&file->entry, 0x00, sizeof(struct FAT_ENTRY));
			// set the file name and extension
			if (fat_setFileName(file, filename) == SUCCESS){
				// set default time
				time.hours = 0;
				time.minutes = 0;
				time.twosecs = 0;
				memcpy(&file->entry.time, time, sizeof(struct FAT_DOSTIME));
				date.data = 0;
				date.month = 0;
				date.year = 0;
				memcpy(&file->entry.date, date, sizeof(struct FAT_DOSDATE));
				// set its initial size to zero
				file->entry.file_size = 0;
				if (create_dir){
					// set the attribute as a directory
					file->entry.attribute.directory = TRUE;
					// alloc an initial cluster for the directory and its entries
					file->entry.start_cluster = fat_getFreeCluster(file->mount);
					if (file->entry.start_cluster != (WORD) FAIL){
						// for one entry
						struct FAT_ENTRY * entry = (struct FAT_ENTRY *)mm_kmalloc(file->mount->cluster_size);
						memset(entry, 0x00, file->mount->cluster_size);
						// write entry to file->entry
						fat_rwCluster(file->mount, file->entry.start_cluster, (BYTE *)entry, FAT_WRITE);
						// terminate the cluster chain and commit the FAT to disk
						fat_setFATCluster(file->mount, file->entry.start_cluster, FAT_ENDOFCLUSTER, TRUE);
						mm_kfree(entry);
					}else {
						#ifdef
							kernel_printf("[create] start_cluster fail\n");
						#endif
						mm_kfree(dirname);
						mm_kfree(file);
						return FAIL;
					}
				}else {
					// set the attribute as a archive file
					file->entry.attribute.archive = TRUE;
					// we have no start cluster yet
					file->entry.start_cluster = FAT_FREECLUSTER;
				}
				// update the new files entry to disk
				ret = fat_updateFileEntry(file);
			}

		}

	}
	mm_kfree(dirname);
	mm_kfree(file);
	return res;
}


int fat_delete(struct VFS_MOUNTPOINT * mount, char * filename){
	int ret = FAIL;
	struct FAT_FILE * file;
	struct FAT_MOUNTPOINT * fat_mount;
	if ((fat_mount = (struct FAT_MOUNTPOINT *)mount->data_ptr) == NULL)
		return FAIL;
	file = (struct FAT_FILE *)mm_kmalloc(sizeof(struct FAT_FILE));
	file->mount = fat_mount;

	// try to find the file
	if (fat_file2entry(fat_mount, filename, NULL, file) == SUCCESS){
		// mark the file as deleted
		file->entry.name[0] = FAT_ENTRY_DELETED;
		// TO-DO, so when cluster is needed, it should also use this as free cluster
		ret = fat_setFileSize(file, 0);	
	}
	mm_kfree(file);
	return ret;
}

/* change the file name */
int fat_rename(struct VFS_MOUNTPOINT * mount, char * src, char * dst){
	int ret = FAIL;
	struct FAT_FILE * file;
	struct FAT_MOUNTPOINT * fat_mount;
	if ((fat_mount = (struct FAT_MOUNTPOINT *)mount->data_ptr) == NULL)
		return FAIL;
	file = (struct FAT_FILE *)mm_kmalloc(sizeof(struct FAT_FILE));
	file->mount = fat_mount;
	// try to find the file
	if (fat_file2entry(fat_mount, src, NULL, file) == SUCCESS){
		if ((dst = strrchr(dst, '/')) != NULL){
			// rename the entry (advancing the dest pointer past the /)
			if (fat_setFileName(file, ++dst) == SUCCESS)
				ret = fat_updateFileEntry(file);
		}
	}
	mm_kfree(file);
	return ret;
}

/* TO-DO, copy file */
int fat_copy(struct VFS_MOUNTPOINT * mount, char * src, char * dst){
	return FAIL;
}

/* ls */
struct VFS_DIRLIST_ENTRY * fat_list(struct VFS_MOUNTPOINT * mount, char * dirname){
	int dirIndex, entryIndex, nameIndex, extIndex;
	struct FAT_ENTRY * dir;
	struct VFS_DIRLIST_ENTRY * entry;
	// retrieve the fat_mount structure
	struct FAT_MOUNTPOINT * fat_mount;
	if ((fat_mont = (struct FAT_MOUNTPOINT *)mount->data_ptr) == NULL)
		return FAIL;
	if (strlen(dirname) == 0)
		dir = fat_mount->rootdir
	else{
		dir = (struct FAT_ENTRY *)mm_kmalloc(fat_mount->cluster_size);
		if (fat_file2entry(fat_mount, dirname, dir, NULL) < 0){
			mm_kfree(dir);
			return FAIL;
		}
		fat_rwCluster(fat_mount, dir->start_cluster, (BYTE *)dir, FAT_READ); // -> dir is storing the first file info (FAT_FILE)
	}

	entry = (struct VFS_DIRLIST_ENTRY *)mm_kmalloc(sizeof(struct VFS_DIRLIST_ENTRY) * 32);	// for each dir, it can has 32 entries (sub files)
	// clear it
	memset(entry, 0x00, sizeof(struct VFS_DIRLIST_ENTRY) * 32);

	for (dirIndex = 0, entryIndex = 0; entryIndex < 32 || dirIndex < 16; dirIndex ++){
		// test if their are any more entries
		if (dir[dirIndex].name[0] == 0x00)
			break;
		// if the file is deleted, continue past it
		if (dir[dirIndex].name[0] == FAT_ENTRY_DELETED)
			continue;
		// if the file size is negative, past it
		if (dir[dirIndex].file_size == -1)
			continue;
		// fill in the name
		memset(entry[entryIndex].name, 0x00, VFS_NAMESIZE);
		for (nameIndex = 0; nameIndex < FAT_NAMESIZE; nameIndex ++){
			if (dir[dirIndex].name[nameIndex] == FAT_PADBYTE)	// end of filename
				break;
			entry[entryIndex].name[nameIndex] = tolower(dir[dirIndex].name[nameIndex]);
		}
		// and the extension if there is one
		if (dir[dirIndex].extension[0] != FAT_PADBYTE){
			entry[entryIndex].name[nameIndex ++] = '.';
			for (extIndex = 0; extIndex < FAT_EXTENSIONSIZE; extIndex ++){
				if (dir[dirIndex].extension[extIndex] == FAT_PADBYTE)
					break;
				entry[entryIndex].name[nameIndex ++] = dir[dirIndex].extension[extIndex];
			}
		}
		// fill in the attributes
		if (dir[dirIndex].attributes.directory)
			entry[entryIndex].attributes.directory = VFS_DIRECTORY;
		else
			entry[entryIndex].attributes.directory = VFS_FILE;
		// file in the size
		entry[entryIndex].size = dir[dirIndex].file_size;
		entryIndex ++;
	}
	mm_kfree(dir);
	return entry;	// caller must free this buffer, it's better to parse buffer from caller *TO-DO
}


int fat_init(void){
	struct VFS_FILESYSTEM * fs;
	fs = (struct VFS_FILESYSTEM *)mm_kmalloc(sizeof(VFS_FILESYSTEM));
	// set the file system type
	fs->fstype = FAT_TYPE;
	// setup the file system calltable
	fs->calltable.open    = fat_open;
	fs->calltable.close   = fat_close;
	fs->calltable.clone   = fat_clone;
	fs->calltable.read    = fat_read;
	fs->calltable.write   = fat_write;
	fs->calltable.seek    = fat_seek;
	fs->calltable.control = fat_control;
	fs->calltable.create  = fat_create;
	fs->calltable.delete  = fat_delete;
	fs->calltable.rename  = fat_rename;
	fs->calltable.copy    = fat_copy;
	fs->calltable.list    = fat_list;
	fs->calltable.mount   = fat_mount;
	fs->calltable.unmount = fat_unmount;
	// register the file system with the VFS
	return vfs_register(fs);
}










