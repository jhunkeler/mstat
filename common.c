#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"

// Globals
const char mstat_magic_bytes[] = MSTAT_MAGIC;
char *mstat_field_names[] = {
        "pid",
        "timestamp",
        "rss",
        "pss",
        "pss_anon",
        "pss_file",
        "pss_shmem",
        "shared_clean",
        "shared_dirty",
        "private_clean",
        "private_dirty",
        "referenced",
        "anonymous",
        "lazy_free",
        "anon_huge_pages",
        "shmem_pmd_mapped",
        "file_pmd_mapped",
        "shared_hugetlb",
        "private_hugetlb",
        "swap",
        "swap_pss",
        "locked",
        NULL,
};

/**
 * Get total number of fields stored in MSTAT file header
 * @param fp pointer to MSTAT file stream
 * @return
 */
int mstat_get_field_count(FILE *fp) {
    int count;
    ssize_t pos;

    count = 0;
    pos = ftell(fp);
    if (pos < 0) {
        return -1;
    }
    if (fseek(fp, MSTAT_FIELD_COUNT, SEEK_SET) < 0) {
        return -1;
    }
    if (!fread(&count, sizeof(count), 1, fp)) {
        return -1;
    }
    if (fseek(fp, pos, SEEK_SET) < 0) {
        return -1;
    }
    return count;
}

/**
 * Read fields stored in MSTAT header
 * @param fp a pointer to MSTAT file
 * @return array of MSTAT fields. NULL on error.
 */
char **mstat_read_fields(FILE *fp) {
    char **fields;
    int total = 0;
    //fseek(fp, MSTAT_FIELD_COUNT, SEEK_SET);
    //fread(&total, sizeof(total), 1, fp);
    total = mstat_get_field_count(fp);
    fseek(fp, MSTAT_MAGIC_SIZE, SEEK_SET);
    fields = calloc(total + 1, sizeof(*fields));
    if (!fields) {
        perror("Unable to allocate memory for fields");
        return NULL;
    }
    for (unsigned i = 0; i < total; i++) {
        char buf[255] = {0};
        unsigned len;
        fread(&len, sizeof(len), 1, fp);
        fread(buf, len, 1, fp);
        fields[i] = strdup(buf);
    }
    return fields;
}

/**
 * Check if `name` is present in `fields` array
 * @param fields array of field names
 * @param name field name to verify
 * @return 0 on success. 1 on error.
 */
int mstat_is_valid_field(char **fields, const char *name) {
    for (size_t i = 0; fields[i] != NULL; i++) {
        if (!strcmp(fields[i], name)) {
            return 0;
        }
    }
    return 1;
}

/**
 * Return record value by field name
 * @param p pointer to MSTAT record
 * @param name field name
 * @return MSTAT field union. ULLONG_MAX on error
 */
union mstat_field_t mstat_get_field_by_name(const struct mstat_record_t *p, const char *name) {
    union mstat_field_t result;
    result.u64 = ULLONG_MAX;

    if (!strcmp(name, "pid")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PID);
    } else if (!strcmp(name, "timestamp")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_TIMESTAMP);
    } else if (!strcmp(name, "rss")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_RSS);
    } else if (!strcmp(name, "pss")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PSS);
    } else if (!strcmp(name, "pss_anon")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PSS_ANON);
    } else if (!strcmp(name, "pss_file")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PSS_FILE);
    } else if (!strcmp(name, "pss_shmem")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PSS_SHMEM);
    } else if (!strcmp(name, "shared_clean")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_SHARED_CLEAN);
    } else if (!strcmp(name, "shared_dirty")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_SHARED_DIRTY);
    } else if (!strcmp(name, "private_clean")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PRIVATE_CLEAN);
    } else if (!strcmp(name, "private_dirty")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PRIVATE_DIRTY);
    } else if (!strcmp(name, "referenced")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_REFERENCED);
    } else if (!strcmp(name, "anonymous")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_ANONYMOUS);
    } else if (!strcmp(name, "lazy_free")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_LAZY_FREE);
    } else if (!strcmp(name, "anon_huge_pages")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_ANON_HUGE_PAGES);
    } else if (!strcmp(name, "shmem_pmd_mapped")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_SHMEM_PMD_MAPPED);
    } else if (!strcmp(name, "file_pmd_mapped")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_FILE_PMD_MAPPED);
    } else if (!strcmp(name, "shared_hugetlb")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_SHARED_HUGETLB);
    } else if (!strcmp(name, "private_hugetlb")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_PRIVATE_HUGETLB);
    } else if (!strcmp(name, "swap")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_SWAP);
    } else if (!strcmp(name, "swap_pss")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_SWAP_PSS);
    } else if (!strcmp(name, "locked")) {
        result = mstat_get_field_by_id(p, MSTAT_FIELD_LOCKED);
    }

    return result;
}

/**
 * Return record value by identifier
 * @param record pointer to MSTAT record
 * @param id MSTAT_FIELD_* constant
 * @return MSTAT field union. ULLONG_MAX on error
 */
union mstat_field_t mstat_get_field_by_id(const struct mstat_record_t *record, unsigned id) {
    union mstat_field_t result;
    result.u64 = ULLONG_MAX;

    switch (id) {
        case MSTAT_FIELD_PID:
            result.u64 = record->pid;
            break;
        case MSTAT_FIELD_TIMESTAMP:
            result.d64 = record->timestamp;
            break;
        case MSTAT_FIELD_RSS:
            result.u64 = record->rss;
            break;
        case MSTAT_FIELD_PSS:
            result.u64 = record->pss;
            break;
        case MSTAT_FIELD_PSS_ANON:
            result.u64 = record->pss_anon;
            break;
        case MSTAT_FIELD_PSS_FILE:
            result.u64 = record->pss_file;
            break;
        case MSTAT_FIELD_PSS_SHMEM:
            result.u64 = record->pss_shmem;
            break;
        case MSTAT_FIELD_SHARED_CLEAN:
            result.u64 = record->shared_clean;
            break;
        case MSTAT_FIELD_SHARED_DIRTY:
            result.u64 = record->shared_dirty;
            break;
        case MSTAT_FIELD_PRIVATE_CLEAN:
            result.u64 = record->private_clean;
            break;
        case MSTAT_FIELD_PRIVATE_DIRTY:
            result.u64 = record->private_dirty;
            break;
        case MSTAT_FIELD_REFERENCED:
            result.u64 = record->referenced;
            break;
        case MSTAT_FIELD_ANONYMOUS:
            result.u64 = record->anonymous;
            break;
        case MSTAT_FIELD_LAZY_FREE:
            result.u64 = record->lazy_free;
            break;
        case MSTAT_FIELD_ANON_HUGE_PAGES:
            result.u64 = record->anon_huge_pages;
            break;
        case MSTAT_FIELD_SHMEM_PMD_MAPPED:
            result.u64 = record->shmem_pmd_mapped;
            break;
        case MSTAT_FIELD_FILE_PMD_MAPPED:
            result.u64 = record->file_pmd_mapped;
            break;
        case MSTAT_FIELD_SHARED_HUGETLB:
            result.u64 = record->shared_hugetlb;
            break;
        case MSTAT_FIELD_PRIVATE_HUGETLB:
            result.u64 = record->private_hugetlb;
            break;
        case MSTAT_FIELD_SWAP:
            result.u64 = record->swap;
            break;
        case MSTAT_FIELD_SWAP_PSS:
            result.u64 = record->swap_pss;
            break;
        case MSTAT_FIELD_LOCKED:
            result.u64 = record->locked;
            break;
        default:
            fprintf(stderr, "%s: unknown id id: %u\n", __FUNCTION__, id);
            break;
    }

    return result;
}

int mstat_check_header(FILE *fp) {
    int result;
    char buf[MSTAT_MAGIC_SIZE] = {0};
    ssize_t pos = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    fread(buf, sizeof(buf), 1, fp);
    if (!strcmp(buf, mstat_magic_bytes)) {
        result = 0;
    } else {
        result = 1;
    }
    fseek(fp, pos, SEEK_SET);
    return result;
}

/**
 * Open an mstat file, or create one if it does not exist
 * @param filename
 * @return
 */
FILE *mstat_open(const char *filename) {
    FILE *fp = NULL;
    char mode[4] = {0};
    int do_header = 0;

    strcpy(mode, "rb+");
    if (access(filename, F_OK) < 0) {
        do_header = 1;
        strcpy(mode, "wb+");
    }

    fp = fopen(filename, mode);
    if (!fp) {
        perror(filename);
        return NULL;
    }

    if (do_header && mstat_write_header(fp)) {
        fclose(fp);
        fprintf(stderr, "unable to write header to mstat database\n");
        return NULL;
    } else {
        if (mstat_check_header(fp)) {
            fprintf(stderr, "%s is not an mstat database\n", filename);
            fclose(fp);
            return NULL;
        }
    }
    mstat_rewind(fp);
    return fp;
}

/**
 * Rewind MSTAT file to the start of the data region
 * @param fp pointer to MSTAT file stream
 * @return
 */
int mstat_rewind(FILE *fp) {
    int fields_end;
    fseek(fp, MSTAT_EOH, SEEK_SET);
    fread(&fields_end, sizeof(fields_end), 1, fp);
    return fseek(fp, fields_end, SEEK_SET);
}

/**
 * Return one record from a MSTAT file per call, until EOF
 * @param fp pointer to MSTAT file stream
 * @param record pointer to MSTAT record
 * @return 0 on success. -1 on error
 */
int mstat_iter(FILE *fp, struct mstat_record_t *record) {
    if (feof(fp))
        return -1;
    if (!fread(&record->pid, sizeof(record->pid), 1, fp)) return -1;
    if (!fread(&record->timestamp, sizeof(record->timestamp), 1, fp)) return -1;
    if (!fread(&record->rss, sizeof(record->rss), 1, fp)) return -1;
    if (!fread(&record->pss, sizeof(record->pss), 1, fp)) return -1;
    if (!fread(&record->pss_anon, sizeof(record->pss_anon), 1, fp)) return -1;
    if (!fread(&record->pss_file, sizeof(record->pss_file), 1, fp)) return -1;
    if (!fread(&record->pss_shmem, sizeof(record->pss_shmem), 1, fp)) return -1;
    if (!fread(&record->shared_clean, sizeof(record->shared_clean), 1, fp)) return -1;
    if (!fread(&record->shared_dirty, sizeof(record->shared_dirty), 1, fp)) return -1;
    if (!fread(&record->private_clean, sizeof(record->private_clean), 1, fp)) return -1;
    if (!fread(&record->private_dirty, sizeof(record->private_dirty), 1, fp)) return -1;
    if (!fread(&record->referenced, sizeof(record->referenced), 1, fp)) return -1;
    if (!fread(&record->anonymous, sizeof(record->anonymous), 1, fp)) return -1;
    if (!fread(&record->lazy_free, sizeof(record->lazy_free), 1, fp)) return -1;
    if (!fread(&record->anon_huge_pages, sizeof(record->anon_huge_pages), 1, fp)) return -1;
    if (!fread(&record->shmem_pmd_mapped, sizeof(record->shmem_pmd_mapped), 1, fp)) return -1;
    if (!fread(&record->file_pmd_mapped, sizeof(record->file_pmd_mapped), 1, fp)) return -1;
    if (!fread(&record->shared_hugetlb, sizeof(record->shared_hugetlb), 1, fp)) return -1;
    if (!fread(&record->private_hugetlb, sizeof(record->private_hugetlb), 1, fp)) return -1;
    if (!fread(&record->swap, sizeof(record->swap), 1, fp)) return -1;
    if (!fread(&record->swap_pss, sizeof(record->swap_pss), 1, fp)) return -1;
    if (!fread(&record->locked, sizeof(record->locked), 1, fp)) return -1;
    return 0;
}

/**
 * Convert smaps_rollup data string to integer
 * @param data value from smaps_rollup key pair
 * @return integer value on success. -1 on error
 */
ssize_t mstat_get_value_smaps(char *data) {
    ssize_t result = -1;
    char *ptr = NULL;

    ptr = strchr(data, ':');
    if (ptr) {
        ptr++;
        result = strtol(ptr, NULL, 10);
    }
    return result;
}

/**
 * Extract value (as string) from smaps_rollup key pair
 * @param data smaps_rollup line
 * @param key name to read
 * @return data from key on success. NULL on error
 */
char *mstat_get_key_smaps(char *data, const char *key) {
    char buf[255] = {0};
    snprintf(buf, sizeof(buf) - 1, "%s:", key);
    if (!strncmp(data, buf, strlen(buf))) {
        return data;
    }
    return NULL;
}

/**
 * Consume /proc/`pid`/smaps_rollup stream
 * @param p pointer to MSTAT record
 * @param fp pointer to file stream
 * @return TODO
 */
int mstat_read_smaps(struct mstat_record_t *p, FILE *fp) {
    char data[1024] = {0};
    for (size_t i = 0; fgets(data, sizeof(data) - 1, fp) != NULL; i++) {
        if (mstat_get_key_smaps(data, "Rss")) {
            p->rss = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Pss")) {
            p->pss = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Pss_Anon")) {
            p->pss_anon = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Pss_File")) {
            p->pss_file = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Pss_Shmem")) {
            p->pss_shmem = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Shared_Clean")) {
            p->shared_clean = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Shared_Dirty")) {
            p->shared_dirty = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Private_Clean")) {
            p->private_clean = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Private_Dirty")) {
            p->private_dirty = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Referenced")) {
            p->referenced = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Anonymous")) {
            p->anonymous = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "LazyFree")) {
            p->lazy_free = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "AnonHugePages")) {
            p->anon_huge_pages = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "ShmemPmdMapped")) {
            p->shmem_pmd_mapped = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "FilePmdMapped")) {
            p->file_pmd_mapped = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Shared_Hugetlb")) {
            p->shared_hugetlb = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Private_Hugetlb")) {
            p->private_hugetlb = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Swap")) {
            p->swap = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "SwapPss")) {
            p->swap_pss = mstat_get_value_smaps(data);
        }
        if (mstat_get_key_smaps(data, "Locked")) {
            p->locked = mstat_get_value_smaps(data);
        }
    }
}

/**
 *
 * @param p pointer to MSTAT record
 * @param pid of target process
 * @return 0 on success, -1 on error
 */
int mstat_attach(struct mstat_record_t *p, pid_t pid) {
    FILE *fp;
    char path[PATH_MAX] = {0};

    snprintf(path, PATH_MAX, "/proc/%d/smaps_rollup", pid);
    if (access(path, F_OK) < 0) {
        return -1;
    }
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    mstat_read_smaps(p, fp);
    fclose(fp);

    return 0;
}

/**
 * Write MSTAT header to data file
 *
 * HEADER FORMAT
 * 0x00 - 0x07 = file identifier (8 bytes)
 * 0x08 - 0x0B = total field records (4 bytes)
 * 0x0C - 0x0F = EOH offset (4 bytes)
 * 0x10 - EOH = field_length (unsigned int), field (string) (n... bytes)
 *
 * @param fp pointer to stream
 * @return 0 on success, -1 on error
 */
int mstat_write_header(FILE *fp) {
    fwrite(mstat_magic_bytes, 6, 1, fp);
    for (int i = 0; i < MSTAT_MAGIC_SIZE - sizeof(mstat_magic_bytes); i++) {
        if (!fwrite("\0", 1, 1, fp)) {
            return -1;
        }
    }
    int rec;
    ssize_t fields_end;

    for (rec = 0; mstat_field_names[rec] != NULL; rec++) {
        unsigned int len = strlen(mstat_field_names[rec]);
        fwrite(&len, sizeof(len), 1, fp);
        fwrite(mstat_field_names[rec], sizeof(char), len, fp);
    }
    fields_end = ftell(fp);

    fseek(fp, MSTAT_FIELD_COUNT, SEEK_SET);
    fwrite(&rec, sizeof(rec), 1, fp);

    fseek(fp, MSTAT_EOH, SEEK_SET);
    fwrite(&fields_end, sizeof(int), 1, fp);
    fseek(fp, fields_end, SEEK_SET);
    return 0;
}

/**
 * Write a MSTAT record to data file
 * @param fp pointer to MSTAT file stream
 * @param record pointer to MSTAT record
 * @return 0 on success. -1 on error
 */
int mstat_write(FILE *fp, struct mstat_record_t *record) {
    if (!fwrite(&record->pid, sizeof(record->pid), 1, fp)) return -1;
    if (!fwrite(&record->timestamp, sizeof(record->timestamp), 1, fp)) return -1;
    if (!fwrite(&record->rss, sizeof(record->rss), 1, fp)) return -1;
    if (!fwrite(&record->pss, sizeof(record->pss), 1, fp)) return -1;
    if (!fwrite(&record->pss_anon, sizeof(record->pss_anon), 1, fp)) return -1;
    if (!fwrite(&record->pss_file, sizeof(record->pss_file), 1, fp)) return -1;
    if (!fwrite(&record->pss_shmem, sizeof(record->pss_shmem), 1, fp)) return -1;
    if (!fwrite(&record->shared_clean, sizeof(record->shared_clean), 1, fp)) return -1;
    if (!fwrite(&record->shared_dirty, sizeof(record->shared_dirty), 1, fp)) return -1;
    if (!fwrite(&record->private_clean, sizeof(record->private_clean), 1, fp)) return -1;
    if (!fwrite(&record->private_dirty, sizeof(record->private_dirty), 1, fp)) return -1;
    if (!fwrite(&record->referenced, sizeof(record->referenced), 1, fp)) return -1;
    if (!fwrite(&record->anonymous, sizeof(record->anonymous), 1, fp)) return -1;
    if (!fwrite(&record->lazy_free, sizeof(record->lazy_free), 1, fp)) return -1;
    if (!fwrite(&record->anon_huge_pages, sizeof(record->anon_huge_pages), 1, fp)) return -1;
    if (!fwrite(&record->shmem_pmd_mapped, sizeof(record->shmem_pmd_mapped), 1, fp)) return -1;
    if (!fwrite(&record->file_pmd_mapped, sizeof(record->file_pmd_mapped), 1, fp)) return -1;
    if (!fwrite(&record->shared_hugetlb, sizeof(record->shared_hugetlb), 1, fp)) return -1;
    if (!fwrite(&record->private_hugetlb, sizeof(record->private_hugetlb), 1, fp)) return -1;
    if (!fwrite(&record->swap, sizeof(record->swap), 1, fp)) return -1;
    if (!fwrite(&record->swap_pss, sizeof(record->swap_pss), 1, fp)) return -1;
    if (!fwrite(&record->locked, sizeof(record->locked), 1, fp)) return -1;
    return 0;
}

/**
 * Compute difference between timespec structures
 * @param end timespec
 * @param start timespec
 * @return seconds
 */
double mstat_difftimespec(const struct timespec end, const struct timespec start) {
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}

/**
 * Compute the min/max of an array
 * @param a input data
 * @param size size of input data
 * @param min pointer to return variable (modified)
 * @param max pointer to return variable (modified)
 */
void mstat_get_mmax(const double a[], size_t size, double *min, double *max) {
    *min = a[0];
    *max = 0;

    for (size_t i = 0; i < size; i++) {
        if (a[i] > *max) {
            *max = a[i];
        }
        if (a[i] < *min) {
            *min = a[i];
        }
    }
}

/**
 * Determine if `name` is available on `PATH`
 * @param name of executable
 * @return 0 on success. >0 on error
 */
int mstat_find_program(const char *name, char *where) {
    char *path;
    char *pathtmp;
    char *path_orig;
    char *token;
    char pathtest[PATH_MAX] = {0};
    char nametmp[PATH_MAX] = {0};

    pathtmp = getenv("PATH");
    if (!pathtmp) {
        return 1;
    }
    path = strdup(pathtmp);
    path_orig = path;

    char *last_element = strrchr(name, '/');
    if (last_element) {
        strncpy(nametmp, last_element + 1, sizeof(nametmp) - 1);
    } else {
        strncpy(nametmp, name, sizeof(nametmp) - 1);
    }

    while ((token = strsep(&path, ":")) != NULL) {
        snprintf(pathtest, PATH_MAX - 1, "%s/%s", token, nametmp);
        if (access(pathtest, F_OK | X_OK) == 0) {
            if (where) {
                strcpy(where, pathtest);
            }
            free(path_orig);
            return 0;
        }
    }
    free(path);
    return 1;
}

