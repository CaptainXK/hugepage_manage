#include <stdio.h>
#include <stdlib.h>//strtoul
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>//isspace()
#include <linux/limits.h>//PATH_MAX

#define MAX_LENGTH 10*1024*1024 //10MB
#define TEST_HUGEPAGE_PATH "/mnt/hugepages/mem0"

uint64_t get_cur_hugepgsz();
uint32_t get_num_hugepages();
int parse_sysfs_value(const char *filename, unsigned long *tmp);

char hugepage_size_str[256]="";

//parse sysfs value containing one integer value
int parse_sysfs_value(const char *filename, unsigned long *tmp)
{
	FILE *f;
	char buf[BUFSIZ];
	char *end = NULL;

	f = fopen(filename,"r");
	if(f==NULL){
		fprintf(stderr,"cannot not open \"%s\" to parse...\n",filename);
		return -1;
	}
	
	if(fgets(buf, sizeof(buf), f) == NULL){
		fprintf(stderr,"cannot not read \"%s\" to parse...\n",filename);
		goto OUT;
	}

	*tmp = strtoul(buf, &end, 0);
	if(buf[0] == '\0' || end == NULL || (*end != '\n')){
		fprintf(stderr,"cannot parse sysfs value \"%s\"...\n",filename);
		goto OUT; 
	}
	return 0;
	
	OUT:
		fclose(f);
		return -1;
}

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

int main(int argc, char** argv){
	int fd, fd_zero, fd_pgmap;
	void *addr=NULL, *try_addr=NULL;
	struct stat st;
	uint64_t physaddr;	
	uint64_t page, hugepgsz;
	int page_size;
	unsigned long virt_pfn;
	off_t offset;
	int ret;
	uint32_t num_pages=0;

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
		printf("No hugepages available");
		return 0;
	}
	
	//try to map a contiguous virtaddr with given length
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
	
/*
	//set file size
	if( ftruncate(fd, MAX_LENGTH) != 0){
		close(fd);
		perror("Set File Size error:");
		return -1;
	}
*/	
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
	
	//map virtaddr to physaddr
	page_size = getpagesize();
	fd_pgmap = open("/proc/self/pagemap", O_RDONLY);
	if(fd_pgmap < 0 ){
		perror("Can not open pagemap file:");
		goto EXIT;
	}
		
	virt_pfn = (unsigned long)addr / page_size;//calculate virtual page frame number
	offset = sizeof(uint64_t) * virt_pfn;//calculate offset
	if(lseek(fd_pgmap, offset, SEEK_SET) == (off_t) - 1){
		perror("seek error in pagemap:");
		close(fd_pgmap);
		goto EXIT;
	}	
	ret = read(fd_pgmap, &page, 8);
	if(ret<0){
		perror("Read pagemap error:");
		close(fd_pgmap);
		goto EXIT;
	}
	else if(ret != 8){
		printf("Read %d but expected 8...\n",ret);
		goto EXIT;
	}
	close(fd_pgmap);
	physaddr = ((page & 0x7fffffffffffffULL) * page_size) + ((unsigned long)addr % page_size);
	
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
	return 0;
}
