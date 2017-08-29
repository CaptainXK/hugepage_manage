#pragma once

#include <stdint.h>
#include <linux/limits.h>
#include "sysfs_ops.h"
#include <stdlib.h>//strtoul
#include <dirent.h>
#include <fnmatch.h>//fnmatch()
#include <sys/file.h>//flock()
#include <sys/queue.h>//LIST_***

#define HUGEPAGE_DIR "/mnt/hugepages"
#define MAX_MEMSEG 256
#define MAX_SOCKET_NB 8
#define MAX_FREE_LIST_NB 13
#define ALIGN_SIZE 64

//align to upper boundary
//page layout :|---page_1---|---page_2---|---....---|---page_n---|
//before align:    |addr-------|
//after align :             |addr--------|
#define ALIGN_ADDR(addr,size) ((addr + (size-1)) & (~(size-1)))

//align to down boundart
//align       :|---align---|---align---|...|---align---|
//before align:		|----val----|
//after align :|----val----|
#define ALIGN_FLOOR(val, align) \
	(typeof(val))( (val) & (~(typeof(val))((align) - 1)) )

#define ALIGN_PTR_FLOOR(ptr, align) \
	((typeof(ptr))ALIGN_FLOOR((uintptr_t)ptr, align))

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

//functions
uint64_t get_cur_hugepgsz();
uint32_t get_num_hugepages();
uint32_t map_hugepages(hugepage_file *hpf, uint32_t number, uint64_t hugepage_sz);
int clean_hugepages(const char * huge_dir);
uint32_t munmap_all_hugepages(hugepage_file *hpf, uint32_t page_number);
uint32_t pages_to_memsegs(hugepage_file *hpf, uint32_t page_number);

//global var
extern char hugepage_size_str[256];
extern hugepage_memseg global_memseg[MAX_MEMSEG];
extern uint32_t nb_memsegs;
