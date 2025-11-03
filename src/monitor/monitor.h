#ifndef MONITOR_H
#define MONITOR_H

/**
 * Measures CPU usage as a percentage.
 * Returns: CPU usage percentage (0.0 - 100.0), or -1.0 on error
 * Note: First call returns 0.0 to establish baseline
 */
float mt_monitor_measure_cpu_usage(void);

/**
 * Measures memory usage in gigabytes.
 * Returns: Used memory in GB, or -1.0 on error
 */
float mt_monitor_measure_memory_usage_gb(void);

#endif // MONITOR_H
