#pragma once
#include "hugepage_memory.h"

struct hugepage_malloc_heap;//dummy definition of hugepage_heap struct to use it in hugepage_malloc_elem

enum elem_state{
	ELEM_FREE = 0,
	ELEM_BUSY,
	ELEM_EMPTY
};

struct hugepage_malloc_elem{
	struct hugepage_malloc_heap *heap;
	struct hugepage_malloc_elem *volatile prev;
	LIST_ENTRY(hugepage_malloc_elem) free_list;
	const struct hugepage_memseg *ms;
	size_t size; 
	enum elem_state state;
}__attribute__((__aligned__(64)));

typedef struct hugepage_malloc_elem hugepage_malloc_elem;

struct hugepage_malloc_heap{
	LIST_HEAD(	,hugepage_malloc_elem) free_head[MAX_FREE_LIST_NB];
	size_t total_size;
	size_t using_size;
}__attribute__((__aligned__(64)));

typedef struct hugepage_malloc_heap hugepage_malloc_heap;

//functions
uint32_t global_heap_init();
void show_heaps_state();

//global vars
extern hugepage_malloc_heap global_malloc_heap[MAX_SOCKET_NB];

