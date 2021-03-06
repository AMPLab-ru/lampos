#include "defs.h"
#include "kernel.h"
#include "pgalloc.h"
#include "sort.h"

#include "mb_parce.h"


#define MB_MEM_FLAG	(1 << 0)
#define MB_BD_FLAG	(1 << 1)
#define MB_CMD_FLAG	(1 << 2)
#define MB_MODS_FLAG	(1 << 3)
#define MB_SYM_1_FLAG	(1 << 4)
#define MB_SYM_2_FLAG	(1 << 5)
#define MB_MMAP_FLAG	(1 << 6)
#define MB_DRIVES_FLAG	(1 << 7)
#define MB_CT_FLAG	(1 << 8)
#define MB_BOOT_FLAG	(1 << 9)
#define MB_APM_FLAG	(1 << 10)
#define MB_VBE_FLAG	(1 << 11)


int
area_cmp(const struct mm_area **a, const struct mm_area **b)
{
	if ((*a)->beg < (*b)->beg)
		return -1;
	if ((*a)->beg > (*b)->beg)
		return 1;
	return 0;
}

void
area_parse(struct mb_mmap *mmap, size_t mmaplen,
				struct mm_area **buf, unsigned int *buflen)
{
	unsigned int i, j, prev;

	j = 0;

	while (mmaplen >= 24 && j < *buflen) {
		if (mmap->type == 1) {
			buf[j]->beg = mmap->base_addr;
			buf[j++]->end = mmap->base_addr + mmap->length;

		}

		mmaplen -= mmap->size;
		mmap = (struct mb_mmap *)((unsigned char *)mmap + mmap->size + 4);
		mmaplen -= 4;

	}
/*
	//TEST
	j = 6;

	buf[0]->beg = 3, buf[0]->end = 5;
	buf[1]->beg = 0, buf[1]->end = 2;
	buf[2]->beg = 1, buf[2]->end = 3;
	buf[3]->beg = 0, buf[3]->end = 2;
	buf[4]->beg = 10, buf[4]->end = 15;
	buf[5]->beg = 11, buf[5]->end = 12;
*/
	*buflen = j;

	sort(buf, *buflen, sizeof(void *), (void *)area_cmp);

	for (i = 0; i < *buflen; i++)
		dprintf("\tbeg = 0x%016llx\tend = 0x%016llx\n", buf[i]->beg, buf[i]->end);

	dprintf("\n");

	for (prev = 0, i = 1; i < *buflen;) {
		if (buf[i]->beg <= buf[prev]->end) {
			*buflen -= 1;

			if (buf[i]->end > buf[prev]->end)
				buf[prev]->end = buf[i]->end;

			for (j = i; j < *buflen; j++)
				SWAP(buf[j], buf[j + 1]);

		}
		else prev++, i++;

	}


	for (i = 0; i < *buflen; i++) {
		if (buf[i]->beg % 0x1000)
			buf[i]->beg += 0x1000 - buf[i]->beg % 0x1000;

		buf[i]->end -= buf[i]->end % 0x1000;

		dprintf("\tbeg = 0x%016llx\tend = 0x%016llx\n", buf[i]->beg, buf[i]->end);
	}
}

void
mb_parse(struct mb_info *mb, struct mm_area ***mm, int *mmlen)
{
	unsigned int i;

	assert_or_panic(mb, "Invalid pointer to multiboot structure");
	assert_or_panic(mm, "Invalid pointer to memory map structure");

	dprintf("\n\t");

	for (i = 11; i != 0; i--)
		dprintf("%d", GET_BIT(mb->flags, i));

	dprintf("\n");

	if (mb->flags & MB_MEM_FLAG) {
		dprintf("\tlower bound memory %dK\n", mb->mem_lower);
		dprintf("\tupper bound memory %dK\n", mb->mem_upper);

	}
	if (mb->flags & MB_BD_FLAG) {
		if (mb->boot_device[3] == 0x80)
			dprintf("\tBios at hard drive\n");
		else
			dprintf("\tBios at floppy\n");

		for (i = 0; i < 3; i++)
			if (mb->boot_device[2 - i] == 0xff)
				dprintf("\tUnused partitioning %d\n", i);
			else
				dprintf("\tUsed partitioning %d\n", i);

	}
	if (mb->flags & MB_CMD_FLAG)
		dprintf("\tcommand line: \"%s\"\n", mb->cmdline + KERNEL_BASE);
	if (mb->flags & MB_MODS_FLAG) {
		dprintf("\t%d modifications loaded\n", mb->mods_count);
		dprintf("\tFirst modification address 0x%x\n", mb->mods_addr);

	}
	if (mb->flags & MB_SYM_1_FLAG) {
		dprintf("\tsymbols 1 tabsize %d\n", mb->symbols.a.tabsize);
		dprintf("\tsymbols 1 strsize %d\n", mb->symbols.a.strsize);
		dprintf("\tsymbols 1 address 0x%x\n", mb->symbols.a.addr);

	}
	else if (mb->flags & MB_SYM_2_FLAG) {
		dprintf("\tsymbols 2 num %d\n", mb->symbols.b.num);
		dprintf("\tsymbols 2 size %d\n", mb->symbols.b.size);
		dprintf("\tsymbols 2 addr %d\n", mb->symbols.b.addr);
		dprintf("\tsymbols 2 shndx %d\n", mb->symbols.b.shndx);

	}
	if (mb->flags & MB_MMAP_FLAG) {
		unsigned int buflen = 16;
		struct mm_area arr[buflen], *buf[buflen];

		for (i = 0; i < buflen; i++)
			buf[i] = arr + i;

		area_parse(mb->mmap_addr + KERNEL_BASE, mb->mmap_length, buf, &buflen);

		*mm = buf;
		*mmlen = buflen;

	}
	/*
	if (mb->flags & MB__FLAG) {
	}*/
}

