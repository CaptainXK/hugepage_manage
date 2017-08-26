#include <stdint.h>
#include <linux/limits.h>
#include "sysfs_ops.h"
#include <stdlib.h>//strtoul
#include <dirent.h>
#include <fnmatch.h>//fnmatch()
#include <sys/file.h>//flock()

#define HUGEPAGE_DIR "/mnt/hugepages"
#define ALIGN_ADDR(x,a) ((x + (a-1))&(~(a-1)))

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
int clean_hugepages(const char * huge_dir);

static char hugepage_size_str[256]="";
