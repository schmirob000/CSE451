// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/trap.h>
#include <kern/pmap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display backtrace of the kernel", mon_backtrace },
	{ "showmappings", "Shows mappings for given page range", mon_showmappings },
	{ "modperms", "Modify permissions of mappings", mon_modperms },
	{ "dumpmem", "Dump contents of range of memory", mon_dumpmem },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_dumpmem(int argc, char **argv, struct Trapframe *tf)
{
	return 0;
}

int
mon_modperms(int argc, char **argv, struct Trapframe *tf)
{
	return 0;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
  if (argc < 3) {
    cprintf("Usage: show mappings requires two args representing mem range");
    return -1;
  }

  int begin = atoi(argv[1]);
  int end = atoi(argv[2]);

  uint32_t pgstart = ROUNDDOWN(begin, PGSIZE);
  uint32_t pgend = ROUNDUP(end, PGSIZE);

  cprintf("startpage: 0x%x\n", pgstart);
  cprintf("endpage  : 0x%x\n", pgend);

  for (uint32_t i = pgstart; i < pgend; i += PGSIZE) {
    cprintf("va 0x%x, pa 0x%x", i, PADDR((void *) i));
  }
  return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// int's are 32 bit this works
  int *ebp = (int *) read_ebp();

	cprintf("Stack backtrace:\n");
  while(ebp != 0) {
	  cprintf("ebp %08x  eip %08x  args %08x %08x %08x %08x %08x \n",
        ebp, ebp[1], ebp[2], ebp[3], ebp[4], ebp[5]);

    struct Eipdebuginfo info;
    debuginfo_eip((uintptr_t) ebp[1], &info);

	  cprintf("%s:%d: ", info.eip_file, info.eip_line);
    cprintf("%.*s+%u\n", info.eip_fn_namelen, info.eip_fn_name, ebp[1] - info.eip_fn_addr);

    ebp = (int *) *ebp;
  }

	return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
