#include <stdint.h>
#include <linux/limits.h>
#include "sysfs_ops.h"
#include <stdlib.h>//strtoul

struct hugepage_file{
	void * addr;
	uint64_t physaddr;
	size_t pagesize;
	int file_id;
	int memseg_id;
	int socket_id;
	char file_path[PATH_MAX];
};

typedef struct hugepage_file hugepage_file;

uint64_t get_cur_hugepgsz();
uint32_t get_num_hugepages();
uint32_t map_hugepages(hugepage_file *hpf, uint32_t number, uint64_t hugepage_sz);

static char hugepage_size_str[256]="";
