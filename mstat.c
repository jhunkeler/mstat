#include <errno.h>
#include <dirent.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include "common.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
static int enable_cls = 1;

struct Option {
    /** Increased verbosity */
    unsigned char verbose;
    /** Overwrite existing file(s) */
    unsigned char clobber;
    /** Program to execute */
    char *program;
    /** PID to track */
    pid_t pid;
    /** PID subprocess status */
    int status;
    /** Output file handle to track */
    FILE *file;
    /** Output root*/
    char root[PATH_MAX];
    /** Output filename */
    char filename[PATH_MAX];
    /** Number of times per second mstat samples a pid */
    double sample_rate;
} option;

/**
 * Interrupt handler.
 * Called on exit.
 * @param sig the trapped signal
 */
static void handle_interrupt(int sig) {
    enable_cls = 0;
    switch (sig) {
        case SIGCHLD:
            waitpid(option.pid, &option.status, WNOHANG|WUNTRACED);
            if (WIFEXITED(option.status)) {
                printf("pid %d returned %d\n", option.pid, WEXITSTATUS(option.status));
            } else {
                fprintf(stderr, "warning: pid %d is likely defunct\n", option.pid);
            }
            return;
        case SIGUSR1:
            if (option.file) {
                if (option.verbose)
                    fprintf(stderr, "flushing %s\n", option.filename);
                fflush(option.file);
            } else {
                if (option.verbose)
                    fprintf(stderr, "flush request ignored. no handle\n");
            }
            return;
        case 0:
        case SIGTERM:
        case SIGINT:
            if (option.file) {
                fflush(option.file);
                fclose(option.file);
                printf("data written: %s\n", option.filename);
            }
            exit(0);
        default:
            break;
    }
    enable_cls = 1;
}

static void usage(char *prog) {
    char *sep;
    char *name;

    sep = strrchr(prog, '/');
    name = prog;
    if (sep) {
        name = sep + 1;
    }
    printf("usage: %s [OPTIONS] [-p PID] | {PROGRAM... ARGS}\n"
           "  -c        clobber 'PID#.mstat' if it exists\n"
           "  -h        this help message\n"
           "  -o DIR    path to output directory (must exist)\n"
           "  -p PID    process id to monitor\n"
           "  -s RATE   samples per second (default: %0.2lf)\n"
           "  -v        increased verbosity\n"
           "", name, option.sample_rate);
}

/**
 * Parse program arguments and update global config
 * @param argc
 * @param argv
 */
int parse_options(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }
    for (int i = 1; i < argc; i++) {
        if (strlen(argv[i]) > 1 && *argv[i] == '-') {
            char *arg = argv[i] + 1;
            if (!strcmp(arg, "h")) {
                usage(argv[0]);
                exit(0);
            }
            if (!strcmp(arg, "v")) {
                option.verbose = 1;
            } else if (!strcmp(arg, "c")) {
                option.clobber = 1;
            } else if (!strcmp(arg, "o")) {
                strncpy(option.root, argv[i+1], PATH_MAX - 1);
                i++;
            } else if (!strcmp(arg, "s")) {
                option.sample_rate = strtod(argv[i+1], NULL);
                i++;
            } else if (!strcmp(arg, "p")) {
                option.pid = (pid_t) strtol(argv[i+1], NULL, 10);
                i++;
            }
        } else {
            // Positional arguments begin here, return index
            return i;
        }
    }
    return -1;
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

static void clearscr() {
    if (!enable_cls)
        return;
    printf("\33[H");
    printf("\33[2J");
}

int main(int argc, char *argv[]) {
    struct mstat_record_t record;
    int positional;

    // Initialize options
    memset(&option, 0, sizeof(option));

    // Initialize record
    memset(&record, 0, sizeof(record));

    // Set default options
    option.sample_rate = 1;
    option.verbose = 0;
    option.clobber = 0;

    // Set options based on arguments
    positional = parse_options(argc, argv);
    if (!option.pid && positional < 0) {
        fprintf(stderr, "missing: -p PID, or PROGRAM with arguments\n\n");
        usage(argv[0]);
        exit(1);
    }

    // Wait for our children
    signal(SIGCHLD, handle_interrupt);
    // Allow user to flush the data stream with USR1
    signal(SIGUSR1, handle_interrupt);
    // Always attempt to exit cleanly
    signal(SIGINT, handle_interrupt);
    signal(SIGTERM, handle_interrupt);

    // Figure out what we are going to monitor.
    // Will it be a user-defined PID or a new process?
    if (option.pid) {
        // User-defined PID
        if (pid_exists(option.pid) < 0) {
            fprintf(stderr, "no pid %d\n", option.pid);
            exit(1);
        }
    } else {
        // New process
        pid_t p = fork();
        if (p == -1) {
            perror("fork");
            exit(1);
        }

        // "where" is the path to the program to execute
        char where[PATH_MAX] = {0};
        if (mstat_find_program(argv[positional], where)) {
            perror(argv[positional]);
            exit(1);
        }

        if (p == 0) {
            // Execute the requested program (with arguments)
            if (execv(where, &argv[positional]) < 0) {
                perror("execv");
                exit(1);
            }

            if (waitpid(option.pid, &option.status, WNOHANG | WUNTRACED) < 0) {
                perror("waitpid failed");
                exit(1);
            }
        }
        option.pid = p;
    }

    // Verify /proc/PID/smaps_rollup is present
    if (smaps_rollup_usable(option.pid) < 0) {
        fprintf(stderr, "pid %d: %s\n", option.pid, strerror(errno));
        exit(1);
    }

    // Set up output directory root and file path
    snprintf(option.filename, PATH_MAX - 1, "%d.mstat", option.pid);
    if (strlen(option.root)) {
        // Strip trailing slash from path
        if (strlen(option.root) > 1 && strrchr(option.root, '/')) {
            option.root[strlen(option.root) - 1] = '\0';
        }

        // Die if the output directory doesn't exist
        if (access(option.root, X_OK) < 0) {
            perror(option.root);
            exit(1);
        }

        // Construct new filename
        char tmppath[PATH_MAX];
        snprintf(tmppath, PATH_MAX - 1, "%s/%s", option.root, option.filename);
        strncpy(option.filename, tmppath, PATH_MAX - 1);
    }

    // Remove previous mstat data file if clobber is enabled
    if (access(option.filename, F_OK) == 0) {
        if (option.clobber) {
            remove(option.filename);
            fprintf(stderr, "%s clobbered\n", option.filename);
        } else {
            fprintf(stderr, "%s file already exists\n", option.filename);
            exit(1);
        }
    }

    // Initialize mstat data file
    option.file = mstat_open(option.filename);
    if (!option.file) {
        perror(option.filename);
        exit(1);
    }

    // Write mstat data file header, if necessary
    if (mstat_check_header(option.file)) {
        mstat_write_header(option.file);
    }

    size_t i;
    struct timespec ts_start, ts_end;
    extern char *mstat_field_names[];

    // Begin tracking time.
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    // Begin sample loop
    printf("PID: %d\nSamples per second: %.2lf\n",
           option.pid, option.sample_rate);
    printf("(interrupt with ctrl-c...)\n");

    i = 0;
    while (1) {
        if (option.verbose && isatty(STDOUT_FILENO)) {
            clearscr();
        }
        memset(&record, 0, sizeof(record));
        record.pid = option.pid;

        // Record run time since last call
        clock_gettime(CLOCK_MONOTONIC, &ts_end);
        record.timestamp = mstat_difftimespec(ts_end, ts_start);

        // Sample memory values
        if (mstat_attach(&record, record.pid) < 0) {
            if (positional < 0) {
                // '-p' monitoring: let the user know when the PID disappears
                fprintf(stderr, "pid: %d disappeared\n", option.pid);
            }
            break;
        }

        if (option.verbose) {
            printf("\nPID: %d, Sample: %zu, Elapsed: %lf\n----\n",
                   record.pid, i, record.timestamp);
            for (size_t n = 2, x = 0; mstat_field_names[n] != NULL; n++) {
                if (x == 3) {
                    x = 0;
                    puts("");
                }
                union mstat_field_t field;
                field = mstat_get_field_by_name(&record, mstat_field_names[n]);
                printf("\t%-16s %-8lu ", mstat_field_names[n], field.u64);
                x++;
            }
            puts("\n");
            printf("(interrupt with ctrl-c...)\n");
        }

        if (mstat_write(option.file, &record) < 0) {
            fprintf(stderr, "Unable to write record to mstat file for pid %d: %s\n",
                    option.pid, strerror(errno));
            break;
        }

        // Perform n samples per second
        usleep((int) (1e6 / option.sample_rate));
        i++;
    }

    handle_interrupt(0);
    return option.status;
}
