#include <kernel/fs/fat.h>
#include <kernel/fs/vfs.h>
#include <kernel/io/io.h>
#include <kernel/mm/mm.h>
#include <kernel/kernel.h>
#include <lib/libc/ctype.h>
#include <lib/libc/string.h>

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

int fat_rwCluster(struct FAT_MOUNTPOINT * mount, int cluster, BYTE * clusterBuffer, int mode){
	int i, block;
	rw vfs_rw;

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
		memcpy(&dir[file->dir_index], &file->entry, sizeof(struct FAT_ENTRY));
		ret = fat_rwCluster(file->mount, file->dir_cluster, (BYTE *)dir, FAT_WRITE);
	}
	mm_kfree(dir);
	return res;
}

