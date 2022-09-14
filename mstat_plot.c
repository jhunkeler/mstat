//
// Created by jhunk on 8/24/22.
//

#include <string.h>
#include <stdarg.h>
#include "common.h"

extern char *mstat_field_names[];

struct GNUPLOT_PLOT {
   char *title;
   char *xlabel;
   char *ylabel;
   char *line_type;
   double line_width;
   unsigned int line_color;
   unsigned char grid_toggle;
   unsigned char autoscale_toggle;
   unsigned char legend_toggle;
   unsigned char legend_enhanced;
   char *legend_title;
};

struct Option {
    unsigned char verbose;
    char *fields[0xffff];
    char filename[PATH_MAX];
} option;


/**
 * Determine if `name` is available on `PATH`
 * @param name of executable
 * @return 0 on success. >0 on error
 */
static int find_program(const char *name) {
    char *path;
    char *pathtmp;
    char *token;

    pathtmp = getenv("PATH");
    if (!pathtmp) {
        return 1;
    }
    path = strdup(pathtmp);
    while ((token = strsep(&path, ":")) != NULL) {
        if (access(token, F_OK | X_OK) == 0) {
            return 0;
        }
    }
    return 1;
}

/**
 * Open a new gnuplot handle
 * @return stream on success, or NULL on error
 */
FILE *gnuplot_open() {
    // -p = persistent window after exit
    return popen("gnuplot -p", "w");
}

/**
 * Close a gnuplot handle
 * @param fp pointer to gnuplot stream
 * @return 0 on success, or <0 on error
 */
int gnuplot_close(FILE *fp) {
    return pclose(fp);
}

/**
 * Send shell command to gnuplot instance
 * @param fp pointer to gnuplot stream
 * @param fmt command to execute (requires caller to end string with a LF "\n")
 * @param ... formatter arguments
 * @return value of `vfprintf()`. <0 on error
 */
int gnuplot_sh(FILE *fp, char *fmt, ...) {
    int status;

    va_list args;
    va_start(args, fmt);
    status = vfprintf(fp, fmt, args);
    va_end(args);

    return status;
}

/**
 * Generate a plot
 * Each GNUPLOT_PLOT pointer in the `gp` array corresponds to a line.
 * @param fp pointer to gnuplot stream
 * @param gp pointer to an array of GNUPLOT_PLOT structures
 * @param x an array representing the x axis
 * @param y an array of double-precision arrays representing the y axes
 * @param x_count total length of array x
 * @param y_count total number of arrays in y
 */
void gnuplot_plot(FILE *fp, struct GNUPLOT_PLOT **gp, double x[], double *y[], size_t x_count, size_t y_count) {
    // Configure plot
    gnuplot_sh(fp, "set title '%s'\n", gp[0]->title);
    gnuplot_sh(fp, "set xlabel '%s'\n", gp[0]->xlabel);
    gnuplot_sh(fp, "set ylabel '%s'\n", gp[0]->ylabel);
    if (gp[0]->grid_toggle)
        gnuplot_sh(fp, "set grid\n");
    if (gp[0]->autoscale_toggle)
        gnuplot_sh(fp, "set autoscale\n");
    if (gp[0]->legend_toggle) {
        gnuplot_sh(fp, "set key nobox\n");
        //gnuplot_sh(fp, "set key bmargin\n");
        gnuplot_sh(fp, "set key font ',5'\n");
        gnuplot_sh(fp, "set key outside\n");
    }
    if (gp[0]->legend_enhanced) {
        gnuplot_sh(fp, "set key enhanced\n");
    } else {
        gnuplot_sh(fp, "set key noenhanced\n");
    }

    // Begin plotting
    gnuplot_sh(fp, "plot ");
    for (size_t i = 0; i < y_count; i++) {
        char pltbuf[1024] = {0};
        sprintf(pltbuf, "'-' ");
        if (gp[0]->legend_toggle) {
            sprintf(pltbuf + strlen(pltbuf), "title '%s' ", gp[i]->legend_title);
            sprintf(pltbuf + strlen(pltbuf), "with lines ");
            if (gp[i]->line_width) {
                sprintf(pltbuf + strlen(pltbuf), "lw %0.1f ", gp[i]->line_width);
            }
            if (gp[i]->line_type) {
                sprintf(pltbuf + strlen(pltbuf), "lt %s ", gp[i]->line_type);
            }
            if (gp[i]->line_color) {
                sprintf(pltbuf + strlen(pltbuf), "lc rgb '#%06x' ", gp[i]->line_color);
            }
            gnuplot_sh(fp, "%s ", pltbuf);
        } else {
            gnuplot_sh(fp, "with lines ");
        }
        if (i < y_count - 1) {
            gnuplot_sh(fp, ", ");
        }
    }
    gnuplot_sh(fp, "\n");

    // Emit MSTAT data
    for (size_t arr = 0; arr < y_count; arr++) {
        for (size_t i = 0; i < x_count; i++) {
            gnuplot_sh(fp, "%lf %lf\n", x[i], y[arr][i]);
        }
        // Commit plot and execute
        gnuplot_sh(fp, "e\n");
    }
    fflush(fp);
}

unsigned int rgb(unsigned char r, unsigned char g, unsigned char b) {
    unsigned int result = r;
    result = result << 8 | g;
    result = result << 8 | b;
    return result;
}


static void show_fields(char **fields) {
    size_t total;
    for (total = 0; fields[total] != NULL; total++);

    for (size_t i = 0, tokens = 0; i < total; i++) {
        if (tokens == 4) {
            printf("\n");
            tokens = 0;
        }
        printf("%-20s", fields[i]);
        tokens++;
    }
    printf("\n");
}

static void usage(char *prog) {
    char *sep;
    char *name;

    sep = strrchr(prog, '/');
    name = prog;
    if (sep) {
        name = sep + 1;
    }
    printf("usage: %s [OPTIONS] <FILE>\n"
           "-h        this help message\n"
           "-l        list mstat fields\n"
           "-f        fields (default: rss,pss,swap)\n"
           "-v        verbose mode\n"
           "\n", name);
}

void parse_options(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Missing path to *.mstat data\n");
        exit(1);
    }
    option.fields[0] = "rss";
    option.fields[1] = "pss";
    option.fields[2] = "swap";
    option.fields[3] = NULL;

    for (int x = 0, i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strlen(argv[i]) > 1 && !strncmp(argv[i], "-", 1)) {
            arg = argv[i] + 1;
            if (!strcmp(arg, "h")) {
                usage(argv[0]);
                exit(0);
            }
            if (!strcmp(arg, "l")) {
                show_fields(&mstat_field_names[MSTAT_FIELD_RSS]);
                exit(0);
            }
            if (!strcmp(arg, "v")) {
                option.verbose = 1;
            }
            if (!strcmp(arg, "f")) {
                char *val = argv[i+1];
                char *token = NULL;
                if (!val) {
                    fprintf(stderr, "%s requires an argument\n", argv[i]);
                    exit(1);
                }
                if (!strcmp(val, "all")) {
                    for (x = MSTAT_FIELD_RSS; mstat_field_names[x] != NULL; x++) {
                        option.fields[x] = strdup(mstat_field_names[x]);
                    }
                    option.fields[x] = NULL;
                } else {
                    while ((token = strsep(&val, ",")) != NULL) {
                        option.fields[x] = token;
                        x++;
                    }
                }
                i++;
            }
        } else {
            strcpy(option.filename, argv[i]);
        }
    }
}

int main(int argc, char *argv[]) {
    struct mstat_record_t p;
    char **stored_fields;
    char **field;
    int data_total;
    double **axis_y;
    double *axis_x;
    double mem_min, mem_max;
    size_t rec;
    FILE *fp;

    parse_options(argc, argv);

    rec = 0;
    data_total = 0;
    mem_min = 0.0;
    mem_max = 0.0;
    field = option.fields;
    stored_fields = NULL;
    axis_x = NULL;
    axis_y = NULL;
    fp = NULL;

    if (access(option.filename, F_OK) < 0) {
        perror(option.filename);
        exit(1);
    }

    fp = mstat_open(option.filename);
    if (!fp) {
        perror(option.filename);
        exit(1);
    }

    // Get total number of user-requested fields
    for (data_total = 0; field[data_total] != NULL; data_total++);

    // Retrieve fields from MSTAT header
    stored_fields = mstat_read_fields(fp);
    for (size_t i = 0; field[i] != NULL; i++) {
        if (mstat_is_valid_field(stored_fields, field[i])) {
            fprintf(stderr, "Invalid field: '%s'\n", field[i]);
            printf("requested field must be one or more of...\n");
            show_fields(stored_fields);
            exit(1);
        }
    }

    // We don't store the number of records in the MSTAT data. Count them here.
    while (!mstat_iter(fp, &p))
        rec++;

    axis_x = calloc(rec, sizeof(axis_x));
    if (!axis_x) {
        perror("Unable to allocate enough memory for axis_x array");
        exit(1);
    }

    axis_y = calloc(data_total + 1, sizeof(**axis_y));
    if (!axis_y) {
        perror("Unable to allocate enough memory for axis_y array");
        exit(1);
    }

    for (int i = 0, n = 0; field[i] != NULL; i++) {
        axis_y[i] = calloc(rec, sizeof(*axis_y[0]));
        n++;
    }

    mstat_rewind(fp);
    printf("Reading: %s\n", argv[1]);

    // Assign requested MSTAT data to y-axis. x-axis will always be time elapsed.
    rec = 0;
    while (!mstat_iter(fp, &p)) {
        axis_x[rec] = mstat_get_field_by_name(&p, "timestamp").d64 / 3600;
        for (int i = 0; i < data_total; i++) {
            axis_y[i][rec] = (double) mstat_get_field_by_name(&p, field[i]).u64 / 1024;
        }
        rec++;
    }

    if (!rec) {
        fprintf(stderr, "MSTAT axis_y file does not have any records\n");
        exit(1);
    } else {
        printf("Records: %zu\n", rec);
    }

    // Show min/max
    for (size_t i = 0; axis_y[i] != NULL && i < data_total; i++) {
        mstat_get_mmax(axis_y[i], rec, &mem_min, &mem_max);
        printf("%s min(%.2lf) max(%.2lf)\n", field[i], mem_min, mem_max);
    }

    if (find_program("gnuplot")) {
        fprintf(stderr, "To render plots please install gnuplot\n");
        exit(1);
    }

    struct GNUPLOT_PLOT **gp = calloc(data_total, sizeof(**gp));
    if (!gp) {
        perror("Unable to allocate memory for gnuplot configuration array");
        exit(1);
    }

    for (size_t n = 0; n < data_total; n++) {
        gp[n] = calloc(1, sizeof(*gp[0]));
        if (!gp[n]) {
            perror("Unable to allocate memory for GNUPLOT_PLOT structure");
            exit(1);
        }
    }

    unsigned char r, g, b;
    char title[255] = {0};
    r = 0x10;
    g = 0x20;
    b = 0x30;
    snprintf(title, sizeof(title) - 1, "Memory Usage (PID %d)", p.pid);

    gp[0]->xlabel = strdup("Time (HR)");
    gp[0]->ylabel = strdup("MB");
    gp[0]->title = strdup(title);
    gp[0]->grid_toggle = 1;
    gp[0]->autoscale_toggle = 1;
    gp[0]->legend_toggle = 1;

    for (size_t i = 0; i < data_total; i++) {
        gp[i]->legend_title = strdup(field[i]);
        gp[i]->line_color = rgb(r, g, b);
        gp[i]->line_width = 0.50;
        r -= 0x30;
        b += 0x20;
        g *= 3;
    }

    printf("Generating plot... ");
    fflush(stdout);

    FILE *plt;
    plt = gnuplot_open();
    if (!plt) {
        fprintf(stderr, "Failed to open gnuplot stream\n");
        exit(1);
    }
    gnuplot_plot(plt, gp, axis_x, axis_y, rec, data_total);
    gnuplot_close(plt);
    printf("done!\n");

    free(axis_x);
    for (size_t i = 0; i < data_total; i++) {
        free(axis_y[i]);
        free(gp[i]);
    }

    return 0;
}
