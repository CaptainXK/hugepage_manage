#define _GNU_SOURCE
#include "runtime_info.h"
#include "sysfs_ops.h"
#include "hugepage_memory.h"
#include <linux/limits.h>
#include <stdio.h>

int get_cpu_id()
{
	int i,cpu_nb;
	cpu_set_t cpuset;

	//get cpu set
	if(sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) == -1){
		perror("Can not get cpu affinity information:");
		return -1;
	}
	
	//get logical cpu number
	cpu_nb = sysconf(_SC_NPROCESSORS_CONF);
	
	for(i=0; i<cpu_nb; i++){
		if(CPU_ISSET(i, &cpuset)){
			return i;
		}
	}
	return -1;
}

uint32_t get_core_id(int cpu_id)
{	
	char path[PATH_MAX];
	unsigned long val;
	
	int len = sprintf(path,"/sys/devices/system/cpu/cpu%d/topology/core_id",cpu_id);	
	if(len <=0 || len>=sizeof(path))
	{
		printf("Read core_id file error in copy path...\n");
		return -1;
	}
	
	if( parse_sysfs_value(path, &val) )
	{
		printf("Read core_id error in file reading...\n");		
	}
	
	return (uint32_t)val;
}

int get_socket_id(int cpu_id)
{
	char path[PATH_MAX];
	int socket_id;
	
	for(socket_id = 0; socket_id < MAX_SOCKET_NB; socket_id ++){
		sprintf(path, "/sys/devices/system/node/node%d/cpu%d", socket_id, cpu_id);
		if(access(path, F_OK) == 0){
			return socket_id;
		}
	}
	return -1;
}
