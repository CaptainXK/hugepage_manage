#pragma once
#include <sched.h>
#include <unistd.h>//sysconf
#include <stdint.h>

extern int socket_id;
extern int core_id=0;
extern int cpu_id=0;

int get_runtime_info();
int get_cur_cpu_id();
int get_cur_core_id(int cpu_id);
int get_cur_socket_id(int cpu_id);
