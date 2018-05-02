/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
	// Check that the user has permission to read memory [s, s+len).
	// Destroy the environment if not.

	// LAB 3: Your code here.
  user_mem_assert(curenv, (void*) s, len, PTE_U);

	// Print the string supplied by the user.
	cprintf("%.*s", len, s);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
	return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
	return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
	int r;
	struct Env *e;

	if ((r = envid2env(envid, &e, 1)) < 0)
		return r;
	if (e == curenv)
		cprintf("[%08x] exiting gracefully\n", curenv->env_id);
	else
		cprintf("[%08x] destroying %08x\n", curenv->env_id, e->env_id);
	env_destroy(e);
	return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
//	-E_INVAL if perm is inappropriate (see above).
//	-E_NO_MEM if there's no memory to allocate the new page,
//		or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
	// Hint: This function is a wrapper around page_alloc() and
	//   page_insert() from kern/pmap.c.
	//   Most of the new code you write should be to check the
	//   parameters for correctness.
	//   If page_insert() fails, remember to free the page you
	//   allocated!

	// LAB 3: Your code here.
  if (((uint32_t) va >= UTOP) || (((uint32_t) va % PGSIZE) != 0)) return -E_INVAL;
  // check if perm is inappropriate 
  if (!(((perm & PTE_U) | (perm & PTE_P)) && ((perm | PTE_SYSCALL) == PTE_SYSCALL))) return -E_INVAL;

  // find proper env
  struct Env *e;
  int ret = envid2env(envid, &e, 1);
  
  if (ret < 0) {
    return ret; //ret = 0 if envid successful, otherwise = -E_BAD_ENV
  }

  struct PageInfo *newp = page_alloc(ALLOC_ZERO);
  if (!newp) return -E_NO_MEM;

  ret = page_insert(e->env_pgdir, newp, va, perm);
  if (ret) {//Ret = 0 if page_insert was successful, otherwise -E_NO_MEM
    page_free(newp);
    return ret;
  }

  return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//		or the caller doesn't have permission to change one of them.
//	-E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//		or dstva >= UTOP or dstva is not page-aligned.
//	-E_INVAL is srcva is not mapped in srcenvid's address space.
//	-E_INVAL if perm is inappropriate (see sys_page_alloc).
//	-E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//		address space.
//	-E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
	     envid_t dstenvid, void *dstva, int perm)
{
	// Hint: This function is a wrapper around page_lookup() and
	//   page_insert() from kern/pmap.c.
	//   Again, most of the new code you write should be to check the
	//   parameters for correctness.
	//   Use the third argument to page_lookup() to
	//   check the current permissions on the page.

	// LAB 3: Your code here.
  if (((uint32_t) srcva >= UTOP) || (((uint32_t) srcva % PGSIZE) != 0)) return -E_INVAL;
  if (((uint32_t) dstva >= UTOP) || (((uint32_t) dstva % PGSIZE) != 0)) return -E_INVAL;
  // check if perm is inappropriate 
  if (!(((perm & PTE_U) | (perm & PTE_P)) && ((perm | PTE_SYSCALL) == PTE_SYSCALL))) return -1*E_INVAL;

  // find proper env for source
  struct Env *esrc;
  int ret = envid2env(srcenvid, &esrc, 1);
  if (ret > 0) panic("srcret bad");
  if (ret) return ret; //-E_BAD_ENV

  // find proper env for dest
  struct Env *edest;
  ret = envid2env(dstenvid, &edest, 1);
  if (ret > 0) panic("destret bad");
  if (ret) return ret; //-E_BAD_ENV

  pte_t *pstor;
  struct PageInfo *srcpp = page_lookup(esrc->env_pgdir, srcva, &pstor);
  if (!srcpp) return -E_INVAL; //srcva not mapped
  // TODO check if srcva is read only in srcvids address space
  //if ((perm & PTE_W) && !(PGOFF(pstor) & PTE_W)) return -E_INVAL;


  ret = page_insert(edest->env_pgdir, srcpp, dstva, perm);
  if (ret) return ret; //-E_NO_MEM

  return 0;


}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
//	-E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
	// Hint: This function is a wrapper around page_remove().

	// LAB 3: Your code here.
  if (((uint32_t) va >= UTOP) || (((uint32_t) va % PGSIZE) != 0)) return -E_INVAL;

  // find proper env
  struct Env *e;
  int ret = envid2env(envid, &e, 1);
  if (ret) return ret; //-E_BAD_ENV

  page_remove(e->env_pgdir, va);

  return 0;
}

// Dispatches to the correct kernel function, passing the arguments.
  int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
  // Call the function corresponding to the 'syscallno' parameter.
  // Return any appropriate return value.
  // LAB 3: Your code here.
  switch (syscallno) {
    // add cases for each syscall enum (as in the header file)
    case SYS_cputs: //user_mem_assert(curenv, (void*) a1, (size_t) a2, PTE_U);
                    sys_cputs((const char *) a1, (size_t) a2);
                    return 0;
                    break;
    case SYS_cgetc: return sys_cgetc();
                    break;
    case SYS_getenvid: return sys_getenvid();
                       break;
    case SYS_env_destroy: return sys_env_destroy((envid_t) a1);
                          break;
    case SYS_page_alloc: return sys_page_alloc((envid_t) a1, (void *) a2, (int) a3);
                         break;
    case SYS_page_map: return sys_page_map((envid_t) a1, (void *) a2, (envid_t) a3,
                           (void *) a4, (int) a5);
                       break;
    case SYS_page_unmap: return sys_page_unmap((envid_t) a1, (void *) a2);
                         break;
    default:
                return -E_INVAL;
  }
}

