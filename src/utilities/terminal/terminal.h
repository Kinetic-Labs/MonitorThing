#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
	char **data;
	size_t count;
	size_t capacity;
} mt_terminal_string_array;

typedef struct {
	char ***data;
	size_t count;
	size_t capacity;
} mt_terminal_row_array;

typedef struct {
	size_t *data;
	size_t count;
	size_t capacity;
} mt_terminal_size_array;

typedef struct {
	mt_terminal_string_array headers;
	mt_terminal_row_array rows;
	mt_terminal_size_array column_widths;
	bool has_logged_header;
} mt_terminal_table_logger;

void mt_terminal_clear_screen();

mt_terminal_table_logger *mt_terminal_table_logger_new(char **headers, size_t header_count);

void mt_terminal_table_logger_free(mt_terminal_table_logger *logger);

void mt_terminal_table_logger_add_row(mt_terminal_table_logger *logger, char **row, size_t row_size);

void mt_terminal_table_logger_log(mt_terminal_table_logger *logger);

char *mt_terminal_table_logger_to_string(const mt_terminal_table_logger *logger);

#endif // TERMINAL_H
