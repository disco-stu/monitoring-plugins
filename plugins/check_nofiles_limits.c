/*
 * Copyright 2015 Adrian Vondendriesch <adrian.vondendriesch@credativ.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/*
 * We have to define __USE_BSD, otherwise readlink
 * will not be available.
 *
 * See unistd.h for more information.
 */
#define __USE_BSD 1
#include <unistd.h>

#include "../include/icinga.h"

/* Program information. */
#define AUTHOR      "Adrian Vondendriesch"
#define PROCNAME    "check_nofiles_limits"
#define VERSION     "0.1"

/* Some procfs variables. */
#define PROCFS      "/proc/"
#define EXE         "/exe"
#define FDSDIR      "/fd"
#define LIMITFILE   "/limits"
#define STATUSFILE  "/status"

/* Numeric Chars to resolve "piddir" */
#define PIDCHARS    "0123456789"

/* Array limits. */
#define MAXBUF      512
#define MAXPIDS     8192

/* Define default warning and critical values. */
#define DEFAULTWARN 70.0
#define DEFAULTCRIT 80.0

/* Define some error messages. */
#define ENOEXECNAME    " No executable name specified."
#define ENOPNAME       " No processname specified."
#define ENOEXECORPNAME "No executable or process name was specified"
#define ENOSUCHEXEC    " No executable found with specified name."
#define ENOWARNVALUE   " No value for parameter warning specified."
#define ENOCRITVALUE   " No value for parameter critical specified."
#define EWARNINVALID   " Invalid value for warning threshold."
#define ECRITINVALID   " Invalid value for critical threshold."
#define EWARNCRIT      " Critical threshold must be greater then warning."

/*
 * Structure to hold variables for each process.
 *
 * Members:
 *  - long          pid:        pid of process
 *  - unsigned long current:    current number of open files
 *  - unsigned long soft_limit: soft limit for nofiles
 *  - unsigned long hard_limit: hard limit for nofiles
 */
typedef struct nofiles {
    long      pid;
    unsigned long current;
    unsigned long soft_limit;
    unsigned long hard_limit;
} nofiles_t;

/*
 * Structure to hold arguments, e.g. --warn ....
 */
typedef struct arguments {
    const char    *s_executable;
    const char    *s_process_name;
    double         nofiles_warn_threshold;
    double         nofiles_crit_threshold;
} arguments_t;

/*
 * print_version:
 *
 * Description:
 *  Prints version information and quit.
 *
 * Note:
 *  Will always exit with return code 0.
 */
void print_version()
{
    printf("%s, version: %s, author: %s\n",
            PROCNAME, VERSION, AUTHOR);

    exit(0);
}

/*
 * print_help:
 *
 * Description:
 *  Prints help and usage information and quit.
 *
 * Note:
 *  Will always exit with return code 0.
 */
void print_help(const char *s_this_name)
{
    printf("Usage: %s [option] (-e <executable_name> | -n <process_name>)\n"
           "\t Process identifier:\n"
           "\t -e, --executable: \tname of the executable\n"
           "\t -n, --processname:\tprocess name\n"
           "\n"
           "\tThresholds:\n"
           "\t -w, --warning:    \twarning threshold (percent)\n"
           "\t -c, --critical:   \tcritical threshold (percent)\n"
           "\n"
           "\tAdditional information:\n"
           "\t -h, --help:       \tprint this help message\n"
           "\t -V, --version:    \tprint version information\n"
           "\n"
           "\tDefault Values:\n"
           "\t warning:          \t%2.1lf %%\n"
           "\t critical:         \t%2.1lf %%\n",
           s_this_name, DEFAULTWARN, DEFAULTCRIT);

    exit(0);
}


/*
 * check_option:
 *
 * Description:
 * Checks a given option against short and long options.
 *
 * Arguments:
 *  - const char *s_option:     the option read from we check against
 *  - const char *s_short_opt:  the short option, e.g. "-h"
 *  - const char *s_long_opt:  the long option, e.g. "--help"
 *
 * Return Values:
 *  - 1 if either short or long option match the given option
 *  - 0 otherwise
 */
int check_option(const char *s_option, const char *s_short_opt, const char *s_long_opt)
{
    if(0 == strncmp(s_option, s_short_opt, MAXBUF) ||
       0 == strncmp(s_option, s_long_opt, MAXBUF))
        return 1;

    return 0;
}

/*
 * write_message:
 *
 * Description:
 *  Prints a message of the following format
 *    "OK/WARNING/CRITICAL/UNKNOWN - msg\n"
 *  based on rc and exits with return code rc.
 *
 * Arguments:
 *  - char *s_message:  the message that will be printed
 *  - int   rc:         exit code
 */
void write_message(char *s_message, int rc)
{
    printf("%s - %s\n", state[rc], s_message);

    exit(rc);
}

/*
 * read_nofiles_limit:
 *
 * Description:
 *  Reads soft an hard limits for "Max Open Files" from
 *  PROCFS/<pid>/LIMITS.
 *
 * Arguments:
 *  - struct nofiles *t_nofiles: structure where soft and hard limits will be
 *                               written tohold
 *
 * Return Value:
 *  returns the filled struct nofiles *t_nofiles
 *
 * Note:
 *  Will not return if something went wrong.
 */
struct nofiles*
read_nofiles_limit(struct nofiles *t_nofiles)
{
    FILE    *fd_limits;
    char     s_limits_path[MAXBUF];
    char     s_buffer[MAXBUF];
    char     s_limit_name[MAXBUF];
    char     s_limit_soft[MAXBUF];
    char     s_limit_hard[MAXBUF];

    /* Build the path to our limits file */
    snprintf(s_limits_path, sizeof(s_limits_path), "%s%ld%s",
            PROCFS, t_nofiles->pid, LIMITFILE);

    /*
     * Try to open the limits file, if this step fails we will fail too.
     */
    if(!(fd_limits = fopen(s_limits_path, "r")))
    {
        /* If we got here, a error occurred that should be reported */
        char    s_message[MAXBUF];

        snprintf(s_message, sizeof(s_message), "ERROR \"%s\", while open %s.",
                strerror(errno), s_limits_path);
        write_message(s_message, UNKNOWN);
    }

    /*
     * Scan for the line "Max open files" and read the associated limits.
     */
    while(fgets(s_buffer, MAXBUF, fd_limits))
    {
        sscanf(s_buffer, "%*s %*s %s %s %s",
                s_limit_name, s_limit_soft, s_limit_hard);
        if(0 == strncmp(s_limit_name, "files", MAXBUF))
        {
            t_nofiles->soft_limit = atol(s_limit_soft);
            t_nofiles->hard_limit = atol(s_limit_hard);

            /*
             * We found what we searched for,
             * stop scanning the file.
             */
            break;
        }
    }

    /* Return the original t_nofiles structure. */
    return t_nofiles;
}

/*
 * read_num_open_files:
 *
 * Description:
 *  Returns the number of currently opened files of a process,
 *  specified by pid.
 *
 * Arguments:
 *  - long  pid:    pid of the process
 *
 * Return Values:
 *  - n: the number of open files hold by this process
 *
 * Note:
 *  Will not return on error.
 */
int read_num_open_files(long pid)
{
    int              n_open_files=0;
    char             s_fds_path[MAXBUF];
    DIR             *dir_fds;
    struct dirent   *dir_entry;

    /* Build the full path to our fd directory. */
    snprintf(s_fds_path, sizeof(s_fds_path), "%s%ld%s",
            PROCFS, pid, FDSDIR);

    if( ! (dir_fds = opendir(s_fds_path)) )
    {
        /*
         * If there is any error opening the fd directory bail out.
         */
        char    msg[MAXBUF];

        snprintf(msg, MAXBUF, "ERROR: while open fd dir \"%s\": \"%s\"",
                s_fds_path, strerror(errno));

        write_message(msg, UNKNOWN);
    }

    /* Every file is one direntry. */
    while(NULL != (dir_entry = readdir(dir_fds)))
        n_open_files++;

    closedir(dir_fds);

    return n_open_files;
}

/*
 * cmp_process_name:
 *
 * Description:
 *  Returns the name of a process based on /proc/<pid>/status as
 *  "Name: ...".
 *
 * Arguments:
 *  - long        pid:  pid of the process
 *  - const char *name: searched process name
 *
 * Return Value:
 *  - 1 if name of the process equals s_name
 *  - 0 otherwise
 *
 * Note:
 *  Will not return on error.
 */
int
cmp_process_name(long pid, const char *s_name)
{
    FILE    *fd_status_file;

    char     s_title[MAXBUF];
    char     s_pname[MAXBUF];
    char     s_path[MAXBUF];


    /* Build the full path to our status file. */
    snprintf(s_path, sizeof(s_path), "%s%ld%s",
            PROCFS, pid, STATUSFILE);

    /*
     * Try to open the status file.
     */
    if( ! (fd_status_file = fopen(s_path, "r")) )
    {
        /*
         * If there is any error opening the status file bail out.
         */
        char    s_message[MAXBUF];

        snprintf(s_message, MAXBUF,
                "ERROR: while open status file \"%s\": \"%s\"",
                s_path, strerror(errno));

        write_message(s_message, UNKNOWN);
    }

    /*
     * We need the first line. TODO Error handling.
     */
    if( EOF == fscanf(fd_status_file, "%s %s", s_title, s_pname))
    {
        /*
         * If there is any error opening the status file bail out.
         */
        char    s_message[MAXBUF];

        fclose(fd_status_file);
        snprintf(s_message, MAXBUF,
                "ERROR: while reading status file \"%s\": \"%s\"\n",
                s_path, strerror(errno));

        write_message(s_message, UNKNOWN);
    }

    fclose(fd_status_file);

    if(0 != strcmp("Name:", s_title))
    {
        /*
         * If the status file don't begin with "Name: ...", bail out.
         */
        char    s_message[MAXBUF];

        snprintf(s_message, MAXBUF,
                "ERROR: status file \"%s\" has wrong format.",
                s_path);

        write_message(s_message, UNKNOWN);
    }

    /* Processname matches. */
    if(0 == strcmp(s_name, s_pname))
        return 1;

    /* Processname doesn't match. */
    return 0;
}

/*
 * build_exe_link:
 *
 * Description:
 *  Simple function to build the path of a exe link usage PROCFS and <pid>.
 *
 * Arguments:
 *  - char     *buffer:     preallocated char *buffer
 *  - char      pid:        pid we whant to build the link for
 *  - size_t    buffer_len: length of the preallocated char* buffer
 *
 * Return Codes:
 *  - 0 if no preallocated string is given or buffer_len is 0
 *  - 1 on success.
 */
int build_exe_link(char* s_buffer, long pid, size_t buffer_len)
{
    /*
     * We shoudl be sure that s_buffer is allocated.
     */
    if(buffer_len == 0 || !s_buffer)
        return 0;

    snprintf(s_buffer, buffer_len, "%s%ld%s",
            PROCFS, pid, EXE);

    return 1;
}

/*
 * read_exe_s_link:
 *
 * Description:
 *  Resolves the target of a specified softling.
 *
 * Arguments:
 *  - char  *s_link:        path to link that should be resoved
 *  - char  *s_buffer:      buffer where the resolved path will be
 *                          written to
 *  - size_t buffer_len:    length of s_buffer
 *
 * Return Value:
 *  Returns the end position of buffer. Points at '\0'.
 *
 * Note:
 *  If a error occured this function will not return.
 */
size_t linktarget(char *s_link, char *s_buffer, size_t buffer_len)
{
    int  pos;

    /* Do the actual s_link lookup. */
    pos = readlink(s_link, s_buffer, buffer_len);

    /*
     * We should check if any error occured and bail out if any.
     */
    if(0 > pos && errno != EACCES)
    {
        char    s_message[MAXBUF];

        snprintf(s_message, MAXBUF,
                "ERROR: while resolving softs_link \"%s\", \"%s\"",
                s_link, strerror(errno));

        write_message(s_message, UNKNOWN);
    }

    if(0 > pos)
        return pos;

    /*
     * Because reads_link do not insert a training '\0',
     * we do it manualy.
     */
    s_buffer[pos] = '\0';

    /*
     * Finaly we return the number of bytes that where
     * written to 's_buffer'. Just in case someone need
     * this position.
     */
    return pos;
}

/*
 * main:
 *
 * Description:
 *  Main function.
 */
int main(int argc, const char *argv[])
{
    int              count;
    int              n_pids = 0;
    int              n_files_total = 0;
    int              rc = 0;

    double           limit_percent = 0.0;
    double           limit_percent_max = 0.0;

    DIR             *dir_proc;
    struct dirent   *dir_entry;

    const char      *option;
    const char      *s_this_name = NULL;

    char            *s_exe_name;
    char             s_exe_link[MAXBUF];
    char             s_exe_link_target[MAXBUF];
    char             s_message[MAXBUF];
    char             s_message_pids[MAXBUF];
    char             s_message_pids_warn[MAXBUF];
    char             s_message_pids_crit[MAXBUF];
    char             s_message_perfdata[MAXBUF];
    char             s_message_tmp[MAXBUF];


    /* Structure to hold our pid information */
    nofiles_t        t_nofiles[MAXPIDS];

    /* Structure to hold our arguments */
    arguments_t      t_arguments =
    {
        NULL,
        NULL,
        DEFAULTWARN,
        DEFAULTCRIT
    };

    /*
     * Save the executable name.
     */
    if(argc > 0)
        s_this_name = argv[0];

    /*
     * Parse the arguments. Suppress the first element,
     * since it's the programname.
     */
    for(count=1; count<argc; count++)
    {
        option = argv[count];

        if(check_option(option, "-h", "--help"))
            print_help(s_this_name);
        else if(check_option(option, "-V", "--version"))
            print_version();
        else if(check_option(option, "-e", "--executable"))
        {
            if(++count >= argc)
                write_message(ENOEXECNAME, UNKNOWN);
            else
                t_arguments.s_executable = argv[count];
        }
        else if(check_option(option, "-n", "--processname"))
        {
            if(++count >= argc)
                write_message(ENOPNAME, UNKNOWN);
            else
                t_arguments.s_process_name = argv[count];
        }
        else if(check_option(option, "-w", "--warning"))
        {
            if(++count >= argc)
                write_message(ENOWARNVALUE, UNKNOWN);
            else
            {
                option = argv[count];
                t_arguments.nofiles_warn_threshold = strtod(option, NULL);
            }
        }
        else if(check_option(option, "-c", "--critical"))
        {
            if(++count >= argc)
                write_message(ENOCRITVALUE, UNKNOWN);
            else
            {
                option = argv[count];
                t_arguments.nofiles_crit_threshold = strtod(option, NULL);
            }
        }
    }

    /*
     * If we parsed the arguments, we should have executable defined.
     */
    if(!t_arguments.s_executable && !t_arguments.s_process_name)
        write_message(ENOEXECORPNAME, UNKNOWN);

    if(0.0 == t_arguments.nofiles_warn_threshold)
            write_message(EWARNINVALID, UNKNOWN);

    if(0.0 == t_arguments.nofiles_crit_threshold)
            write_message(ECRITINVALID, UNKNOWN);

    if(t_arguments.nofiles_crit_threshold <= t_arguments.nofiles_warn_threshold)
        write_message(EWARNCRIT, UNKNOWN);

    /*
     * We got the executable name, so we can got to work.
     */
    dir_proc = opendir(PROCFS);

    while(NULL != (dir_entry = readdir(dir_proc)))
    {
        long     pid;

        /* Skip directories and files which are not of format [0-9]* */
        if (!(strspn(dir_entry->d_name, PIDCHARS) == strlen(dir_entry->d_name))) {
            continue;
        }
        pid = atoi(dir_entry->d_name);
        /*printf("dirname %s\n", dir_entry->d_name);*/

        if(t_arguments.s_executable)
        {
            /*
             * Build the full path to the exe link.
             */
            rc = build_exe_link(s_exe_link, pid, sizeof(s_exe_link));

            /*
             * Resove the target of exe. If something went wront, move on.
             */
            rc = linktarget(s_exe_link, s_exe_link_target, MAXBUF);
            if(rc < 0)
                continue;

            /*
             * Now that we got a valid processes we should check that processes
             * againts the given executable name.
             */
            s_exe_name = basename(s_exe_link_target);
            if(0 != strncmp(s_exe_name, t_arguments.s_executable, MAXBUF))
                continue;
        }
        if(t_arguments.s_process_name)
        {
            /*
             * Check against the process name.
             */
            rc = cmp_process_name(pid, t_arguments.s_process_name);

            if(rc <= 0)
                continue;
        }

        /*
         * At this point we got a valid pid with the target executable name.
         */

        /*
         * Because we need to remember the found pids and limits we should save
         * that values.
         */
        t_nofiles[n_pids].pid = pid;
        read_nofiles_limit(&t_nofiles[n_pids]);

        /*
         * Get the number of currently open files to this process.
         */
        t_nofiles[n_pids].current = read_num_open_files(t_nofiles[n_pids].pid);

        n_pids++;
    }

    /* At this point we don't need the dir_proc anymore */
    closedir(dir_proc);

    /* Reset rc, just to be sure */
    rc = OK;
    /* We also need to initialize s_message_pids */
    s_message_pids[0] = '\0';

    /*
     * Check each process against the specified thresholds
     */
    for(count=0; count<n_pids; count++)
    {
        /*
         * Count the total number of files, that where in use by all processes.
         */
        n_files_total += t_nofiles[count].current;

        snprintf(s_message_tmp, MAXBUF, " %lu (%lu/%lu)",
                t_nofiles[count].pid, t_nofiles[count].current,
                t_nofiles[count].soft_limit);

        limit_percent =
            ( (double) t_nofiles[count].current / t_nofiles[count].soft_limit * 100);

        if(limit_percent > limit_percent_max)
        {
            limit_percent_max = limit_percent;

            /* Check against critical values */
            if(limit_percent > t_arguments.nofiles_crit_threshold)
            {
                rc = CRITICAL;
                strncat(s_message_pids_crit, s_message_tmp,
                        sizeof(s_message_pids_crit)-strlen(s_message_pids_crit));
            }
            /* Check against warning values */
            else if(limit_percent > t_arguments.nofiles_warn_threshold)
            {
                rc = WARNING;
                strncat(s_message_pids_warn, s_message_tmp,
                        sizeof(s_message_pids_warn)-strlen(s_message_pids_warn));
            }
        }
        else
        {
            /* All non warning or critical pids */
            strncat(s_message_pids, s_message_tmp,
                    sizeof(s_message_pids)-strlen(s_message_pids));
        }
    }

    /*
     * Build the coresponding s_message.
     */
    snprintf(s_message, sizeof(s_message),
            "Files in use: total: %d / per PID - ", n_files_total);
    snprintf(s_message_perfdata, sizeof(s_message_perfdata),
            "| total_files=%d;0;0 number_of_processes=%d;0;0",
            n_files_total, n_pids);

    /*
     * Print critical pids.
     */
    if(strlen(s_message_pids_crit))
    {
        strncat(s_message, " CRITICAL PIDs ",
                sizeof(s_message)-strlen(s_message));
        strncat(s_message, s_message_pids_crit,
                sizeof(s_message)-strlen(s_message));
    }

    /*
     * Print warning pids.
     */
    if(strlen(s_message_pids_warn))
    {
        strncat(s_message, " WARNING PIDs ",
                sizeof(s_message)-strlen(s_message));
        strncat(s_message, s_message_pids_warn,
                sizeof(s_message)-strlen(s_message));
    }

    /*
     * Print all other pids.
     */
    strncat(s_message, " OK PIDs ",
            sizeof(s_message)-strlen(s_message));

    strncat(s_message, s_message_pids,
            sizeof(s_message)-strlen(s_message));
    strncat(s_message, s_message_perfdata,
            sizeof(s_message)-strlen(s_message));

    /*
     * Last but no least: print the message.
     */
    write_message(s_message, rc);
}
