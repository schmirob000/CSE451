
#include <inc/lib.h>

/*
 * Panic is called on unresolvable fatal errors.
 * It prints "panic: <message>", then causes a breakpoint exception,
 * which causes JOS to enter the JOS kernel monitor.
 */
void
_panic(const char *file, int line, const char *fmt, ...)
{
	va_list ap;
	char buf[256];
	int r = 0;

	va_start(ap, fmt);

	// Print the panic message
	r = snprintf(buf, sizeof(buf), "[%08x] user panic in %s at %s:%d: ",
		     sys_getenvid(), binaryname, file, line);
	r += vsnprintf(buf + r, sizeof(buf) - r, fmt, ap);
	r += snprintf(buf + r, sizeof(buf) - r, "\n");
	sys_cputs(buf, r);

	// Cause a breakpoint exception
	while (1)
		asm volatile("int3");
}

