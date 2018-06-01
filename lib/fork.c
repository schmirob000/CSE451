// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	if (!utf) cprintf("goddammit");
  void *addr = (void *) utf->utf_fault_va;
  //cprintf("%p\n", addr);
	uint32_t err = utf->utf_err;
  //cprintf("%d\n", err);
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err & FEC_WR)) {
    panic("non-write page fault in user code, Error: 0x%x, Addr: 0x%x", err, addr);
  }

  pte_t pte = PGOFF(uvpt[(uint32_t) addr/PGSIZE]);

  //if (!(pte & PTE_COW))
    //panic("write page fault in user not COW");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
  envid_t envid = sys_getenvid();
  void *addr_aligned = (void*) ROUNDDOWN((uint32_t)addr, PGSIZE);
  sys_page_alloc(envid, PFTEMP, PTE_P | PTE_U | PTE_W);
  memmove(PFTEMP, addr_aligned, PGSIZE);
  sys_page_map(envid, PFTEMP, envid, addr_aligned, PTE_P | PTE_U | PTE_W);
  sys_page_unmap(envid, PFTEMP);
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	// LAB 4: Your code here.
	int err;
	err = sys_page_map(sys_getenvid(), (void*) (pn*PGSIZE), envid, (void*) (pn*PGSIZE), PTE_P | PTE_U | PTE_COW); //map in child
  if (err < 0)
    panic("sys_page_map error while trying to duppage, %e", err);

	err = sys_page_map(sys_getenvid(), (void*) (pn*PGSIZE), sys_getenvid(), (void*) (pn*PGSIZE), PTE_P | PTE_U | PTE_COW); //remap in us
  if (err < 0)
    panic("sys_page_map error while trying to duppage, %e", err);

	return 0;
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	// LAB 4: Your code here.
	envid_t envid;

  set_pgfault_handler(&pgfault);
  envid = sys_exofork();

  if (envid < 0)
    panic("sys_exofork: %e", envid);
  if (envid == 0) {
		thisenv = &envs[ENVX(sys_getenvid())];
    set_pgfault_handler(&pgfault);
    return 0;
  }
  sys_env_set_pgfault_upcall(envid, thisenv->env_pgfault_upcall);

  uint32_t addr;
  for (addr = (uint32_t) 0; addr < (uint32_t) USTACKTOP; addr += PGSIZE) {
    if (uvpd[PDX(addr)] & PTE_P) {
      int perms = uvpt[addr/PGSIZE];
      if (perms & PTE_SHARE) {
        sys_page_map(0, (void*) addr, envid, (void*) addr, PTE_SYSCALL | PTE_SHARE);
      } else if ((perms & PTE_P) && (perms & PTE_U) && (perms & PTE_W)) {
        duppage(envid, addr/PGSIZE);
      } else if ((perms & PTE_P) && (perms & PTE_U)) {
        sys_page_map(0, (void*) addr, envid, (void*) addr, PTE_P | PTE_U);
      }
    }
  }

  sys_page_alloc(envid, (void*) (UXSTACKTOP-PGSIZE), PTE_P | PTE_U | PTE_W);

  for (addr = (uint32_t) UXSTACKTOP; addr < (uint32_t) ULIM; addr += PGSIZE)
    sys_page_map(sys_getenvid(), (void*) addr, envid, (void*) addr, PTE_P | PTE_U);

  int r;
  if ((r = sys_env_set_status(envid, ENV_RUNNABLE)) < 0)
    panic("sys_env_set_status: %e", r);

  return envid;
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
  return -E_INVAL;
}
