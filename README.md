# MSTAT

Record the memory usage of a process over time.

# How to use MSTAT

```shell
mstat <pid_here>
```

## Plotting

Requires `gnuplot` to be installed

```shell
mstat_plot <pid_here>.mstat
```

## CSV export

```shell
mstat_export <pid_here>.mstat > data.csv
```

