# MSTAT

Record the memory usage of a process over time.

# How to use MSTAT

```text
usage: mstat [OPTIONS] [-p PID] | {PROGRAM... ARGS}
-c        clobber 'PID#.mstat' if it exists
-h        this help message
-o DIR    path to output directory (must exist)
-p PID    process id to monitor
-s RATE   samples per second (default: 1.00)
-v        increased verbosity
```

## Monitor an existing process

```shell
$ mstat -p 12345
PID: 12345
Samples per second: 1.00
(interrupt with ctrl-c...)
```

## Monitor a new process

```shell
$ mstat -s 10000 free
PID: 12345 
Samples per second: 10000.00
(interrupt with ctrl-c...)
               total        used        free      shared  buff/cache   available
Mem:        28455904     8951836    10728332      476032     8775736    18596172
Swap:       16777212           0    16777212
pid 148257 returned 0
data written: 12345.mstat
```

## Plotting

Requires `gnuplot` to be installed.
- Debian / Ubuntu
  - `apt update && apt install gnuplot`
- Fedora / RHEL
  - `yum install gnuplot`
- Arch Linux
  - `pacman -S gnuplot`


```shell
$ mstat_plot 12345.mstat
```

## CSV export

```shell
$ mstat_export 12345.mstat > 12345.csv
```

