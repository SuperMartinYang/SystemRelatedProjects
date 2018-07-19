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