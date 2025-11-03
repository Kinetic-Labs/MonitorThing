#include "terminal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ANSI_GREEN "\x1b[32m"
#define ANSI_RESET "\x1b[0m"

void mt_terminal_clear_screen() {
	printf("\033c\n");
}

static size_t mt_terminal_strlen(const char *str) {
	return str ? strlen(str) : 0;
}

static char *mt_terminal_strdup(const char *str) {
	if(!str)
		return NULL;

	size_t len = strlen(str);
	char *dup = malloc(len + 1);

	if(dup)
		memcpy(dup, str, len + 1);

	return dup;
}

static size_t mt_terminal_max(size_t a, size_t b) {
	return a > b ? a : b;
}

static void mt_terminal_string_array_init(mt_terminal_string_array *arr, size_t initial_capacity) {
	arr->data = malloc(initial_capacity * sizeof(char *));
	arr->count = 0;
	arr->capacity = initial_capacity;
}

static void mt_terminal_string_array_free(mt_terminal_string_array *arr) {
	if(!arr || !arr->data)
		return;

	for(size_t i = 0; i < arr->count; i++)
		free(arr->data[i]);

	free(arr->data);
}

static void mt_terminal_row_array_init(mt_terminal_row_array *arr, size_t initial_capacity) {
	arr->data = malloc(initial_capacity * sizeof(char **));
	arr->count = 0;
	arr->capacity = initial_capacity;
}

static void mt_terminal_row_array_free(mt_terminal_row_array *arr, size_t row_size) {
	if(!arr || !arr->data)
		return;

	for(size_t i = 0; i < arr->count; i++) {
		if(arr->data[i]) {
			for(size_t j = 0; j < row_size; j++)
				free(arr->data[i][j]);

			free(arr->data[i]);
		}
	}

	free(arr->data);
	arr->data = NULL;
	arr->count = 0;
	arr->capacity = 0;
}

static void mt_terminal_size_array_init(mt_terminal_size_array *arr, size_t initial_capacity) {
	arr->data = malloc(initial_capacity * sizeof(size_t));
	arr->count = 0;
	arr->capacity = initial_capacity;
}

static void mt_terminal_size_array_free(mt_terminal_size_array *arr) {
	if(!arr || !arr->data)
		return;

	free(arr->data);
}

static void mt_terminal_row_array_push(mt_terminal_row_array *arr, char **row) {
	if(arr->count >= arr->capacity) {
		arr->capacity *= 2;
		arr->data = realloc(arr->data, arr->capacity * sizeof(char **));
	}

	arr->data[arr->count++] = row;
}

mt_terminal_table_logger *mt_terminal_table_logger_new(char **headers, size_t header_count) {
	mt_terminal_table_logger *logger = malloc(sizeof(mt_terminal_table_logger));

	if(!logger)
		return NULL;

	mt_terminal_string_array_init(&logger->headers, header_count);
	mt_terminal_row_array_init(&logger->rows, 16);
	mt_terminal_size_array_init(&logger->column_widths, header_count);

	for(size_t i = 0; i < header_count; i++) {
		logger->headers.data[i] = mt_terminal_strdup(headers[i]);
		logger->headers.count++;

		size_t width = mt_terminal_strlen(headers[i]);
		logger->column_widths.data[i] = width;
		logger->column_widths.count++;
	}

	logger->has_logged_header = false;

	return logger;
}

void mt_terminal_table_logger_free(mt_terminal_table_logger *logger) {
	if(!logger)
		return;

	mt_terminal_string_array_free(&logger->headers);
	mt_terminal_row_array_free(&logger->rows, logger->headers.count);
	mt_terminal_size_array_free(&logger->column_widths);

	free(logger);
}

void mt_terminal_table_logger_add_row(mt_terminal_table_logger *logger, char **row, size_t row_size) {
	if(!logger || !row)
		return;

	for(size_t i = 0; i < row_size && i < logger->column_widths.count; i++) {
		size_t cell_len = mt_terminal_strlen(row[i]);
		logger->column_widths.data[i] = mt_terminal_max(logger->column_widths.data[i], cell_len);
	}

	char **row_copy = malloc(row_size * sizeof(char *));
	for(size_t i = 0; i < row_size; i++)
		row_copy[i] = mt_terminal_strdup(row[i]);

	mt_terminal_row_array_push(&logger->rows, row_copy);
}

void mt_terminal_table_logger_log(mt_terminal_table_logger *logger) {
	if(!logger)
		return;

	printf("%s", ANSI_GREEN);

	if(!logger->has_logged_header) {
		printf("| ");

		for(size_t i = 0; i < logger->headers.count; i++)
			printf("%-*s | ", (int)logger->column_widths.data[i], logger->headers.data[i]);

		printf("\n");

		printf("|-");
		for(size_t i = 0; i < logger->column_widths.count; i++) {
			for(size_t j = 0; j < logger->column_widths.data[i]; j++)
				printf("-");

			printf("-|-");
		}

		printf("\b\b\n");

		logger->has_logged_header = true;
	}

	for(size_t i = 0; i < logger->rows.count; i++) {
		printf("| ");
		char **row = logger->rows.data[i];

		size_t row_size = logger->headers.count;

		for(size_t j = 0; j < row_size; j++) {
			size_t width = j < logger->column_widths.count ? logger->column_widths.data[j] : mt_terminal_strlen(row[j]);
			printf("%-*s | ", (int)width, row[j] ? row[j] : "");
		}

		printf("\n");
	}

	printf("%s", ANSI_RESET);

	mt_terminal_row_array_free(&logger->rows, logger->headers.count);
	mt_terminal_row_array_init(&logger->rows, 16);
}

char *mt_terminal_table_logger_to_string(const mt_terminal_table_logger *logger) {
	if(!logger)
		return NULL;

	size_t total_size = 1024;
	char *output = malloc(total_size);
	if(!output)
		return NULL;

	size_t pos = 0;

	for(size_t i = 0; i < logger->rows.count; i++) {
		char **row = logger->rows.data[i];
		size_t row_size = logger->headers.count;

		while(pos + 512 > total_size) {
			total_size *= 2;
			output = realloc(output, total_size);

			if(!output)
				return NULL;
		}

		pos += sprintf(output + pos, "| ");

		for(size_t j = 0; j < row_size; j++) {
			size_t width = j < logger->column_widths.count ? logger->column_widths.data[j] : mt_terminal_strlen(row[j]);
			pos += sprintf(output + pos, "%-*s | ", (int)width, row[j] ? row[j] : "");
		}

		pos += sprintf(output + pos, "\n");
	}

	return output;
}
