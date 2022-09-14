#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include "common.h"


struct Option {
    /** Increased verbosity */
    unsigned char verbose;
    /** Overwrite existing file(s) */
    unsigned char clobber;
    /** PID to track */
    pid_t pid;
    /** Output file handle to track */
    FILE *file;
    /** Number of times per second mstat samples a pid */
    double sample_rate;
} option;

/**
 * Interrupt handler.
 * Called on exit.
 * @param sig the trapped signal
 */
static void handle_interrupt(int sig) {
    switch (sig) {
        case SIGUSR1:
            if (option.file) {
                fprintf(stderr, "flushed handle: %p\n", option.file);
                fflush(option.file);
            } else {
                fprintf(stderr, "flush request ignored. no handle");
            }
            return;
        case 0:
        case SIGTERM:
        case SIGINT:
            if (option.file) {
                fflush(option.file);
                fclose(option.file);
            }
            exit(0);
        default:
            break;
    }
}

static void usage(char *prog) {
    char *sep;
    char *name;

    sep = strrchr(prog, '/');
    name = prog;
    if (sep) {
        name = sep + 1;
    }
    printf("usage: %s [OPTIONS] <PID>\n"
           "-h        this help message\n"
           "-c        clobber output file if it exists\n"
           "-s        set sample rate (default: %0.2lf)\n"
           "\n", name, option.sample_rate);
}

/**
 * Parse program arguments and update global config
 * @param argc
 * @param argv
 */
void parse_options(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }
    for (int x = 0, i = 1; i < argc; i++) {
        if (strlen(argv[i]) > 1 && *argv[i] == '-') {
            char *arg = argv[i] + 1;
            if (!strcmp(arg, "h")) {
                usage(argv[0]);
                exit(0);
            }
            if (!strcmp(arg, "v")) {
                option.verbose = 1;
            } else if (!strcmp(arg, "s")) {
               option.sample_rate = strtod(argv[i+1], NULL);
               i++;
            } else if (!strcmp(arg, "c")) {
                option.clobber = 1;
            }
        } else {
            option.pid = (pid_t) strtol(argv[i], NULL, 10);
            x++;
        }
    }
}

/**
 * Check if `pid` directory exists in /proc
 * @param pid
 * @return 0 if exists and can read the directory. -1 if not
 */
int pid_exists(pid_t pid) {
    char path[PATH_MAX] = {0};
    snprintf(path, sizeof(path) - 1, "/proc/%d", pid);
    return access(path, F_OK | R_OK | X_OK);
}

/**
 * Check /proc/`pid` provides the smaps_rollups file
 * @param pid
 * @return 0 if exists and can read the file. -1 if not
 */
int smaps_rollup_usable(pid_t pid) {
    char path[PATH_MAX] = {0};
    snprintf(path, sizeof(path) - 1, "/proc/%d/smaps_rollup", pid);
    return access(path, F_OK | R_OK);
}


int main(int argc, char *argv[]) {
    struct mstat_record_t record;
    char path[PATH_MAX];

    // Set default options
    option.sample_rate = 1;
    option.verbose = 0;
    option.clobber = 0;
    // Set options based on arguments
    parse_options(argc, argv);

    // Allow user to flush the data stream with USR1
    signal(SIGUSR1, handle_interrupt);
    // Always attempt to exit cleanly
    signal(SIGINT, handle_interrupt);
    signal(SIGTERM, handle_interrupt);

    // For each PID passed to the program, begin gathering stats
    sprintf(path, "%d.mstat", option.pid);

    if (pid_exists(option.pid) < 0) {
        fprintf(stderr, "no pid %d\n", option.pid);
        exit(1);
    }

    if (smaps_rollup_usable(option.pid) < 0) {
        fprintf(stderr, "pid %d: %s\n", option.pid, strerror(errno));
        exit(1);
    }

    if (access(path, F_OK) == 0) {
        if (option.clobber) {
            remove(path);
            fprintf(stderr, "%s clobbered\n", path);
        } else {
            fprintf(stderr, "%s file already exists\n", path);
            exit(1);
        }
    }

    option.file = mstat_open(path);
    if (!option.file) {
        perror(path);
        exit(1);
    }

    if (mstat_check_header(option.file)) {
        mstat_write_header(option.file);
    }


    size_t i;
    struct timespec ts_start, ts_end;

    // Begin tracking time.
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    // Begin sample loop
    printf("PID: %d\nSamples per second: %.2lf\n(interrupt with ctrl-c...)\n", option.pid, option.sample_rate);
    i = 0;
    while (1) {
        memset(&record, 0, sizeof(record));
        record.pid = option.pid;

        // Record run time since last call
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        record.timestamp = mstat_difftimespec(ts_end, ts_start);

        // Sample memory values
        if (mstat_attach(&record, record.pid) < 0) {
            fprintf(stderr, "lost pid %d\n", option.pid);
            break;
        }

        if (option.verbose) {
            printf("pid: %d, sample: %zu, elapsed: %lf, rss: %li\n", record.pid, i, record.timestamp, record.rss);
        }

        if (mstat_write(option.file, &record) < 0) {
            fprintf(stderr, "Unable to write record to mstat file for pid %d\n", option.pid);
            break;
        }
        usleep((int) (1e6 / option.sample_rate));
        i++;
    }

    handle_interrupt(0);
    return 0;
}
