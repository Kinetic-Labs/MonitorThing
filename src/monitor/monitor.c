#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>

static host_cpu_load_info_data_t prev_cpuinfo = {0};
static int first_call = 1;

float mt_monitor_measure_cpu_usage() {
	host_cpu_load_info_data_t cpuinfo;
	mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

	if(host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) != KERN_SUCCESS)
		return -1.0;

	if(first_call) {
		prev_cpuinfo = cpuinfo;
		first_call = 0;
		return 0.0;
	}

	unsigned long long user_diff = cpuinfo.cpu_ticks[CPU_STATE_USER] - prev_cpuinfo.cpu_ticks[CPU_STATE_USER];
	unsigned long long nice_diff = cpuinfo.cpu_ticks[CPU_STATE_NICE] - prev_cpuinfo.cpu_ticks[CPU_STATE_NICE];
	unsigned long long sys_diff = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] - prev_cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
	unsigned long long idle_diff = cpuinfo.cpu_ticks[CPU_STATE_IDLE] - prev_cpuinfo.cpu_ticks[CPU_STATE_IDLE];

	prev_cpuinfo = cpuinfo;

	unsigned long long total_diff = user_diff + nice_diff + sys_diff + idle_diff;

	if(total_diff == 0)
		return 0.0;

	return ((float)(user_diff + nice_diff + sys_diff) / total_diff) * 100.0;
}

#else

static unsigned long long prev_user = 0, prev_nice = 0, prev_sys = 0, prev_idle = 0;
static unsigned long long prev_iowait = 0, prev_irq = 0, prev_softirq = 0;
static int first_call = 1;

float mt_monitor_measure_cpu_usage() {
	FILE *fp;
	char line[256];
	unsigned long long user, nice, sys, idle, iowait, irq, softirq;

	fp = fopen("/proc/stat", "r");
	if(fp == NULL)
		return -1.0;
	if(fgets(line, sizeof(line), fp) != NULL) {
		sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &sys, &idle, &iowait, &irq, &softirq);
	}
	fclose(fp);

	if(first_call) {
		prev_user = user;
		prev_nice = nice;
		prev_sys = sys;
		prev_idle = idle;
		prev_iowait = iowait;
		prev_irq = irq;
		prev_softirq = softirq;
		first_call = 0;
		return 0.0;
	}

	unsigned long long user_diff = user - prev_user;
	unsigned long long nice_diff = nice - prev_nice;
	unsigned long long sys_diff = sys - prev_sys;
	unsigned long long idle_diff = idle - prev_idle;
	unsigned long long iowait_diff = iowait - prev_iowait;
	unsigned long long irq_diff = irq - prev_irq;
	unsigned long long softirq_diff = softirq - prev_softirq;

	prev_user = user;
	prev_nice = nice;
	prev_sys = sys;
	prev_idle = idle;
	prev_iowait = iowait;
	prev_irq = irq;
	prev_softirq = softirq;

	unsigned long long total_diff =
	    user_diff + nice_diff + sys_diff + idle_diff + iowait_diff + irq_diff + softirq_diff;

	if(total_diff == 0)
		return 0.0;

	return ((float)(user_diff + nice_diff + sys_diff + iowait_diff + irq_diff + softirq_diff) / total_diff) * 100.0;
}

#endif
