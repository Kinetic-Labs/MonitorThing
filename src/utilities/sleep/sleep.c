#include "sleep.h"

#include <unistd.h>

void msleep(const unsigned int miliseconds) {
	usleep(miliseconds * 1000);
}
