#ifndef MSTAT_GNUPLOT_H
#define MSTAT_GNUPLOT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct GNUPLOT_PLOT {
    char *title;
    char *xlabel;
    char *ylabel;
    char *line_type;
    double line_width;
    unsigned int line_color;
    unsigned char grid_toggle;
    unsigned int grid_mxtics;
    unsigned int grid_mytics;
    unsigned char autoscale_toggle;
    unsigned char legend_toggle;
    unsigned char legend_enhanced;
    char *legend_title;
};

FILE *gnuplot_open();
int gnuplot_close(FILE *fp);
int gnuplot_wait(FILE *fp);
int gnuplot_sh(FILE *fp, char *fmt, ...);
void gnuplot_plot(FILE *fp, struct GNUPLOT_PLOT **gp, double x[], double *y[], size_t x_count, size_t y_count);
unsigned int gnuplot_rgb(unsigned char r, unsigned char g, unsigned char b);

#endif //MSTAT_GNUPLOT_H
