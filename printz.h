#ifndef _PRINTZ_H_
#define _PRINTZ_H_

#include <stdarg.h>
#include <stdio.h>

#define print_stderr pre
#define print_stdout pro

// Print to stderr, with newline; invoke as pre / print_stderr
void pre(const char* __restrict format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stderr, format, va);
	va_end(va);
	fprintf(stderr, "\n");
}

// Print to stdout, with newline; invoke as pro / print_stdout
void pro(const char* __restrict format, ...)
{
	va_list va;

	va_start(va, format);
	vfprintf(stdout, format, va);
	va_end(va);
	fprintf(stdout, "\n");
}

#endif
