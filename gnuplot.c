#include "gnuplot.h"
#include <string.h>
#include <stdarg.h>

/**
 * Open a new gnuplot handle
 * @return stream on success, or NULL on error
 */
FILE *gnuplot_open() {
    // -p = persistent window after exit
    //return popen("gnuplot --persist", "w");
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
 * Wait for plot window to exit
 * @param fp pointer to gnuplot stream
 * @return value of `gnuplot_sh`
 */
int gnuplot_wait(FILE *fp) {
    return gnuplot_sh(fp, "pause mouse close\n");
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
    //fprintf(stdout, fmt, args);
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
    if (gp[0]->grid_toggle) {
        gnuplot_sh(fp, "set grid\n");
        if (gp[0]->grid_mxtics || gp[0]->grid_mytics) {
            gnuplot_sh(fp, "set mytics %zu\n", gp[0]->grid_mytics);
            gnuplot_sh(fp, "set mxtics %zu\n", gp[0]->grid_mxtics);
            gnuplot_sh(fp, "set grid mxtics mytics\n");
        }
    }
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

unsigned int gnuplot_rgb(unsigned char r, unsigned char g, unsigned char b) {
    unsigned int result = r;
    result = result << 8 | g;
    result = result << 8 | b;
    return result;
}