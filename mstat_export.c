#include "common.h"

int main(int argc, char *argv[]) {
    FILE *fp;
    struct mstat_record_t p;
    char **fields;
    size_t fields_total;

    if (argc < 2) {
        fprintf(stderr, "Missing path to *.mstat data file\n");
        exit(1);
    }

    if (access(argv[1], F_OK)) {
        perror(argv[1]);
        exit(1);
    }

    fp = mstat_open(argv[1]);
    if (!fp) {
        perror(argv[1]);
        exit(1);
    }

    fields = mstat_read_fields(fp);
    if (!fields) {
        fprintf(stderr, "Unable to obtain field names from %s\n", argv[1]);
        exit(1);
    }

    fields_total = mstat_get_field_count(fp);
    for (size_t i = 0; i < fields_total; i++) {
        printf("%s", fields[i]);
        if (i < fields_total - 1) {
            printf(",");
        }
    }
    puts("");

    if (mstat_rewind(fp) < 0) {
        perror("Unable to rewind");
        exit(1);
    }

    while (!mstat_iter(fp, &p)) {
        char buf[1024] = {0};
        for (size_t i = 0; i < fields_total; i++) {
            struct mstat_record_t *pptr = &p;
            union mstat_field_t result;
            result = mstat_get_field_by_name(pptr, fields[i]);

            if (!strcmp(fields[i], "timestamp")) {
                snprintf(buf, sizeof(buf) - 1, "%lf", result.d64);
            } else {
                snprintf(buf, sizeof(buf) - 1, "%zu", result.u64);
            }
            if (i < fields_total - 1) {
                strcat(buf, ",");
            }
            printf("%s", buf);
        }
        puts("");
    }

    fclose(fp);
    return 0;
}