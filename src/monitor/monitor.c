#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>

static host_cpu_load_info_data_t prev_cpuinfo = {0};
static int first_call = 1;

float mt_monitor_measure_cpu_usage() {
	host_cpu_load_info_data_t cpuinfo;
	mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;

	if(host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) != KERN_SUCCESS)
		return -1.0f;

	if(first_call) {
		prev_cpuinfo = cpuinfo;
		first_call = 0;
		return 0.0f;
	}

	const unsigned long long user_diff = cpuinfo.cpu_ticks[CPU_STATE_USER] - prev_cpuinfo.cpu_ticks[CPU_STATE_USER];
	const unsigned long long nice_diff = cpuinfo.cpu_ticks[CPU_STATE_NICE] - prev_cpuinfo.cpu_ticks[CPU_STATE_NICE];
	const unsigned long long sys_diff = cpuinfo.cpu_ticks[CPU_STATE_SYSTEM] - prev_cpuinfo.cpu_ticks[CPU_STATE_SYSTEM];
	const unsigned long long idle_diff = cpuinfo.cpu_ticks[CPU_STATE_IDLE] - prev_cpuinfo.cpu_ticks[CPU_STATE_IDLE];

	prev_cpuinfo = cpuinfo;

	const unsigned long long total_diff = user_diff + nice_diff + sys_diff + idle_diff;
	if(total_diff == 0)
		return 0.0f;

	return (float)(user_diff + nice_diff + sys_diff) / (float) total_diff * 100.0f;
}

float mt_monitor_measure_memory_usage_gb() {
	vm_statistics64_data_t vm_stats;
	mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

	if(host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stats, &count) != KERN_SUCCESS)
		return -1.0f;

	vm_size_t page_size;
	host_page_size(mach_host_self(), &page_size);

	const unsigned long long used_bytes = (vm_stats.active_count + vm_stats.wire_count + vm_stats.compressor_page_count) *
	                                 (unsigned long long)page_size;

	return (float)used_bytes / (1024.0f * 1024.0f * 1024.0f);
}

#else

typedef struct {
	unsigned long long user;
	unsigned long long nice;
	unsigned long long system;
	unsigned long long idle;
	unsigned long long iowait;
	unsigned long long irq;
	unsigned long long softirq;
} cpu_stats_t;

static cpu_stats_t prev_stats = {0};
static int first_call = 1;

static int read_cpu_stats(cpu_stats_t *stats) {
	FILE *p_file = fopen("/proc/stat", "r");
	if(p_file == NULL)
		return -1;

	char line[256];
	int result = -1;

	if(fgets(line, sizeof(line), p_file) != NULL) {
		result = sscanf(line, "cpu %llu %llu %llu %llu %llu %llu %llu", &stats->user, &stats->nice, &stats->system,
		                &stats->idle, &stats->iowait, &stats->irq, &stats->softirq);
	}

	fclose(p_file);
	return (result == 7) ? 0 : -1;
}

float mt_monitor_measure_cpu_usage() {
	cpu_stats_t current_stats;

	if(read_cpu_stats(&current_stats) != 0)
		return -1.0;

	if(first_call) {
		prev_stats = current_stats;
		first_call = 0;
		return 0.0;
	}

	unsigned long long user_diff = current_stats.user - prev_stats.user;
	unsigned long long nice_diff = current_stats.nice - prev_stats.nice;
	unsigned long long sys_diff = current_stats.system - prev_stats.system;
	unsigned long long idle_diff = current_stats.idle - prev_stats.idle;
	unsigned long long iowait_diff = current_stats.iowait - prev_stats.iowait;
	unsigned long long irq_diff = current_stats.irq - prev_stats.irq;
	unsigned long long softirq_diff = current_stats.softirq - prev_stats.softirq;

	prev_stats = current_stats;

	unsigned long long total_diff =
	    user_diff + nice_diff + sys_diff + idle_diff + iowait_diff + irq_diff + softirq_diff;

	if(total_diff == 0)
		return 0.0;

	unsigned long long active_diff = user_diff + nice_diff + sys_diff + iowait_diff + irq_diff + softirq_diff;

	return ((float)active_diff / total_diff) * 100.0;
}

float mt_monitor_measure_memory_usage_gb() {
	FILE *p_file = fopen("/proc/meminfo", "r");
	if(p_file == NULL)
		return -1.0;

	char line[256];
	unsigned long long mem_total = 0, mem_available = 0;
	int found_total = 0, found_available = 0;

	while(fgets(line, sizeof(line), p_file) != NULL) {
		if(sscanf(line, "MemTotal: %llu kB", &mem_total) == 1) {
			found_total = 1;
		} else if(sscanf(line, "MemAvailable: %llu kB", &mem_available) == 1) {
			found_available = 1;
		}

		if(found_total && found_available)
			break;
	}

	fclose(p_file);

	if(!found_total || !found_available)
		return -1.0;

	unsigned long long used_kb = mem_total - mem_available;
	return (float)used_kb / (1024.0 * 1024.0);
}

#endif
