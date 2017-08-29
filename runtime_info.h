#pragma once
#include <sched.h>
#include <unistd.h>//sysconf
#include <stdint.h>

int get_cpu_id();
uint32_t get_core_id();
int get_socket_id(int cpu_id);
