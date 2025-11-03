#include "terminal.h"
#include <stdio.h>

void mt_terminal_clear_screen() {
	printf("\033c\n");
}
