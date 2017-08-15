/* Simplest soft offline testcase */
#include <inttypes.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#define err(x) perror(x), exit(1)

#define MADV_SOFT_OFFLINE 101          /* soft offline page for testing */

#define PAGE_2MB (4096UL*512)

#define PRESENT_MASK (1UL<<63)
#define SWAPPED_MASK (1UL<<62)
#define PAGE_TYPE_MASK (1UL<<61)
#define PFN_MASK     ((1UL<<55)-1)

#define KPF_THP      (1UL<<22)

int PS;

void print_paddr_and_flags(char *bigmem, int pagemap_file, int kpageflags_file)
{
	uint64_t paddr;
	uint64_t page_flags;

	if (pagemap_file) {
		pread(pagemap_file, &paddr, sizeof(paddr), ((long)bigmem>>12)*sizeof(paddr));


		if (kpageflags_file) {
			pread(kpageflags_file, &page_flags, sizeof(page_flags), 
				(paddr & PFN_MASK)*sizeof(page_flags));

			fprintf(stderr, "vpn: 0x%lx, pfn: 0x%lx is %s %s, %s, %s\n",
				((long)bigmem)>>12,
				(paddr & PFN_MASK),
				paddr & PAGE_TYPE_MASK ? "file-page" : "anon",
				paddr & PRESENT_MASK ? "there": "not there",
				paddr & SWAPPED_MASK ? "swapped": "not swapped",
				page_flags & KPF_THP ? "thp" : "not thp"
				/*page_flags*/
				);

		}
	}
}

int main(void)
{
	const char *pagemap_template = "/proc/%d/pagemap";
	const char *kpageflags_proc = "/proc/kpageflags";
	PS = PAGE_2MB * 2;
	char pagemap_proc[255];
	int pagemap_fd;
	int kpageflags_fd;
	char *map = aligned_alloc(PAGE_2MB, PS);

	if (map == (char *)-1L)
		err("mmap");

	sprintf(pagemap_proc, pagemap_template, getpid());
	pagemap_fd = open(pagemap_proc, O_RDONLY);

	if (pagemap_fd == -1)
	{
		perror("read pagemap:");
		exit(-1);
	}

	kpageflags_fd = open(kpageflags_proc, O_RDONLY);

	if (kpageflags_fd == -1)
	{
		perror("read kpageflags:");
		exit(-1);
	}

	if (madvise(map, PS, MADV_HUGEPAGE) < 0)
		perror("madvise HUGEPAGE");

	*map = 1;
	*(map+PAGE_2MB) = 1;

	print_paddr_and_flags(map, pagemap_fd, kpageflags_fd);
	print_paddr_and_flags(map + PAGE_2MB, pagemap_fd, kpageflags_fd);

	if (madvise(map + 4096, PS - 4096, MADV_SOFT_OFFLINE) < 0) 
		perror("madvise SOFT_OFFLINE");
	
	*map = 2;
	*(map+PAGE_2MB) = 2;
	print_paddr_and_flags(map, pagemap_fd, kpageflags_fd);
	print_paddr_and_flags(map + PAGE_2MB, pagemap_fd, kpageflags_fd);

	return 0;
}
