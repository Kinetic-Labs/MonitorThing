#include "main.h"
#include "monitor/monitor.h"
#include "utilities/sleep/sleep.h"
#include "utilities/terminal/terminal.h"
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_POLL_RATE 36000
#define MIN_POLL_RATE 10

bool running = true;

void signal_handler(const int signum) {
	switch(signum) {
		case 2: {
			printf("\nRecieved SIGINT, shutting down...\n");
			running = false;
		}
	}
}

int kbhit() {
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF) {
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}

void check_quit() {
	if(kbhit()) {
		char the_char = getchar();

		if(the_char == 'q') {
			running = false;
		}
	}
}

void display_loop(const unsigned int poll_rate) {
	mt_terminal_clear_screen();

	printf("%s\n", license_message);
	printf("Starting in 1s...\n");
	sleep(1);

	const int HEADER_COUNT = 2;
	char *headers[] = {"CPU", "Memory"};
	char row1_buffer[64] = {0};
	char row2_buffer[64] = {0};
	char *row1[2] = {row1_buffer, row2_buffer};

	while(running) {
		mt_terminal_clear_screen();
		float cpu_usage = mt_monitor_measure_cpu_usage();
		float memory_usage = mt_monitor_measure_memory_usage_gb();

		if(cpu_usage == 0.0 || memory_usage == 0.0) {
			printf("Loading...\n");
			msleep(500);
			continue;
		}

		sprintf(row1_buffer, "Usage: %.1f%%", cpu_usage);
		sprintf(row2_buffer, "Usage: %.1f%%", cpu_usage);

		mt_terminal_table_logger *logger = mt_terminal_table_logger_new(headers, HEADER_COUNT);
		mt_terminal_table_logger_add_row(logger, row1, HEADER_COUNT);
		mt_terminal_table_logger_log(logger);
		mt_terminal_table_logger_free(logger);

		check_quit();

		msleep(poll_rate);
	}
}

void show_help() {
	printf("Usage: monitor_thing [OPTIONS]\n\n");
	printf("Options:\n");
	printf("  -h, --help              Show this help message\n");
	printf("  -p, --poll-rate RATE    Set polling rate in seconds (default: 1)\n");
	printf("                          Valid range: %d-%d seconds\n", MIN_POLL_RATE, MAX_POLL_RATE);
	printf("\nFor detailed documentation, see: man monitor_thing\n");
}

int parse_unsigned_int(const char *str, unsigned int *result) {
	char *endptr;
	long val;

	if(str == NULL || *str == '\0') {
		fprintf(stderr, "Error: Empty value provided\n");
		return 0;
	}

	errno = 0;
	val = strtol(str, &endptr, 10);

	if(errno == ERANGE || val > UINT_MAX || val < 0) {
		fprintf(stderr, "Error: Value out of range\n");
		return 0;
	}

	if(endptr == str) {
		fprintf(stderr, "Error: No digits found\n");
		return 0;
	}

	if(*endptr != '\0') {
		fprintf(stderr, "Error: Invalid characters in value\n");
		return 0;
	}

	*result = (unsigned int)val;

	return 1;
}

int main(int argc, char **argv) {
	signal(SIGINT, signal_handler);
	unsigned int poll_rate = 500; // ms

	for(int i = 1; i < argc; i++) {
		if(strncmp(argv[i], "-h", 3) == 0 || strncmp(argv[i], "--help", 7) == 0) {
			show_help();
			return 0;
		} else if(strncmp(argv[i], "-p", 3) == 0 || strncmp(argv[i], "--poll-rate", 12) == 0) {
			i++;
			if(i >= argc) {
				fprintf(stderr, "Error: --poll-rate requires an argument\n");
				show_help();
				return 1;
			}

			if(!parse_unsigned_int(argv[i], &poll_rate)) {
				fprintf(stderr, "Error: Invalid poll rate: %s\n", argv[i]);
				return 1;
			}

			if(poll_rate < MIN_POLL_RATE || poll_rate > MAX_POLL_RATE) {
				fprintf(stderr, "Error: Poll rate must be between %d and %d miliseconds\n", MIN_POLL_RATE,
				        MAX_POLL_RATE);
				return 1;
			}
		} else {
			fprintf(stderr, "Error: Unrecognized argument: %s\n", argv[i]);
			show_help();
			return 1;
		}
	}

	display_loop(poll_rate);
	mt_terminal_clear_screen();

	printf("Exiting...\n");

	return 0;
}
