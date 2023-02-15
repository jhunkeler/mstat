cmake_minimum_required(VERSION 3.0)
project(mstat C)
include(GNUInstallDirs)

set(CMAKE_C_STANDARD 99)
add_executable(mstat mstat.c common.c)
add_executable(mstat_plot mstat_plot.c common.c gnuplot.c gnuplot.h)
add_executable(mstat_export mstat_export.c common.c)

install(TARGETS mstat mstat_plot mstat_export
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
