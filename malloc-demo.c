#include "page-info.h"
#include "linux/kernel-page-flags.h"

#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_KIB (7 * 1024)  // 7 MiB
int main(int argc, char **argv) {
    size_t size = 0;
    if (argc >= 2) {
        size = atoi(argv[1]) * 1024;
    }
    if (!size) {
        size = DEFAULT_KIB * 1024;
    }

    printf("Allocating an array of size %zu KiB using malloc\n", size / 1024);
    char *array = malloc(size);
    if (!array) {
        err(EXIT_FAILURE, "malloc failed");
    }
    memset(array, 1, size); // commit the pages

    page_info_array pinfo = get_info_for_range(array, array + size);
    flag_count thp_count = get_flag_count(pinfo, KPF_THP);

    free(array);

    if (thp_count.pages_available) {
    printf("Source pages allocated with transparent hugepages: %4.1f%% (%lu total pages, %4.1f%% flagged)\n",
        100.0 * thp_count.pages_set / thp_count.pages_total,
        thp_count.pages_total,
        100.0 * thp_count.pages_available / thp_count.pages_total);
    } else {
        printf("Couldn't determine hugepage info (you are probably not running as root)\n");
    }
    free_info_array(pinfo);
}
