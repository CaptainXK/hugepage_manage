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
//    idx      |     size   
//free_list[0]---(0,    2^ 8]
//free_list[1]---(2^ 8, 2^10]
//free_list[2]---(2^10, 2^12]
//free_list[3]---(2^12, 2^14]
//free_list[4]---(2^14, 2^16]
//...
size_t find_free_list_idx(size_t size)
{
	#define MALLOC_MINSIZE_LOG2 8
	#define MALLOC_LOG2_INCREMENT 2
	
	size_t log2;
	size_t index;
	
	//if size <= 2^8, return 0
	if(size <= (1UL << MALLOC_MINSIZE_LOG2))
		return 0;
	
	//find the smallest 2 ^ (log2) >= size
	//__builtin_clzl(long var) : In binary bits array of var ,number of 0 before the first "1" from left to right.
	//"log2" is the largest number that is power of 2 and smaller than "size"
	//for example, when size is 4096
	//the largest number, that is smaller than 4097 and is power of 2, is 4096
	//so the "log2" is log2(4096) = 12
	//unsigned long var consist of 64 bits
	//how to calculate?(size = 4096)
	//4096          = 00000000 00000000 00000000 00000000 00000000 00000000 00010000 00000000
	//4096-1 = 4095 = 00000000 00000000 00000000 00000000 00000000 00000000 00001111 11111111
	//log2 = 64 - 52 = 12 = log2(4096)  
	log2 = sizeof(size) * 8 - __builtin_clzl(size-1);

	//why "(log2-min_log2)+(increment - 1)/increment"?
	//for example:
	//we want to "7/2 = 3.5 = 4", but system will get 3 instead of 4
	//so we add 7 by "increment-1" to get result we want 	
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
