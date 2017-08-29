#include <stdio.h>
#include "hugepage_malloc.h"
#include "string.h"

hugepage_malloc_heap global_malloc_heap[MAX_SOCKET_NB];

//init a elem
void malloc_elem_init(hugepage_malloc_elem *elem, hugepage_malloc_heap *heap, const hugepage_memseg *ms, size_t size)
{
	elem->heap = heap;
	elem->ms = ms;
	elem->prev = NULL;
	memset(&elem->free_list, 0, sizeof(elem->free_list));
	elem->size = size;
	elem->state = ELEM_FREE;		
}

//create end elem 
void malloc_elem_mkend(hugepage_malloc_elem *elem, hugepage_malloc_elem *prev)
{
	malloc_elem_init(elem, prev->heap, prev->ms, 0);
	elem->prev = prev;
	elem->state = ELEM_BUSY;
}

//find suitable free list id
size_t find_free_list_idx(size_t size)
{
	#define MALLOC_MINSIZE_LOG2 8
	#define MALLOC_LOG2_INCREMENT 2
	
	size_t log2;
	size_t index;
	
	if(size <= (1UL << MALLOC_MINSIZE_LOG2))
		return 0;

	//find the smallest 2 ** (log2) >= size
	log2 = sizeof(size) * 8 - __builtin_clzl(size-1);

	index = (log2 - MALLOC_MINSIZE_LOG2 + MALLOC_LOG2_INCREMENT - 1) / MALLOC_LOG2_INCREMENT;

	return index <= MAX_FREE_LIST_NB - 1 ? index: MAX_FREE_LIST_NB - 1; 	
}

//insert elem into free list
void malloc_elem_insert_freelist(hugepage_malloc_elem *elem)
{
	size_t idx;

    idx = find_free_list_idx(elem->size);	
	elem->state = ELEM_FREE;
	LIST_INSERT_HEAD(&elem->heap->free_head[idx], elem, free_list);	
}

//add a memseg to the heap of specified socket_id
uint32_t memsegs_to_heaps(hugepage_memseg *ms)
{
	int socket_id = ms->socket_id;
	hugepage_malloc_elem *start_elem = (hugepage_malloc_elem *)ms->addr;
	hugepage_malloc_elem *end_elem = (void *)((uintptr_t)start_elem + (ms->len - sizeof(hugepage_malloc_elem))); 	
 	end_elem = ALIGN_PTR_FLOOR(end_elem, ALIGN_SIZE);
	const size_t elem_size = (uintptr_t)end_elem - (uintptr_t)start_elem;	
	
	malloc_elem_init(start_elem, &global_malloc_heap[socket_id], ms, elem_size);
	malloc_elem_mkend(end_elem, start_elem);
	malloc_elem_insert_freelist(start_elem);
	
	global_malloc_heap[socket_id].total_size += elem_size;				
}

//global malloc hugepage heap initialization
uint32_t global_heap_init()
{
	uint32_t ms_cnt;
	hugepage_memseg *ms;
	
	memset(global_malloc_heap, 0, sizeof(hugepage_malloc_heap)*MAX_SOCKET_NB);

	for(ms = &global_memseg[0], ms_cnt = 0; 
			(ms_cnt < nb_memsegs) && ms->len > 0;
				ms++, ms_cnt++)
	{
		//add a memseg into heap
	    memsegs_to_heaps(ms);		
	}
	if(ms_cnt < nb_memsegs)
		return 1;
	else
		return 0;
}

//print current heaps' state
void show_heaps_state()
{
	uint32_t i;
	for(i=0; i<MAX_SOCKET_NB; i++){
		printf("heap_%u total_size:%lu using_size:%lu\n\n", i,
												global_malloc_heap[i].total_size,
												global_malloc_heap[i].using_size);
	}
	return;
}
