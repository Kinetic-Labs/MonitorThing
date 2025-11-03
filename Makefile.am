AM_CFLAGS = -std=c99 -Werror

bin_PROGRAMS = monitor_thing

monitor_thing_SOURCES = \
	src/main.c \
	src/monitor/monitor.c \
	src/utilities/terminal/terminal.c

noinst_HEADERS = \
	src/monitor/monitor.h \
	src/utilities/terminal/terminal.h \
	src/utilities/types/bool.h

run: monitor_thing
	./monitor_thing

clean-local:
	rm -f *~
	rm -f src/*~
	rm -f *.o
	rm -f src/*.o
	rm -f core
	rm -f *.log
