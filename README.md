# page-info

A small utility library to return information about the memory pages backing a given region. For example, you can answer questions like:

 - How many of these pages are physically present in RAM?
 - What fraction of this allocation is backed by huge pages?
 - Have any pages in this range been swapped out to the swapfile?

Basically this parses the `/proc/$PID/pagemap` file for the current process, which returns basic information, and then if possible it looks up more interesting flags on a per-page basis in `/proc/kpagemap`. The available flags are documented [here](https://www.kernel.org/doc/Documentation/vm/pagemap.txt) and more briefly on the [proc manpage](http://man7.org/linux/man-pages/man5/proc.5.html).

## Example

As a simple example, here's a snippet which prints to stdout the percentage of pages that have been allocated with huge pages.

    char *array = malloc(size);
    memset(array, 1, size); // commit the pages

    page_info_array pinfo = get_info_for_range(array, array + size);
    flag_count thp_count = get_flag_count(pinfo, KPF_THP);
    if (thp_count.pages_available) {
    printf("Source pages allocated with transparent hugepages: %4.1f%% (%lu total pages, %4.1f%% flagged)\n",
        100.0 * thp_count.pages_set / thp_count.pages_total,
        thp_count.pages_total,
        100.0 * thp_count.pages_available / thp_count.pages_total);
    } else {
        printf("Couldn't determine hugepage info (you are probably not running as root)\n");
    }

A slightly more fleshed out version of example is available as a standalone program as [malloc-demo](malloc-demo.c). On my system it reports (this depends heavily on the value in `/sys/kernel/mm/transparent_hugepage/enabled`):

```
Allocating an array of size 7168 KiB using malloc
Source pages allocated with transparent hugepages: 85.7% (1793 total pages, 100.0% flagged)
```

## Permissions

Unfortunately (from the perspective of those wanting to use this library to its maximum capability), most of the juicy infomation about backing pages lives in the `/proc/kpagemap` file and this file is only accessible as root. You can still use this utility as a regular user, but only a handful of flags that are encoded directly in `/proc/pagemap` are available. They are those directly named in the `page_info` structure in `page-info.h`:

```
    /* soft-dirty set */
    bool softdirty;
    /* exclusively mapped, see e.g., https://patchwork.kernel.org/patch/6787921/ */
    bool exclusive;
    /* is a file mapping */
    bool file;
    /* page is swapped out */
    bool swapped;
    /* page is present, i.e, a physical page is allocated */
    bool present;
```

So you can determine if a page is present, swapped out, its soft-dirty status, whether it is exclusive and whether it is a file mapping, but not much more. On older kernels, you can also get the _physical frame number_ (the `pfn`) field, which is essentially the physical address of the page (shifted right by 12).

So if you want the full info about a mapped region, you have to run this as root.

## Building

Just run `make` which builds the `page-info-test` binary.

## Running the test

You can run the `page-info-test` binary to see the information obtained by getting page info on a series of allocations via `malloc`, starting at 256 KiB and running through 4 GiB. Information is presented both before and after touching each page in the allocation via `memset`. The difference is that for larger allocation sizes, most pages in the allocation are not present until you touch them, so limited information is available (indeed, there are no pages backing them, so questions about the nature of the backing pages have no answer). 

Here's a portion of the output on my system:

```
                                                     PFN  sdirty   excl   file swappd presnt LOCK ACTI SLAB BUDD MMAP ANON SWAP SWAP COMP COMP HUGE UNEV HWPO NOPA  KSM  THP BALL ZERO IDLE 

   MADV_HUGEPAGE    2.00 MiB BEFORE memset:   ----------  1.0000 0.0019 0.0000 0.0000 0.0019 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
   MADV_HUGEPAGE    2.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
     MADV_NORMAL    2.00 MiB BEFORE memset:   ----------  1.0000 0.5029 0.0000 0.0000 0.5029 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
     MADV_NORMAL    2.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
 MADV_NOHUGEPAGE    2.00 MiB BEFORE memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
 MADV_NOHUGEPAGE    2.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00

   MADV_HUGEPAGE    4.00 MiB BEFORE memset:   ----------  1.0000 0.0010 0.0000 0.0000 0.0010 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
   MADV_HUGEPAGE    4.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.50 0.00 0.00 1.00 1.00 0.00 0.50 0.00 0.50 0.00 0.00 0.00 0.00 0.00 0.50 0.00 0.00 0.00
     MADV_NORMAL    4.00 MiB BEFORE memset:   ----------  1.0000 0.5015 0.0000 0.0000 0.5015 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
     MADV_NORMAL    4.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
 MADV_NOHUGEPAGE    4.00 MiB BEFORE memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
 MADV_NOHUGEPAGE    4.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00

   MADV_HUGEPAGE    8.00 MiB BEFORE memset:   ----------  1.0000 0.0005 0.0000 0.0000 0.0005 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
   MADV_HUGEPAGE    8.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00
     MADV_NORMAL    8.00 MiB BEFORE memset:   ----------  1.0000 0.0010 0.0000 0.0000 0.0010 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
     MADV_NORMAL    8.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00
 MADV_NOHUGEPAGE    8.00 MiB BEFORE memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00
 MADV_NOHUGEPAGE    8.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00

   MADV_HUGEPAGE   16.00 MiB BEFORE memset:   ----------  1.0000 0.0002 0.0000 0.0000 0.0002 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
   MADV_HUGEPAGE   16.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.13 0.00 0.00 1.00 1.00 0.00 0.13 0.00 0.87 0.00 0.00 0.00 0.00 0.00 0.87 0.00 0.00 0.00
     MADV_NORMAL   16.00 MiB BEFORE memset:   ----------  1.0000 0.5004 0.0000 0.0000 0.5004 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00
     MADV_NORMAL   16.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00
 MADV_NOHUGEPAGE   16.00 MiB BEFORE memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00
 MADV_NOHUGEPAGE   16.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.25 0.00 0.00 1.00 1.00 0.00 0.25 0.00 0.75 0.00 0.00 0.00 0.00 0.00 0.75 0.00 0.00 0.00

   MADV_HUGEPAGE   32.00 MiB BEFORE memset:   ----------  1.0000 0.0001 0.0000 0.0000 0.0001 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
   MADV_HUGEPAGE   32.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.06 0.00 0.00 1.00 1.00 0.00 0.06 0.00 0.94 0.00 0.00 0.00 0.00 0.00 0.94 0.00 0.00 0.00
     MADV_NORMAL   32.00 MiB BEFORE memset:   ----------  1.0000 0.0001 0.0000 0.0000 0.0001 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
     MADV_NORMAL   32.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 0.06 0.00 0.00 1.00 1.00 0.00 0.06 0.00 0.94 0.00 0.00 0.00 0.00 0.00 0.94 0.00 0.00 0.00
 MADV_NOHUGEPAGE   32.00 MiB BEFORE memset:   ----------  1.0000 0.0001 0.0000 0.0000 0.0001 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
 MADV_NOHUGEPAGE   32.00 MiB AFTER  memset:   ----------  1.0000 1.0000 0.0000 0.0000 1.0000 0.00 1.00 0.00 0.00 1.00 1.00 0.00 1.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00
```
You can see, for example, by looking at the `presnt` column, what fraction of pages are "present in RAM" - as the allocations become larger, only a small fraction (usually the first page) of an allocation is present after allocation, but all are present following the `memset`. You can also look at the `THP` column to see that some fraction of the larger allocatoins are usually backed by huge pages, depending on the value of the `madvise()` call.

There are many other columns which have more or less interesting information depending on your scenario. The first few columns in lowercase (`sdirty   excl   file swappd presnt`) are available without special permissions since they come from `/proc/$PID/pagemap`, but the following uppercase columns require `/proc/kpageflags` access and so are generally only available to processes running as root (more precisely, those with the `CAP_SYS_ADMIN` priviledge).

## Using it in your project.

Just copy `page-info.c` and `page-info.h` into your project and include `page-info.h` in any file where you want to access the exposed methods.
