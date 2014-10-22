/*
   check_meminfo - quick&dirty plugin for nagios
   Copyright (C) 2006 Thomas A.F. Eckhardt <tafe unkelhaeusser net>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERSION "0.1.1"
#define PROCFS_MEMINFO "/proc/meminfo"
#define BUFFER_LEN 127
#define MAX_LEN_STATE 7

#define B_TO_TB ((double) 1024 * 1024 * 1024 * 1024)  
#define B_TO_GB ((double) 1024 * 1024 * 1024)  
#define B_TO_MB ((double) 1024 * 1024) 
#define B_TO_KB ((double) 1024)

/*
 * print_help:
 *
 * print help output to stdout
 */
void print_help (char* progname)
{
    printf("Usage:\n");
    printf(" %s [options]\n", progname);
    printf("\n");
    printf("Options\n");
    printf(" -w, --warning\t\twarning threshold (in bytes)\n");
    printf(" -c, --critical\t\tcriticalthreshold (in bytes)\n");
    printf(" -W, --Warning\t\twarning threshold (in percent)\n");
    printf(" -C, --Critical\t\tcriticalthreshold (in percent)\n");
    printf(" -v, --verbose\t\tverbose output\n");
    printf("\n");
    printf(" -h, --help\t\tdisplay this help text\n");
    printf(" -V, --version\t\toutput version information\n");
}

/*
 * print_version():
 *
 * prints version information to stdout
 */
void print_version()
{
    printf("check_meminfo (%s)\n", VERSION);
}

/*
 * print_error:
 *
 * prints a error message to stderr and exit with
 * return code 3 (UNKNOWN)
 */
void print_error(char* s_error)
{
    fprintf(stderr, "%s\n", s_error);
    exit(3);
}

/*
 * make_human_readable:
 *
 * Takes a allocated string of size BUFFER_LEN.
 * It wirte the size of any long int number in a human readable format (kB, MB,
 * GB, TB).
 */
void make_human_readable(char* human_readable, long int number)
{
    if(number >  B_TO_TB)
        sprintf(human_readable, "%.2lf TB", (double) number / B_TO_TB);
    else if(number > B_TO_GB)
        sprintf(human_readable, "%.2lf GB", (double) (number / B_TO_GB));
    else if(number > B_TO_MB)
        sprintf(human_readable, "%.2lf MB", (double) number / B_TO_MB);
    else if(number > B_TO_KB)
        sprintf(human_readable, "%.2lf kB", (double) number / B_TO_KB);
    else
        sprintf(human_readable, "%.2lf B", (double)  number);
}

int main (int argc, char** argv)
{
	FILE                *fd;

	char                 buffer[BUFFER_LEN],
                        *progname;
    char                 state[3][9] = {
                            "OK",
                            "WARNING",
                            "CRITICAL"},
                         memavailable_human_readable[BUFFER_LEN];

	long int             memtotal = 0,
                         memfree = 0,
                         membuffer = 0,
                         memcached = 0,
                         swaptotal = 0,
                         swapfree = 0;

	long int             memused = 0,
                         swapused = 0,
                         memavailable = 0;

    long int             warning = -1,
                         critical = -1;

    int                  warning_percent = -1,
                         critical_percent = -1;

    int                  verbose = 0,
                         i = 0,
                         state_rc = 0;

    double               memavailable_percent = 0;


    /*
     * parse arguments
     */
    if(argc && argc > 1)
    {
        progname = argv[0];
        for(i=0; i<argc; i++)
        {
            if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
            {
                /*
                 * print help and out
                 */
                print_help(progname);
                exit(0);
            }
            else if(strcmp(argv[i], "-V") == 0 || strcmp(argv[i], "--version") == 0)
            {
                print_version();
                exit(0);
            }
            else if(strcmp(argv[i], "-w") == 0 || strcmp(argv[i], "--warning") == 0)
            {
                if(i >= argc-1)
                    print_error("you have to provide a value for warning");
                warning = atoi(argv[++i]);
            }
            else if(strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--critical") == 0)
            {
                if(i >= argc-1)
                    print_error("you have to provide a value for critical");
                critical = atoi(argv[++i]);
            }
            else if(strcmp(argv[i], "-W") == 0 || strcmp(argv[i], "--Warning") == 0)
            {
                if(i >= argc-1)
                    print_error("you have to provide a value for Warning");
                warning_percent = atoi(argv[++i]);
            }
            else if(strcmp(argv[i], "-C") == 0 || strcmp(argv[i], "--Critical") == 0)
            {
                if(i >= argc-1)
                    print_error("you have to provide a value for Critical");
                critical_percent = atoi(argv[++i]);
            }
            else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0)
                verbose = 1;
        }
    }

    if(verbose)
    {
        printf("Variables:\n");
        printf("  - verbose:\ton\n");
        printf("  - warning:\t%ld\n", warning);
        printf("  - critical:\t%ld\n", critical);
        printf("  - Warning:\t%d\n", warning_percent);
        printf("  - Critical\t%d\n", critical_percent);
    }

    /*
     * check if warning is greater then critical
     */
    if(critical != -1 && warning != -1 &&
            critical > warning)
    {
        print_error("critical must be smaller then warning");
    }
    if(critical_percent != -1 && warning_percent != -1 &&
            critical_percent > warning_percent)
    {
        print_error("critical must be smaller then warning");
    }

    /*
     * check if percentage values are between 0 and 1
     */
    if(warning_percent != -1 && (warning_percent < 0 || warning_percent > 100))
        print_error("Warning can't be smaler then 0 or greater then 100");
    if(critical_percent != -1 && (critical_percent < 0 || critical_percent > 100))
        print_error("Critical can't be smaler then 0 or greater then 100");


    if(verbose)
        printf("fopen PROGFS_MEMINFO\n");

    /*
     * try to open PROGFS_MEMINFO, exit with error any errror occur
     */
	if ((fd = fopen (PROCFS_MEMINFO, "r")) == NULL)
    {
        perror("fopen() (PROGFS_MEMINFO) failed"); exit(3);
    }

    /*
     * Try to read line by line and get the information we need.
     * Because the in PROCFS_MEMINFO are kB we have to multiply this with 1024.
     */
	while ( fgets(buffer, BUFFER_LEN, fd) != NULL) {

		if (sscanf (buffer, "MemTotal:%ldkB", &memtotal)) {
            memtotal *= 1024;
		} else if (sscanf (buffer, "MemFree:%ldkB", &memfree)) {
            memfree *= 1024;
		} else if (sscanf (buffer, "Buffers:%ldkB", &membuffer)) {
            membuffer *= 1024;
		} else if (sscanf (buffer, "Cached:%ldkB", &memcached)) {
            memcached *= 1024;
		} else if (sscanf (buffer, "SwapTotal:%ldkB", &swaptotal)) {
            swaptotal *= 1024;
		} else if (sscanf (buffer, "SwapFree:%ldkB", &swapfree)) {
            swapfree *= 1024;
		}

	}

	fclose (fd);

    if(verbose)
        printf("fclose PROGFS_MEMINFO\n");

    /*
     * if memory is zero we did something wrong
     */
    if (memtotal == 0) {
        fprintf(stderr, "No memory? quitting..\n");
        exit(3);
    }

	memused = memtotal - memfree - membuffer - memcached;
	swapused = swaptotal - swapfree;
    memavailable = (memfree + (membuffer+memcached));
    memavailable_percent = ((double)memavailable / (double)memtotal) *100;

    if(verbose)
    {
        printf("Variables\n");
        printf("  - memtotal:\t%ld\n", memtotal);
        printf("  - memfree\t%ld\n", memfree);
        printf("  - membuffer\t%ld\n", membuffer);
        printf("  - memcached\t%ld\n", memcached);
        printf("  - swaptotal\t%ld\n", swaptotal);
        printf("  - swapfree\t%ld\n", swapfree);
        printf("Calculated variables\n");
        printf("  - memused\t%ld\n", memused);
        printf("  - swapused\t%ld\n", swapused);
        printf("  - memavailable\t%ld\n", memavailable);
        printf("  - memavailable_percent\t%f\n", memavailable_percent);
    }

    /*
     * set warning and critical to then minimal threshold (absolut or percent)
     */
    if(warning < (warning_percent * memtotal / 100))
        warning = (warning_percent * memtotal / 100);
    if(critical < (critical_percent * memtotal / 100))
        critical = (critical_percent * memtotal / 100);

    /*
     * becouse warning and critical are initialized with -1 we need to set both
     * values to 0, if nobody requested other thresholds
     */
    (warning == -1) ? (warning = 0) : warning;
    (critical == -1) ? (critical = 0) : critical;

    if(verbose)
    {
        printf("Thresholds\n");
        printf("  - warning\t%ld\n", warning);
        printf("  - critical\t%ld\n", critical);
    }

    /*
     * check if the available memory reaches the warning or critical level
     */
    if(memavailable <= critical)
        state_rc = 2;
    else if(memavailable <= warning)
        state_rc = 1;

    make_human_readable((char*)&memavailable_human_readable, memavailable);

	printf(
        "%s - Free: %4.2f %% (%s) "
        "|memavailable=%ldB;%ld;%ld;0.0, memtotal=%ld;0.0;0.0;0.0; "
        "memused=%ld;0.0;0.0;0.0; membuffer=%ld;0.0;0.0;0.0; "
        "memcached=%ld;0.0;0.0;0.0; swaptotal=%ld;0.0;0.0;0.0; "
        "swapused=%ld;0.0;0.0;0.0;\n",
		state[state_rc], memavailable_percent, memavailable_human_readable,
        memavailable, warning,
        critical, memtotal,
        memused, membuffer,
        memcached, swaptotal,
        swapused);

    exit(state_rc);
}
