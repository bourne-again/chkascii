#include "assertz.h"
#include "printz.h"

// Following are the headers already included via assertz.h & printz.h :
// <<<:
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
// :>>>

#if !defined _WIN32 && !defined _WIN64
#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/utsname.h>
#else
#error _WIN32/_WIN64 Not Supported
#endif

#include <new>
#include <string>

#define MAXPATHLEN 1024

#define HTAB 9
#define LINEFEED 10

#define ASCII_MIN 32
#define ASCII_MAX 126

bool good_os()
{
	bool b_os = false;
	struct utsname sysinfo;
	const char* ppos[] = { "BSD", "Linux", "CYGWIN", 0 };

	uname(&sysinfo);

	while (*ppos)
	{
		if (strstr(sysinfo.sysname, *ppos))
		{
			b_os = true;
			break;
		}

		(*ppos)++;
	}

	return b_os;
}

void errexit(const char* errmsg, int errcode = -1)
{
	if (errmsg)
	{
		pre(errmsg);
	}

	exit(errcode);
}

// Ensure a variable number of booleans are all set to false
void voff(const char* errmsg, const bool* pb, ...)
{
	assert_ptr(pb);

	if (*pb)
	{
		errexit(errmsg);
	}

	const bool* p = 0;
	va_list vl;

	va_start(vl, pb);

	while ((p = va_arg(vl, const bool*)))
	{
		if (*p)
		{
			errexit(errmsg);
		}
	}

	va_end(vl);
}

void usage()
{
	pre("Usage: chkascii [options] <file>");
	pre("Legend of options as below:");
	pre("[--accept=<ASCII1>[,<ASCII2>]]");
	pre("[--maxjunk=<MAX>]");
	pre("[--summary]");
	pre("[--quiet]");
	pre("If you specify multiple <file> paths, only the first is considered.");
	pre("All optional arguments are max-once.");
	pre("Mutually exclusive options: --maxjunk|--summary|--quiet");
	pre("Default maxjunk value is 1; use 0 for unlimited.");

	exit(-1);
}

struct asciiaccept
{
	int ascii;
	asciiaccept* next;
};

// Extract a single integer arg after '=' in a parameter string "LEAD=arg"
int extract(const char* p)
{
	assert_ptr(p);
	assert_nz(*p, __LINE__, __FILE__, &usage);

	char buffer[16];
	const char* p2 = strchr(p, '=');

	assert_ptr(p2, __LINE__, __FILE__, &usage);
	p2++;
	assert_nz(*p2, __LINE__, __FILE__, &usage);

	strcpy(buffer, p2);
	buffer[sizeof(buffer)-1] = 0;
	assert_num(buffer, __LINE__, __FILE__, &usage);

	return atoi(buffer);
}

// Parse a string of the form LEAD=sz1[,sz2] to extract substrings sz<*>
int parse(const char* p, std::string** args)
{
	assert_ptr(args);
	assert_ptr(p);
	assert_nz(*p);

	const char* p2 = strchr(p, '=');

	assert_ptr(p2, __LINE__, __FILE__, &usage);
	p2++;

	if (! *p2)
	{
		*args = 0;
		return 0;
	}

	const char* p3 = p2;
	int commas = 0;

	while ((p3 = strchr(p3, ',')))
	{
		commas++;
		p3++;
	}

	p3 = p2;

	*args = new(std::nothrow) std::string[commas+1];
	assert_ptr(*args);

	for (int i = 0; i < commas+1; i++)
	{
		assert_ptr(p3);
		char buffer[256];

		strcpy(buffer, p3);
		buffer[sizeof(buffer)-1] = 0;

		char* p4 = strchr(buffer, ',');

		if (p4)
		{
			*p4 = 0;
		}

		(*args)[i] = buffer;
		p3 = strchr(p3, ',');
		p3++;
	}

	return commas+1;
}

asciiaccept* newnode(asciiaccept** pp, const std::string* psz)
{
	assert_ptr(pp);
	assert_ptr(psz);

	char buffer[16];

	strcpy(buffer, psz->c_str());
	buffer[sizeof(buffer)-1] = 0;
	assert_nz(*buffer, __LINE__, __FILE__, &usage);
	assert_num(buffer, __LINE__, __FILE__, &usage);

	*pp = new(std::nothrow) asciiaccept;
	assert_ptr(*pp);

	(*pp)->next = 0;
	(*pp)->ascii = atoi(buffer);

	return *pp;
}

bool chkchar(int ch, int* plf, const asciiaccept* pacc)
{
	assert_ptr(plf);

	bool b_good = false;
	const int specials[] = { HTAB, LINEFEED };

	if ((ch >= ASCII_MIN) && (ch <= ASCII_MAX))
	{
		b_good = true;
	}
	else
	{
		if (ch == LINEFEED)
		{
			 (*plf)++;
		}

		for (int i = 0; i < (int) (sizeof(specials)/sizeof(specials[0])); i++)
		{
			if (ch == specials[i])
			{
				b_good = true;
				break;
			}
		}

		while ((pacc) && (b_good == false))
		{
			b_good = (ch == pacc->ascii);
			pacc = pacc->next;
		}
	}

	return b_good;
}

char* filepath = 0;
asciiaccept* pHead = 0;
asciiaccept* pNode = 0;

void release()
{
	pNode = pHead;

	while (pNode)
	{
		asciiaccept* pNext = pNode->next;
		delete pNode;
		pNode = pNext;
	}

	delete[] filepath;
}

int main(int argc, const char* argv[])
{
	assert_gt(argc, 1, __LINE__, __FILE__, &usage);
	atexit(&release);

	if (! good_os())
	{
		errexit("Unsupported OS !!");
	}

	int maxjunk = 1;
	struct stat fist;

	const char* ACCEPT = "--accept=";
	const int len_ACCEPT = strlen(ACCEPT);
	bool b_accept = false;

	const char* MAXJUNK = "--maxjunk=";
	const int len_MAXJUNK = strlen(MAXJUNK);
	bool b_maxjunk = false;

	const char* SUMMARY = "--summary";
	const int len_SUMMARY = strlen(SUMMARY);
	bool b_summary = false;

	const char* QUIET = "--quiet";
	const int len_QUIET = strlen(QUIET);
	bool b_quiet = false;

	const char* pexclusive = "Exclusive options: --maxjunk|--summary|--quiet";
	argc--;

	while (argc > 0)
	{
		if (strncmp(argv[argc], ACCEPT, len_ACCEPT) == 0)
		{
			voff("Multiple --accept parameters not permitted", &b_accept, 0);
			b_accept = true;

			std::string* psz = 0;
			int z = 0;

			int accs = parse(argv[argc], &psz);
			assert_ptr(psz, __LINE__, __FILE__, &usage);

			pHead = newnode(&pNode, &psz[z++]);

			while (z < accs)
			{
				pNode = newnode(&(pNode->next), &psz[z++]);
			}

			delete[] psz;
			psz = 0;
		}
		else if (strncmp(argv[argc], MAXJUNK, len_MAXJUNK) == 0)
		{
			voff(pexclusive, &b_maxjunk, &b_quiet, &b_summary, 0);
			b_maxjunk = true;
			maxjunk = extract(argv[argc]);
		}
		else if (strcmp(argv[argc], SUMMARY) == 0)
		{
			voff(pexclusive, &b_maxjunk, &b_quiet, &b_summary, 0);
			b_summary = true;
			maxjunk = 0;
		}
		else if (strcmp(argv[argc], QUIET) == 0)
		{
			voff(pexclusive, &b_maxjunk, &b_quiet, &b_summary, 0);
			b_quiet = true; // implicitly, maxjunk is 1 (the default value)
		}
		else if (stat(argv[argc], &fist) == 0)
		{
			if (! (strlen(argv[argc]) < MAXPATHLEN))
			{
				char buff[64];
				sprintf(buff, "Max permitted path length is %d", MAXPATHLEN-1);
				errexit(buff);
			}

			assert_nz(S_ISREG(fist.st_mode), __LINE__, __FILE__, &usage);

			//Before allocating, delete any existing memory taken up by argv[+]
			delete[] filepath;

			filepath = new(std::nothrow) char[MAXPATHLEN];
			assert_ptr(filepath);

			strcpy(filepath, argv[argc]);
			filepath[MAXPATHLEN-1] = 0;
		}
		else
		{
			usage();
		}

		argc--;
	}

	assert_ptr(filepath, __LINE__, __FILE__, &usage);
	const int len = fist.st_size;

	if (maxjunk == 0)
	{
		maxjunk = len;
	}

	int i = 0;
	int lf = 0;
	int post_lf = 0;
	int baddies = 0;

	int firstbad_ascii = 0;
	int firstbad_offset = 0;
	int firstbad_column = 0;
	int firstbad_row = 0;

	unsigned char uc;
	unsigned char retval = 0;

	int fd = 0;

	if (len)
	{
		unsigned char last = 0;

		fd = open(filepath, O_RDONLY);
		assert_gt(fd, 2);	// 0 -> stdin; 1 -> stdout; 2 -> stderr;

		int j = pread(fd, (unsigned char*) &last, 1, len-1);
		assert_eq(j, 1);

		if (last != LINEFEED)
		{
			retval = retval | (1<<7);
		}
	}

	while (i < len)
	{
		int k = read(fd, (unsigned char*) &uc, 1);
		assert_eq(k, 1);

		int lf_in = lf;
		bool is_good = chkchar(uc, &lf, pHead);
		post_lf = (lf_in == lf) ? post_lf+1 : 0;

		if (! is_good)
		{
			retval = retval | (1<<0);

			if (baddies == 0)
			{
				firstbad_ascii = uc;
				firstbad_offset = i;
				firstbad_row = lf+1;
				firstbad_column = post_lf;
			}

			baddies++;

			if ((b_summary == false) && (b_quiet == false))
			{
				pro("Bad ASCII (decimal %d) at offset %d", uc, i);
				pro("<Coordinates : line %d ; column %d>", lf+1, post_lf);
			}

			if (baddies >= maxjunk)
			{
				break;
			}
		}

		i++;
	}

	if (fd)
	{
		close(fd);
	}

	if ((retval & (1<<7)) && (b_quiet == false))
	{
		pro("<file> is NOEOL");
	}

	if (b_summary)
	{
		pro("Bad ASCII chars/Total bytes = %d/%d", baddies, len);

		if (baddies)
		{
			pro
			(
				"First bad ASCII (decimal %d) at offset %d",
				firstbad_ascii,
				firstbad_offset
			);

			pro
			(
				"Coordinates: line %d; column %d",
				firstbad_row,
				firstbad_column
			);
		}
	}

	return retval;
}
