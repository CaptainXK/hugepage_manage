#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include <linux/limits.h>//PATH_MAX
#include "hugepage_memory.h"
#include "hugepage_malloc.h"

//#define MAX_LENGTH 10*1024*1024 //10MB
//#define TEST_HUGEPAGE_PATH "/mnt/hugepages/mem0"

int main(int argc, char** argv){
	int fd, fd_zero;
	void *addr=NULL, *try_addr=NULL;
	struct stat st;
	uint64_t hugepgsz;
	uint32_t ret, i;
	uint32_t num_pages=0;
	hugepage_file *tmp_hp;	

	//clean unused hugepage files
	clean_hugepages(HUGEPAGE_DIR);
	
	//get hugepage size in current system	
	hugepgsz = get_cur_hugepgsz();
	if(hugepgsz <= 0){
		printf("No hugepage available...\n");
		return 0;
	}
	
	//get free hugepages' number
	num_pages = get_num_hugepages();
	if(num_pages == 0)
	{
		printf("No hugepages available...\n");
		return 0;
	}
	
	tmp_hp = malloc(num_pages * sizeof(hugepage_file));
	if(tmp_hp == NULL)
	{
		perror("Malloc error");
		return 0;
	}
	ret = map_hugepages(tmp_hp, num_pages, hugepgsz);
	if(ret != num_pages){
		printf("hugepage files init error...\n");
		return 0;
	}
	else{
		printf("hugepage files init done...\n");
	}
	ret = pages_to_memsegs(tmp_hp, num_pages);
	if(ret == num_pages){
		printf("%u hugepage memsegs init done...\n", nb_memsegs);
	}
	ret = global_heap_init();
	if(ret == 0){
		printf("Malloc heap init done...\n");
	}
	else
		printf("Malloc heap init error...\n");
	show_heaps_state();
/*
	for(i=0; i<num_pages; i++)
	{
		printf("file path:%s\n",tmp_hp[i].file_path);
		printf("virtual addr:%lu\n",(uint64_t)tmp_hp[i].addr);
		printf("physical addr:%lu\n\n",tmp_hp[i].physaddr);
		unlink(tmp_hp[i].file_path);
	}			
*/
/*
	//try to map a contiguous virtaddri space with given length
	fd_zero = open("/dev/zero",O_RDONLY);
	if(fd_zero<0){
		perror("Zero open error:");
		return -1;
	}
	try_addr = mmap(try_addr, MAX_LENGTH, PROT_READ, MAP_PRIVATE, fd_zero, 0);
	if(try_addr == MAP_FAILED){
		perror("Map zero error:");
		close(fd_zero);
		return -1;
	}
	munmap(try_addr, MAX_LENGTH);
	close(fd_zero);	

	fd = open(TEST_HUGEPAGE_PATH, O_CREAT|O_RDWR, S_IRUSR|S_IWUSR);
	if(fd < 0){
		perror("Open Error:");
		return -1;
	}
	

//	//set file size
//	if( ftruncate(fd, MAX_LENGTH) != 0){
//		close(fd);
//		perror("Set File Size error:");
//		return -1;
//	}
	
	fstat(fd, &st);
	printf("Before mmap, file size is %ld\n", st.st_size);
	
	//mmap hugepage file
	addr = mmap(try_addr, MAX_LENGTH, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_POPULATE, fd, 0);
	if(addr == MAP_FAILED){
		perror("Map Error:");
		close(fd);
		unlink(TEST_HUGEPAGE_PATH);//remove file
		return -1;
	}
	
	
	fstat(fd, &st);
	printf("After mmap, file size is %ld\n", st.st_size);
	printf("Start virtual and physical address for first hugepage is %lu and %lu\n", (uint64_t)addr, physaddr);
	if(st.st_size == 0)
		goto EXIT;	

	memcpy(addr, "abcd", 4);
	printf("File content:%s\n",(char *)addr);
EXIT:		
	munmap(addr, MAX_LENGTH);
	close(fd);
	unlink(TEST_HUGEPAGE_PATH);
*/
	return 0;
}
