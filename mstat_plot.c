#include "common.h"
#include "gnuplot.h"

extern char *mstat_field_names[];

static struct Option {
    unsigned char verbose;
    char *fields[0xffff];
    char filename[PATH_MAX];
} option;

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
    printf("usage: %s [OPTIONS] {FILE}\n"
           "  -f NAME[,...]   mstat field(s) to plot (default: rss,pss,swap)\n"
           "  -h              this help message\n"
           "  -l              list mstat fields\n"
           "  -v              verbose mode\n"
           "", name);
}

static void parse_options(int argc, char *argv[]) {
    if (argc < 2) {
        usage(argv[0]);
        exit(1);
    }

    option.fields[0] = "rss";
    option.fields[1] = "pss";
    option.fields[2] = "swap";
    option.fields[3] = NULL;

    for (int x = 0, i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strlen(arg) > 1 && !strncmp(arg, "-", 1)) {
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
                mstat_check_argument_str(argv, arg, i);
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
    size_t data_total;
    double **axis_y;
    double *axis_x;
    double mem_min, mem_max;
    size_t rec;
    FILE *fp;

    // Initialize options
    memset(&option, 0, sizeof(option));
    parse_options(argc, argv);

    rec = 0;
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
    printf("Reading: %s\n", option.filename);

    // Assign requested MSTAT data to y-axis. x-axis will always be time elapsed.
    rec = 0;
    while (!mstat_iter(fp, &p)) {
        axis_x[rec] = mstat_get_field_by_name(&p, "timestamp").d64 / 3600;
        for (size_t i = 0; i < data_total; i++) {
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

    if (mstat_find_program("gnuplot", NULL)) {
        fprintf(stderr, "To render plots please install gnuplot\n");
        exit(1);
    }

    struct GNUPLOT_PLOT **gp = calloc(data_total + 1, sizeof(*gp[0]));
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

    char title[255] = {0};
    snprintf(title, sizeof(title) - 1, "Memory Usage (PID %d)", p.pid);

    gp[0]->xlabel = strdup("Time (HR)");
    gp[0]->ylabel = strdup("MB");
    gp[0]->title = strdup(title);
    gp[0]->grid_toggle = 1;
    gp[0]->grid_mytics = 5;
    gp[0]->grid_mxtics = 5;
    gp[0]->autoscale_toggle = 1;
    gp[0]->legend_toggle = 1;

    for (size_t i = 0; i < data_total; i++) {
        gp[i]->legend_title = strdup(field[i]);
        gp[i]->line_width = 1.0;
        gp[i]->line_color = 0;
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
    gnuplot_wait(plt);
    gnuplot_close(plt);
    printf("done!\n");

    free(axis_x);
    for (size_t i = 0; i < data_total; i++) {
        free(axis_y[i]);
        free(gp[i]);
    }
    free(axis_y);
    free(gp);
    return 0;
}
