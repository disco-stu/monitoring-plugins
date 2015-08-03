
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/icinga.h"

#define VERSION "0.1"
#define PROCFS_STAT "/proc/stat"
#define BUFFER_LEN 1024

/*
 * define error messages
 */
#define ENOWARNING  "you must provide a warning threshold\n"
#define ENOCRITICAL "you must provide a critical threshold\n"

typedef struct stat
{
    long long    user;
    long long    nice;
    long long    system;
    long long    idle;
    long long    iowait;
    long long    irq;
    long long    softirq;
    time_t      taken;
} stat_t;

char fallback_tmpdir[] = "/tmp/";

/*
 * print_help:
 *
 * print help output to stdout
 */
void print_help (const char *progname)
{
    printf("Usage:\n");
    printf(" %s [options]\n", progname);
    printf("\n");
    printf("Options\n");
    printf(" -wu, --warning_user\t\twarning threshold (in percent)\n");
    printf(" -cu, --critical_user\t\tcriticalthreshold (in percent)\n");
    printf(" -v,      --verbose\t\tverbose output\n");
    printf("\n");
    printf(" -h,      --help\t\tdisplay this help text\n");
    printf(" -V,      --version\t\toutput version information\n");
}

/*
 * print_version:
 *
 * prints version information to stdout
 */
void print_version()
{
    printf("check_procstat (%s)\n", VERSION);
}

/*
 * exit_with_message:
 *
 * print a message to stdout and exit with return code rc
 */
void exit_with_message(int rc, char *message)
{
    fprintf(stdout, "%s - %s\n", state[rc], message);

    exit(rc);
}

/*
 * write_tmp_stats:
 *
 * writes a temporary stats file containing all statistics red from PROCFS_STAT
 * to /tmp
 *
 * this function will return UNKNOWN if we are not able to open that file for
 * writing
 */
void write_tmp_stats(char *tmpdir, stat_t *stat)
{
    FILE    *stats_file;
    char     path[BUFFER_LEN];

    sprintf(path, "%s%s", tmpdir, "/check_procstat.tmp");

    stats_file = fopen(path, "w+");

    if(!stats_file)
    {
        perror("/tmp/check_procstat.tmp");
        exit_with_message(CRITICAL, "could not open /tmp/check_procstats.tmp"
                "for writing");
    }

    fwrite(stat, sizeof(char), sizeof(stat_t), stats_file);

    fclose(stats_file);
}

/*
 * read_tmp_stats:
 *
 * Reads the temporary statistic information from a temporary file. 
 *
 * If it fails in some way opening the file (does not exists, no permission,
 * etc) it will return 0. Otherwise 1.
 */
int read_tmp_stats(char* tmpdir, stat_t *stat)
{
    FILE    *stats_file;
    char     path[BUFFER_LEN];

    sprintf(path, "%s%s", tmpdir, "/check_procstat.tmp");

    stats_file = fopen(path, "r+");

    if(!stats_file)
    {
        /*perror("/tmp/check_procstat.tmp");*/
        return 0;
    }

    fread(stat, sizeof(char), sizeof(stat_t), stats_file);

    fclose(stats_file);

    return 1;
}

/*
 * get_tmpdir:
 *
 * if set, this function returns TMPDIR from environment
 * otherwise TMP from environment.
 * If both are unset it returns a hard coded temporary
 * directory defined by global "fallback_tmpdir".
 */
char* get_tmpdir()
{
    char    *tmpdir;
    tmpdir = getenv("TMPDIR");

    if(NULL == tmpdir)
        tmpdir = getenv("TMP");

    if(NULL == tmpdir)
        tmpdir = fallback_tmpdir;

    return tmpdir;
}

int main(int argc, const char *argv[])
{
    FILE            *fd_progfs_stat;
    const char      *progname,
                    *arg;

    stat_t           stat,
                     /* initialize old_stat, just in case we got no old data */
                     old_stat = {0, 0, 0, 0, 0, 0, 0, 0,},
                     diff_stat = {0, 0, 0, 0, 0, 0, 0, 0,};

    struct tm       *tm_taken;

    char             err_message[BUFFER_LEN],
                     buffer[BUFFER_LEN];

    char            *tmpdir;

    int              verbose = 0,
                     stats_read = 0,
                     rc = OK,
                     i;

    long long        sum;

    double           p_user,
                     p_nice,
                     p_system,
                     p_idle,
                     p_iowait,
                     p_irq,
                     p_softirq,

                     /* declare default warning and critical values (percent) */
                     w_user    = 100.0,
                     w_nice    = 100.0,
                     w_system  = 100.0,
                     w_idle    = 100.0,
                     w_iowait  = 100.0,
                     w_irq     = 100.0,
                     w_softirq = 100.0,
                     c_user    = 100.0,
                     c_nice    = 100.0,
                     c_system  = 100.0,
                     c_idle    = 100.0,
                     c_iowait  = 100.0,
                     c_irq     = 100.0,
                     c_softirq = 100.0;

    /*
     * get the tmpdir from env (or fallback)
     */
    tmpdir = get_tmpdir();
    /*
     * parse the given arguments
     */
    if(argc > 0)
    {
        progname = argv[0];
        for (i = 1; i < argc; i++)
        {
            arg = argv[i];

            /*
             * if we got a parameter like -w or -c without a value, complain
             * about it
             */
            if((strcmp(arg,"-wu") == 0 || strcmp(arg,"--warning_user") == 0 ||
               strcmp(arg,"-cu") == 0  || strcmp(arg,"--critical_user") == 0) &&
               i+1 >= argc)
            {
                snprintf(err_message, BUFFER_LEN,
                        "you have to provide a value for %s", arg);
                exit_with_message(UNKNOWN, err_message);
            }

            if(strcmp(arg,"-wu") == 0 || strcmp(arg,"--warning_user") == 0)
                w_user = atof(argv[++i]);
            if(strcmp(arg,"-cu") == 0 || strcmp(arg,"--critical_user") == 0)
                c_user = atof(argv[++i]);
            if(strcmp(arg,"-v") == 0 || strcmp(arg,"--verbose") == 0)
                verbose = 1;
            if(strcmp(arg,"-h") == 0 || strcmp(arg,"--help") == 0)
                print_help(progname);
            if(strcmp(arg,"-V") == 0 || strcmp(arg,"--version") == 0)
                print_help(progname);
        }
    }

    if(verbose)
    {
        printf("Environment Variables used:\n");
        printf("  - tmpdir: %s\n", tmpdir);
        printf("Parameters:\n");
        printf("  - warning user: %f\n", w_user);
        printf("  - critical user: %f\n", c_user);
        printf("  - verbose: %d\n", verbose);
    }


    fd_progfs_stat = fopen(PROCFS_STAT, "r");
    if(fd_progfs_stat == 0)
    {
        perror(PROCFS_STAT);
        exit_with_message(CRITICAL, "could not open " PROCFS_STAT);
    }

    /*
     * read the first line of PROCFS_STAT
     * format: cpu  11575308 719865 3133282 32173965 850628 555908 416221 0 0 0
     */
    fgets(buffer, BUFFER_LEN, fd_progfs_stat);

    /* take the current (local) time */
    time(&stat.taken);
    tm_taken = localtime(&stat.taken);

    fclose(fd_progfs_stat);

    if(verbose)
        printf("Buffer:\n%s", buffer);

    sscanf(buffer, "cpu  %lld %lld %lld %lld %lld %lld %lld",
            &stat.user, &stat.nice, &stat.system, &stat.idle,
            &stat.iowait, &stat.irq, &stat.softirq);

    stats_read = read_tmp_stats(tmpdir, &old_stat);
    write_tmp_stats(tmpdir, &stat);

    if(verbose)
    {
        printf("Red:\n");
        printf("  - user %lld\n", stat.user);
        printf("  - nice %lld\n", stat.nice);
        printf("  - system %lld\n", stat.system);
        printf("  - idle %lld\n", stat.idle);
        printf("  - iowait %lld\n", stat.iowait);
        printf("  - irq %lld\n", stat.irq);
        printf("  - softirq %lld\n", stat.softirq);
        printf("  - taken: %s", asctime(tm_taken));
        printf("  - taken (raw): %ld\n", stat.taken);
    }

    if(verbose && stats_read)
    {
        tm_taken = localtime(&old_stat.taken);
        printf("Red (old_stat):\n");
        printf("  - user %lld\n", old_stat.user);
        printf("  - nice %lld\n", old_stat.nice);
        printf("  - system %lld\n", old_stat.system);
        printf("  - idle %lld\n", old_stat.idle);
        printf("  - iowait %lld\n", old_stat.iowait);
        printf("  - irq %lld\n", old_stat.irq);
        printf("  - softirq %lld\n", old_stat.softirq);
        printf("  - taken: %s", asctime(tm_taken));
        printf("  - taken (raw): %ld\n", old_stat.taken);
    }

    if(stats_read)
    {
        diff_stat.user = stat.user - old_stat.user;
        diff_stat.nice = stat.nice - old_stat.nice;
        diff_stat.system = stat.system - old_stat.system;
        diff_stat.idle = stat.idle - old_stat.idle;
        diff_stat.iowait = stat.iowait - old_stat.iowait;
        diff_stat.irq = stat.irq - old_stat.irq;
        diff_stat.softirq = stat.softirq - old_stat.softirq;
        diff_stat.taken = stat.taken - old_stat.taken;

        if(verbose)
        {
            printf("Red (old_stat):\n");
            printf("  - user %lld\n", diff_stat.user);
            printf("  - nice %lld\n", diff_stat.nice);
            printf("  - system %lld\n", diff_stat.system);
            printf("  - idle %lld\n", diff_stat.idle);
            printf("  - iowait %lld\n", diff_stat.iowait);
            printf("  - irq %lld\n", diff_stat.irq);
            printf("  - softirq %lld\n", diff_stat.softirq);
            printf("  - seconds ago %ld\n", diff_stat.taken);
        }
    }

    sum = diff_stat.user + diff_stat.nice + diff_stat.system + diff_stat.idle +
            diff_stat.iowait + diff_stat.irq + diff_stat.softirq;

    if(verbose)
        printf("  - sum: %lld\n", sum);

    /* calculate percentage */
    p_user = (double)diff_stat.user / sum * 100;
    p_nice = (double)diff_stat.nice / sum * 100;
    p_system = (double)diff_stat.system / sum * 100;
    p_idle = (double)diff_stat.idle / sum * 100;
    p_iowait = (double)diff_stat.iowait / sum * 100;
    p_irq = (double)diff_stat.irq / sum * 100;
    p_softirq = (double)diff_stat.softirq / sum * 100;

    if(verbose)
    {
        printf("Percentages:\n");
        printf("  - user: %f\n", p_user);
        printf("  - nice: %f\n", p_nice);
        printf("  - system: %f\n", p_system);
        printf("  - idle: %f\n", p_idle);
        printf("  - iowait: %f\n", p_iowait);
        printf("  - irq: %f\n", p_irq);
        printf("  - softirq: %f\n", p_softirq);
    }

    /*
     * check if any value is greater than the defined threshold and set rc
     * appropriate
     */
    if(p_user > w_user      || p_nice > w_nice     || p_system > w_system ||
            p_idle > w_idle || p_iowait > w_iowait || p_irq > w_irq       ||
            p_softirq > w_softirq)
        rc = WARNING;
    if(p_user > c_user      || p_nice > c_nice     || p_system > c_system ||
            p_idle > c_idle || p_iowait > c_iowait || p_irq > c_irq       ||
            p_softirq > c_softirq)
        rc = CRITICAL;

    sprintf(buffer, "user=%.2f nice=%.2f system=%.2f idle=%.2f iowait=%.2f irq=%.2f softirq=%.2f "
            "|user=%f%%;0;0 nice=%f%%;0;0 "
            "system=%f%%;0;0 idle=%f%%;0;0 "
            "iowait=%f%%;0;0 irq=%f%%;0;0 "
            "softirq=%f%%;0;0",
            p_user, p_nice, p_system, p_idle, p_iowait, p_irq, p_softirq,
            p_user, p_nice,
            p_system, p_idle,
            p_iowait, p_irq,
            p_softirq);
    exit_with_message(rc, buffer);

    /* suppress compiler warnings */
    return rc;
}
