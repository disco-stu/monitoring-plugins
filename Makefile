CC=gcc
CFLAGS=-Wall --pedantic -std=c99 -g 
LDFLAGS=
SOURCEDIR=plugins
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=check_meminfo check_procstat check_nofiles_limits

all: $(EXECUTABLE)

check_meminfo: check_meminfo.o
	$(CC) $(LDFLAGS) $< -o $@

check_procstat: check_procstat.o
	$(CC) $(LDFLAGS) $< -o $@

check_nofiles_limits: check_nofiles_limits.o
	$(CC) $(LDFLAGS) $< -o $@

%.o: $(SOURCEDIR)/%.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f *.o
	rm -f $(EXECUTABLE)
