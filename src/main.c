#include <stdio.h>
#include <unistd.h>

#include "monitor/monitor.h"
#include "utilities/terminal/terminal.h"
#include "utilities/types/bool.h"

int main(int argc, char **argv) {
	while(TRUE) {
		mt_terminal_clear_screen();
		float usage = mt_monitor_measure_cpu_usage();

		printf("CPU Usage: %.1f%s", usage, "%");
		fflush(stdout);
		sleep(1);
	}

	return 0;
}
