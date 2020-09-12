#ifndef _ASSERTZ_H_
#define _ASSERTZ_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

typedef void (*pfx)();

void callback(pfx cb = 0)
{
	if (cb)
	{
		pfx ptr = (pfx) cb;
		(*ptr)();
	}
}

void assert_bool(bool arg, int line = 0, const char* fname = 0, pfx cb = 0)
{
	if (! arg)
	{
		if (line)
		{
			if (fname)
			{
				fprintf
				(
					stderr,
					"Aborting ! Runtime check failed at line %d of file %s\n",
					line,
					fname
				);
			}
			else
			{
				fprintf
				(
					stderr,
					"Aborting ! Runtime check failed at line %d\n",
					line
				);
			}
		}
		else
		{
			fprintf(stderr, "Aborting ! Runtime check failed");
		}

		if (cb)
		{
			callback(cb);
		}

		exit(-1);
	}
}

void assert_z(long arg, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg == 0), line, fname, cb);
}

void assert_nz(long arg, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg != 0), line, fname, cb);
}

void assert_ptr(const void* p, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool(((unsigned long) p >= sizeof(void*)), line, fname, cb);
}

void assert_nullptr(const void* p, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool(((unsigned long) p == 0), line, fname, cb);
}

void assert_gt(long arg, long min, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg > min), line, fname, cb);
}

void assert_ge(long arg, long min, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg >= min), line, fname, cb);
}

void assert_lt(long arg, long max, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg < max), line, fname, cb);
}

void assert_le(long arg, long max, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg <= max), line, fname, cb);
}

void assert_eq(long arg, long rhs, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg == rhs), line, fname, cb);
}

void assert_ne(long arg, long rhs, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_bool((arg != rhs), line, fname, cb);
}

void assert_num(const char* p, int line = 0, const char* fname = 0, pfx cb = 0)
{
	assert_ptr(p, line, fname, cb);
	int max = strlen(p);

	for (int i = 0; i < max; i++)
	{
		assert_bool(isdigit(p[i]), line, fname, cb);
	}
}

#endif
