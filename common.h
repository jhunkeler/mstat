#ifndef MSTAT_COMMON_H
#define MSTAT_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

#define MSTAT_MAGIC "MSTAT"
#define MSTAT_FIELD_COUNT 0x08
#define MSTAT_EOH 0x0C
#define MSTAT_MAGIC_SIZE 0x10

struct mstat_record_t {
    pid_t pid;
    double timestamp;
    size_t rss,
            pss,
            pss_anon,
            pss_file,
            pss_shmem,
            shared_clean,
            shared_dirty,
            private_clean,
            private_dirty,
            referenced,
            anonymous,
            lazy_free,
            anon_huge_pages,
            shmem_pmd_mapped,
            file_pmd_mapped,
            shared_hugetlb,
            private_hugetlb,
            swap,
            swap_pss,
            locked;
};

enum {
    MSTAT_FIELD_PID = 0,
    MSTAT_FIELD_TIMESTAMP,
    MSTAT_FIELD_RSS,
    MSTAT_FIELD_PSS,
    MSTAT_FIELD_PSS_ANON,
    MSTAT_FIELD_PSS_FILE,
    MSTAT_FIELD_PSS_SHMEM,
    MSTAT_FIELD_SHARED_CLEAN,
    MSTAT_FIELD_SHARED_DIRTY,
    MSTAT_FIELD_PRIVATE_CLEAN,
    MSTAT_FIELD_PRIVATE_DIRTY,
    MSTAT_FIELD_REFERENCED,
    MSTAT_FIELD_ANONYMOUS,
    MSTAT_FIELD_LAZY_FREE,
    MSTAT_FIELD_ANON_HUGE_PAGES,
    MSTAT_FIELD_SHMEM_PMD_MAPPED,
    MSTAT_FIELD_FILE_PMD_MAPPED,
    MSTAT_FIELD_SHARED_HUGETLB,
    MSTAT_FIELD_PRIVATE_HUGETLB,
    MSTAT_FIELD_SWAP,
    MSTAT_FIELD_SWAP_PSS,
    MSTAT_FIELD_LOCKED,
};

union mstat_field_t {
    size_t u64;
    double d64;
};

int mstat_get_field_count(FILE *fp);
char **mstat_read_fields(FILE *fp);
int mstat_is_valid_field(char **fields, const char *name);
union mstat_field_t mstat_get_field_by_id(const struct mstat_record_t *record, unsigned id);
union mstat_field_t mstat_get_field_by_name(const struct mstat_record_t *p, const char *name);
int mstat_check_header(FILE *fp);
FILE *mstat_open(const char *filename);
int mstat_rewind(FILE *fp);
ssize_t mstat_get_value_smaps(char *data);
char *mstat_get_key_smaps(char *data, const char *key);
int mstat_read_smaps(struct mstat_record_t *p, FILE *fp);
int mstat_attach(struct mstat_record_t *p, pid_t pid);
int mstat_write_header(FILE *fp);
int mstat_write(FILE *fp, struct mstat_record_t *p);
int mstat_iter(FILE *fp, struct mstat_record_t *p);
void mstat_get_mmax(const double a[], size_t size, double *min, double *max);
double mstat_difftimespec(struct timespec end, struct timespec start);
int mstat_find_program(const char *name, char *where);

#endif //MSTAT_COMMON_H
