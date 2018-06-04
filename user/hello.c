// hello, world
#include <inc/lib.h>
#include <inc/string.h>

void
umain(int argc, char **argv)
{
	cprintf("hello, world\n");
	cprintf("i am environment %08x\n", thisenv->env_id);
	sys_set_color(0x400);
  cprintf("%awow colorrr\n", 0x0400);
  cprintf("color is: %x\n", color);
}
