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

	const size_t len = strlen(str);
	char *dup = malloc(len + 1);

	if(dup)
		memcpy(dup, str, len + 1);

	return dup;
}

static size_t mt_terminal_max(size_t a, size_t b) {
	return a > b ? a : b;
}

static void mt_terminal_string_array_init(mt_terminal_string_array *arr, const size_t initial_capacity) {
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

 // ReSharper disable once CppDFAConstantParameter
static void mt_terminal_row_array_init(mt_terminal_row_array *arr, const size_t initial_capacity) {
	arr->data = malloc(initial_capacity * sizeof(char **));
	arr->count = 0;
	arr->capacity = initial_capacity;
}

static void mt_terminal_row_array_free(mt_terminal_row_array *arr, const size_t row_size) {
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

static int mt_terminal_row_array_push(mt_terminal_row_array *arr, char **row) {
	if(arr->count >= arr->capacity) {
		const size_t new_capacity = arr->capacity * 2;
		char ***new_data = realloc(arr->data, new_capacity * sizeof(char **));
		if(!new_data)
			return 0;

		arr->data = new_data;
		arr->capacity = new_capacity;
	}

	arr->data[arr->count++] = row;
	return 1;
}

mt_terminal_table_logger *mt_terminal_table_logger_new(char **headers, size_t header_count) {
	mt_terminal_table_logger *logger = malloc(sizeof(mt_terminal_table_logger));

	if(!logger)
		return NULL;

	mt_terminal_string_array_init(&logger->headers, header_count);
	if(header_count > 0 && !logger->headers.data) {
		free(logger);
		return NULL;
	}

	mt_terminal_row_array_init(&logger->rows, 16);
	if(!logger->rows.data) {
		mt_terminal_string_array_free(&logger->headers);
		free(logger);
		return NULL;
	}

	mt_terminal_size_array_init(&logger->column_widths, header_count);
	if(header_count > 0 && !logger->column_widths.data) {
		mt_terminal_row_array_free(&logger->rows, 0);
		mt_terminal_string_array_free(&logger->headers);
		free(logger);
		return NULL;
	}

	for(size_t i = 0; i < header_count; i++) {
		logger->headers.data[i] = mt_terminal_strdup(headers[i]);
		if(!logger->headers.data[i] && headers[i]) {
			mt_terminal_table_logger_free(logger);
			return NULL;
		}
		logger->headers.count++;

		const size_t width = mt_terminal_strlen(headers[i]);
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

	const size_t num_columns = logger->headers.count;

	for(size_t i = 0; i < row_size && i < num_columns; i++) {
		const long unsigned int cell_len = mt_terminal_strlen(row[i]);
		logger->column_widths.data[i] = mt_terminal_max(logger->column_widths.data[i], cell_len);
	}

	char **row_copy = malloc(num_columns * sizeof(char *));
	if(!row_copy)
		return;

	for(size_t i = 0; i < num_columns; i++) {
		const char *cell_value = i < row_size ? row[i] : "";
		row_copy[i] = mt_terminal_strdup(cell_value);

		if(!row_copy[i] && cell_value) {
			for(size_t j = 0; j < i; j++)
				free(row_copy[j]);
			free(row_copy);
			return;
		}
	}

	if(!mt_terminal_row_array_push(&logger->rows, row_copy)) {
		for(size_t i = 0; i < num_columns; i++)
			free(row_copy[i]);
		free(row_copy);
	}
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

		const size_t row_size = logger->headers.count;

		for(size_t j = 0; j < row_size; j++) {
			const size_t width = j < logger->column_widths.count ? logger->column_widths.data[j] : mt_terminal_strlen(row[j]);
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
		const size_t row_size = logger->headers.count;

		while(pos + 512 > total_size) {
			const size_t new_size = total_size * 2;
			char *new_output = realloc(output, new_size);
			if(!new_output) {
				free(output);
				return NULL;
			}
			output = new_output;
			total_size = new_size;
		}

		pos += sprintf(output + pos, "| ");

		for(size_t j = 0; j < row_size; j++) {
			const size_t width = j < logger->column_widths.count ? logger->column_widths.data[j] : mt_terminal_strlen(row[j]);
			pos += sprintf(output + pos, "%-*s | ", (int)width, row[j] ? row[j] : "");
		}

		pos += sprintf(output + pos, "\n");
	}

	return output;
}
