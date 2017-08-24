#include "hugepage_memory.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>//isspace()
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>


uint64_t virt_to_phys(void * addr);

//map virtaddr to physaddr
uint64_t virt_to_phys(void * addr)
{
	int page_size;
    unsigned long virt_pfn;
	off_t offset;
	int ret;
	uint64_t physaddr, page;
	int fd_pgmap;
			
	page_size = getpagesize();
	fd_pgmap = open("/proc/self/pagemap", O_RDONLY);
	if(fd_pgmap < 0 ){
		perror("Can not open pagemap file:");
		return 0;
	}
		
	virt_pfn = (unsigned long)addr / page_size;//calculate virtual page frame number
	offset = sizeof(uint64_t) * virt_pfn;//calculate offset
	if(lseek(fd_pgmap, offset, SEEK_SET) == (off_t) - 1){
		perror("seek error in pagemap:");
		goto EXIT;
	}	
	ret = read(fd_pgmap, &page, 8);
	if(ret<0){
		perror("Read pagemap error:");
		goto EXIT;
	}
	else if(ret != 8){
		printf("Read %d but expected 8...\n",ret);
		goto EXIT;
	}
	close(fd_pgmap);
	physaddr = ((page & 0x7fffffffffffffULL) * page_size) + ((unsigned long)addr % page_size);
	
	return physaddr;
EXIT:
	close(fd_pgmap);
	return 0;	
}

//get current system's hugepage size
uint64_t get_cur_hugepgsz()
{
	FILE * fd = fopen("/proc/meminfo", "r");
	char buf[256];
	const char hugepagesz_str[]="Hugepagesize:";
	int i=0,j=0;
	uint64_t size = 0;
	int pgsz_str_len = strlen(hugepagesz_str);
	
	if(fd == NULL){
		perror("Can not open meminfo:");
		return -1;
	}
	while(fgets(buf, sizeof(buf), fd)){
		if(strncmp(buf, hugepagesz_str, pgsz_str_len) == 0){
			i = pgsz_str_len;
			while( isspace( (int)buf[i] ) )
				i++;
			while( buf[i] >= '0' && buf[i] <= '9' ){
				size = size*10 + (int)(buf[i]-'0');
				i++;
			}
			if(buf[i]==' ')
				i++;
			sprintf(hugepage_size_str, "hugepages-%lu%c%c",size,buf[i],buf[i+1]);
			switch(buf[i]){
				case 'G':
				case 'g': size *= 1024;
				case 'M':
				case 'm': size *= 1024;
				case 'K':
				case 'k': size *= 1024;
				default:
					break;
			}
		}	
	}
	return size;
}

//get current avaliable hugepages
uint32_t get_num_hugepages()
{
	char resv_path[PATH_MAX]="";
	char num_path[PATH_MAX]="";
	long unsigned resv_pages, num_pages;	

	if(strcmp(hugepage_size_str," ") == 0){
		printf("No valid hugepage size...\n");
		return -1;
	}
	sprintf(resv_path,"/sys/kernel/mm/hugepages/%s/resv_hugepages", hugepage_size_str);
	sprintf(num_path,"/sys/kernel/mm/hugepages/%s/free_hugepages", hugepage_size_str);
    if(parse_sysfs_value(resv_path, &resv_pages) != 0)
		return 0;	
    if(parse_sysfs_value(num_path, &num_pages) != 0)
		return 0;	
	if(num_pages == 0){
		printf("No free hugepages...\n");
		return 0;
	}
	if(num_pages >= resv_pages)
		num_pages -= resv_pages;
	else if(resv_pages != 0)
		num_pages = 0;
	if(num_pages > UINT32_MAX)
		num_pages = UINT32_MAX;
		
	return num_pages;	
}

//map hugepage file into userspace
uint32_t map_hugepages(hugepage_file *hpf, uint32_t number, uint64_t hugepage_sz)
{
	uint32_t i;
	int fd;
	void * virtaddr;
	void * vma_addr = NULL;
	
	for(i=0; i<number; i++){
		hpf[i].file_id = i;
		memset(hpf[i].file_path, 0, sizeof(hpf[i].file_path));
		sprintf(hpf[i].file_path, "/mnt/hugepages/hugepage_map%u", hpf[i].file_id);
		hpf[i].file_path[sizeof(hpf[i].file_path)-1] = '\0';
		fd = open(hpf[i].file_path, O_CREAT | O_RDWR, 0600);
		if(fd<0){
			perror("hugepage file open error:");
			return i;
		}
		virtaddr = mmap(vma_addr, hugepage_sz, PROT_READ|PROT_WRITE, MAP_SHARED | MAP_POPULATE, fd, 0);	
		if(virtaddr == MAP_FAILED){
			perror("Map hugepage file error:");
			return i;
		}
		hpf[i].addr = virtaddr;
		hpf[i].physaddr = virt_to_phys(hpf[i].addr);
		if(hpf[i].physaddr == 0){
			printf("Fail to get physaddr of %u page...\n", i);
			return i;
		}	
	}
	
	return i;
}