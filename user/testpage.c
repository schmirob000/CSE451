#include <inc/lib.h>

#define VA	((void *)0xa0000000)

static char buf[PGSIZE] __attribute__((aligned(PGSIZE)));

void
umain(int argc, char **argv)
{
	binaryname = "testpage";

	// sys_page_alloc error cases
	assert(sys_page_alloc(-1, VA, PTE_U|PTE_P) == -E_BAD_ENV);
	assert(sys_page_alloc(0, (void *)UTOP + PGSIZE, PTE_U|PTE_P) == -E_INVAL);
	assert(sys_page_alloc(0, VA + 1, PTE_U|PTE_P) == -E_INVAL);
	assert(sys_page_alloc(0, VA, PTE_U|PTE_P|PTE_G) == -E_INVAL);
	assert(sys_page_alloc(0, VA, 0) == -E_INVAL);

	// sys_page_map error cases
	assert(sys_page_map(-1, buf, 0, buf, PTE_U|PTE_P) == -E_BAD_ENV);
	assert(sys_page_map(0, buf, -1, buf, PTE_U|PTE_P) == -E_BAD_ENV);
	assert(sys_page_map(0, buf + 1, 0, VA, PTE_U|PTE_P) == -E_INVAL);
	assert(sys_page_map(0, (void *)UTOP + PGSIZE, 0, VA, PTE_U|PTE_P) == -E_INVAL);
	assert(sys_page_map(0, buf, 0, (void *)UTOP + PGSIZE, PTE_U|PTE_P) == -E_INVAL);
	assert(sys_page_map(0, buf, 0, buf - 1, PTE_U|PTE_P) == -E_INVAL);
	assert(sys_page_map(0, VA, 0, VA, PTE_U|PTE_P) == -E_INVAL);
	assert(sys_page_map(0, buf, 0, buf, PTE_U|PTE_P|PTE_G) == -E_INVAL);
	assert(sys_page_map(0, buf, 0, buf, 0) == -E_INVAL);

	// sys_page_unmap error cases
	assert(sys_page_unmap(-1, VA) == -E_BAD_ENV);
	assert(sys_page_unmap(0, (void *)UTOP + PGSIZE) == -E_INVAL);
	assert(sys_page_unmap(0, VA + 1) == -E_INVAL);
	assert(sys_page_unmap(0, VA) == 0);

	// map buf at VA & write to VA
	assert(!(uvpd[PDX(VA)] & PTE_P) || !(uvpt[PGNUM(VA)] & PTE_P));
	assert(sys_page_map(0, buf, 0, VA, PTE_U|PTE_P|PTE_W) == 0);
	assert((uvpd[PDX(VA)] & PTE_P) && (uvpt[PGNUM(VA)] & PTE_P));
	strcpy(VA, "hello");
	assert(strcmp(VA, "hello") == 0);
	assert(strcmp(buf, "hello") == 0);

	// swap the pages at buf & VA
	assert(sys_page_alloc(0, VA, PTE_U|PTE_P|PTE_W) == 0);
	assert(*(char *)VA == 0); // new page mapped
	strcpy(VA, "world");
	assert(strcmp(buf, "hello") == 0);
	assert(strcmp(VA, "world") == 0);
	assert(sys_page_map(0, buf, 0, UTEMP, PTE_U|PTE_P|PTE_W) == 0);
	assert(sys_page_map(0, VA, 0, buf, PTE_U|PTE_P|PTE_W) == 0);
	assert(sys_page_map(0, UTEMP, 0, VA, PTE_U|PTE_P|PTE_W) == 0);
	assert(strcmp(buf, "world") == 0);
	assert(strcmp(VA, "hello") == 0);

	cprintf("page tests passed\n");
}
