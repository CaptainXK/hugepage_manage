#include <stdint.h>
#include <linux/limits.h>
#include "sysfs_ops.h"
#include <stdlib.h>//strtoul
#include <dirent.h>
#include <fnmatch.h>//fnmatch()
#include <sys/file.h>//flock()

#define HUGEPAGE_DIR "/mnt/hugepages"
#define ALIGN_ADDR(addr,size) ((addr + (size-1)) & (~(size-1)))
#define MAX_MEMSEG 256

struct hugepage_file{
	void * addr;
	uint64_t physaddr;
	size_t pagesize;
	int file_id;
	int memseg_id;
	int socket_id;
	char file_path[PATH_MAX];
};

struct hugepage_memseg{
	uint64_t phys_addr;
	union{
		void * addr;
		uint64_t addr_64;
	};
	size_t len;
	uint64_t hugepage_sz;
	int socket_id;
}__attribute__((__packed__));

typedef struct hugepage_file hugepage_file;
typedef struct hugepage_memseg hugepage_memseg;

uint64_t get_cur_hugepgsz();
uint32_t get_num_hugepages();
uint32_t map_hugepages(hugepage_file *hpf, uint32_t number, uint64_t hugepage_sz);
int clean_hugepages(const char * huge_dir);
uint32_t pages_to_memsegs(hugepage_file *hpf, uint32_t page_number);
uint32_t munmap_all_hugepages(hugepage_file *hpf, uint32_t page_number);

static char hugepage_size_str[256]="";
static hugepage_memseg global_memseg[MAX_MEMSEG];
uint32_t nb_memsegs;
