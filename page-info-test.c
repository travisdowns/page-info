/*
 * page-info-test.c
 *
 * A basic test for page-info that allocates increasing amounts of memory with malloc() and then tries
 * three types of madvise calls: no call, MADV_HUGEPAGE and MADV_NOHUGEPAGE and gets page info on the result.
 *
 * Not really a "test" at all since we don't validate the results but you can eyeball the results and make
 * sure they look sane. Note that you dont't really expect the MADV flags to always work, since we are often
 * calling this on memory that has already been paged in (returned by malloc), and we are often calling it
 * on regions smaller than 2MB where we don't expect it to work anyways.
 */
#include "page-info.h"

#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <err.h>
#include <sys/mman.h>
#include <stdlib.h>

static inline void *pagedown(void *p, unsigned psize) {
    return (void *)(((uintptr_t)p) & -(uintptr_t)psize);
}

int getenv_int(const char *var, int def) {
    const char *val = getenv(var);
    return val ? atoi(val) : def;
}

typedef struct {
    char const *name;
    int flag;
} advice_s;

#define ADVICE(flag) { #flag, flag }

const advice_s advices[] = { ADVICE(MADV_HUGEPAGE), ADVICE(MADV_NORMAL), ADVICE(MADV_NOHUGEPAGE), {} };

void print_for_range(void *start, void *end) {
    page_info_array infos = get_info_for_range(start, end);
    fprint_ratios_noheader(stdout, infos);
    free_info_array(infos);
}

void do_full_table() {
    int psize = getpagesize();

    printf("%44s", "");
    fprint_info_header(stdout);

    size_t max_kib = getenv_int("MAX_KIB", 1024 * 1024);

    for (size_t kib = 256; kib <= max_kib; kib *=2) {
        for (const advice_s *advice = advices; advice->name; advice++) {
            size_t size = kib * 1024;
            char *b = malloc(size);
            if (advice->flag != MADV_NORMAL) {
                char *ba = pagedown(b, psize);
                if (madvise(ba, size, advice->flag)) {
                    err(EXIT_FAILURE, "madvise(%s) failed", advice->name);
                }
            }
            if (!b) {
                // this probably wont' happen because with overcommit the malloc itself generally succeeds,
                // but you may get killed later when mapping in pages
                err(EXIT_FAILURE, "Allocating %zu bytes failed, exiting...", size);
            }
            printf("%16s %7.2f MiB BEFORE memset: ", advice->name, (double)kib / 1024);
            print_for_range(b, b + size);
            memset(b, 0x42, size);
            printf("%16s %7.2f MiB AFTER  memset: ", advice->name, (double)kib / 1024);
            print_for_range(b, b + size);
            free(b);
        }
        printf("\n");
    }
}

#define W "7"

// printing the
void do_one_flag_ratio(char const * name) {

    int flag = flag_from_name(name);
    if (flag < 0) {
        errx(EXIT_FAILURE, "Couldn't find flag with name '%s'", name);
    }

    printf("       size memset %10s %" W "s %" W "s %" W "s\n", "FLAG", "SET", "UNSET", "UNAVAIL");

    for (size_t kib = 256; kib <= 1024 * 1024; kib *=2) {
        size_t size = kib * 1024;
        char *b = malloc(size);
        if (!b) {
            // this probably wont' happen because with overcommit the malloc itself generally succeeds,
            // but you may get killed later when mapping in pages
            err(EXIT_FAILURE, "Allocating %zu bytes failed, exiting...", size);
        }

        flag_count count;
        count = get_flag_count(get_info_for_range(b, b + size), flag);
        printf("%7.2f MiB BEFORE %10s %"W"zu %"W"zu %"W"zu\n", (double)kib / 1024, name,
                count.pages_set, count.pages_available - count.pages_set, count.pages_total - count.pages_available);
        memset(b, 0x42, size);
        count = get_flag_count(get_info_for_range(b, b + size), flag);
        printf("%7.2f MiB AFTER  %10s %"W"zu %"W"zu %"W"zu\n", (double)kib / 1024, name,
                count.pages_set, count.pages_available - count.pages_set, count.pages_total - count.pages_available);
    }
}

int main(int argc, char** argv) {

    int psize = getpagesize();

    printf("PAGE_SIZE = %d, PID = %ld\n", psize, (long)getpid());

    if (argc == 1) {
        do_full_table();
    } else if (argc == 2) {
        do_one_flag_ratio(argv[1]);
    } else {
        fprintf(stderr, "Usage: page-info-test [flag]\n");
        return EXIT_FAILURE;
    }

    printf("DONE\n");
}


