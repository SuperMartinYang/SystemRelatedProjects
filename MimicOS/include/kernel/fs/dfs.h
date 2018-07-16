#ifndef _KERNEL_FS_DFS_H_
#define _KERNEL_FS_DFS_H_

#include <sys/types.h>
#include <kernel/io/io.h>

#define DFS_TYPE 	2

struct DFS_ENTRY {
	struct DFS_ENTRY * prev;
	struct IO_CALLTABLE * calltable;	// SEE THE WEIRD PTR, USED IN THE END OF DFS.C
	char * name;
	int type;
};

struct DFS_ENTRY * dfs_add(char *, struct IO_CALLTABLE *, int);

int dfs_remove(char *);

int dfs_init(void);

#endif