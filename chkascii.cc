#include <stdbool.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <new>
#include <string>

#include <unistd.h>
#include <sys/stat.h>
#include <sys/file.h>

#if !defined _WIN32 && !defined _WIN64
#include <sys/utsname.h>
#endif

#include "assertz.h"
#include "printz.h"

bool good_os()
{
#if defined _WIN32 || defined _WIN64
	return false;
#endif

	bool b_os = false;
	struct utsname sysinfo;

	uname(&sysinfo);

	if (strcmp(sysinfo.sysname, "FreeBSD") == 0)
	{
		b_os = true;
	}
	else if (strcmp(sysinfo.sysname, "Linux") == 0)
	{
		b_os = true;
	}
	else if (strncmp(sysinfo.sysname, "CYGWIN_NT", strlen("CYGWIN_NT")) == 0)
	{
		b_os = true;
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

void voff(const char* errmsg, const bool* pb, ...)
{
	assert_ptr(pb, __LINE__);

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
	assert_ptr(p, __LINE__);
	assert_nz(*p, __LINE__);

	char buffer[16];
	const char* p2 = strchr(p, '=');

	assert_ptr(p2, __LINE__);
	p2++;
	assert_nz(*p2, __LINE__, (long) &usage);

	strcpy(buffer, p2);
	buffer[sizeof(buffer)-1] = 0;
	assert_num(buffer, __LINE__, (long) &usage);

	return atoi(buffer);
}

// Parse a string of the form LEAD=sz1[,sz2] to extract substrings sz<*>
int parse(const char* p, std::string** args)
{
	assert_ptr(args, __LINE__);
	assert_ptr(p, __LINE__);
	assert_nz(*p, __LINE__);

	const char* p2 = strchr(p, '=');

	assert_ptr(p2, __LINE__);
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
	assert_ptr(*args, __LINE__);

	for (int i = 0; i < commas+1; i++)
	{
		assert_ptr(p3, __LINE__);
		char buffer[64];

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
	assert_ptr(pp, __LINE__);
	assert_ptr(psz, __LINE__);

	char buffer[256];

	strcpy(buffer, (*psz).c_str());
	buffer[sizeof(buffer)-1] = 0;
	assert_nz(*buffer, __LINE__, (long) &usage);
	assert_num(buffer, __LINE__, (long) &usage);

	*pp = new(std::nothrow) asciiaccept;
	assert_ptr(*pp, __LINE__);

	(*pp)->next = 0;
	(*pp)->ascii = atoi(buffer);

	return *pp;
}

bool chkchar(int ch, int* plf, const asciiaccept* pacc)
{
	bool b_good = false;
	const int specials[] = { 9, 10 };

	if ((ch > 31) && (ch < 127))
	{
		b_good = true;
	}
	else
	{
		if ((plf) && (ch == 10))
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

		while (pacc)
		{
			if (ch == pacc->ascii)
			{
				b_good = true;
				break;
			}

			pacc = pacc->next;
		}
	}

	return b_good;
}

char* filepath = 0;

void release()
{
	delete[] filepath;
	filepath = 0;
}

int main(int argc, const char* argv[])
{
	atexit(&release);

	if (! good_os())
	{
		pre("Unsupported OS !!");
		exit(-1);
	}

	if (argc < 2)
	{
		usage();
	}

	const int FILEPATHLEN = 1024;
	const int BLOCKSIZE = 512;

	int maxjunk = 1;
	struct stat filestat;

	asciiaccept* pHead = 0;
	asciiaccept* pNode = 0;

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

			std::string* psz;
			int z = 0;

			int accs = parse(argv[argc], &psz);
			assert_nz(accs, __LINE__, (long) &usage);
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
		else if (stat(argv[argc], &filestat) == 0)
		{
			assert_nz(S_ISREG(filestat.st_mode), __LINE__, (long) &usage);

			//Before allocating, delete any existing memory taken up by argv[+]
			delete[] filepath;
			filepath = 0;

			filepath = new(std::nothrow) char[FILEPATHLEN];
			assert_ptr(filepath, __LINE__);

			strcpy(filepath, argv[argc]);
			filepath[FILEPATHLEN-1] = 0;
		}
		else
		{
			usage();
		}

		argc--;
	}

	assert_ptr(filepath, __LINE__, (long) &usage);
	const int len = filestat.st_size;

	if (maxjunk == 0)
	{
		maxjunk = len;
	}

	int fd = open(filepath, O_RDONLY);
	assert_gt(fd, 2, __LINE__);

	int i = 0;
	int lf = 0;
	int loops = 0;
	int to_read = 0;
	int post_lf = 0;
	int baddies = 0;

	int firstbad_ascii = 0;
	int firstbad_offset = 0;
	int firstbad_column = 0;
	int firstbad_row = 0;

	unsigned char block[BLOCKSIZE];
	unsigned char retval = 0;

	while (i < len)
	{
		int j = 0;

		loops++;
		to_read = (len-i < BLOCKSIZE) ? len-i : BLOCKSIZE;
		memset(block, 0, sizeof(block));

		while (j < to_read)
		{
			j += read(fd, (unsigned char*) block+j, to_read-j);
		}

		int k = 0;
		bool b_outbreak = false;

		for (k = 0; k < to_read; k++)
		{
			unsigned char ch = block[k];

			int lf_in = lf;
			bool is_good = chkchar(ch, &lf, pHead);
			post_lf = (lf_in == lf) ? post_lf+1 : 0;

			if (! is_good)
			{
				retval = retval | (1<<0);
				int X = (loops-1)*(sizeof(block)) + k + 1;

				if (baddies == 0)
				{
					firstbad_ascii = ch;
					firstbad_offset = X;
					firstbad_row = lf+1;
					firstbad_column = post_lf;
				}

				baddies++;

				if ((b_summary == false) && (b_quiet == false))
				{
					pro("Bad ASCII (decimal %d) at offset %d", ch, X);
					pro("<Coordinates : line %d ; column %d>", lf+1, post_lf);
				}

				if (baddies >= maxjunk)
				{
					b_outbreak = true;
					k++;
					break;
				}
			}
		}

		i += k;

		if (b_outbreak)
		{
			break;
		}
	}

	close(fd);
	pNode = pHead;

	while (pNode)
	{
		asciiaccept* pNext = pNode->next;
		delete pNode;
		pNode = pNext;
	}

	pHead = 0;

	if (len)
	{
		unsigned char last = 0;

		int lfd = open(filepath, O_RDONLY);
		assert_gt(lfd, 2, __LINE__);

		int lresult = lseek(lfd, -1, SEEK_END);
		assert_ne(lresult, -1, __LINE__);

		int lread = read(lfd, &last, 1);
		assert_eq(lread, 1, __LINE__);

		close(lfd);

		if (last != 10)
		{
			retval = retval | (1<<7);

			if (b_quiet == false)
			{
				pro("<file> is NOEOL");
			}
		}
	}

	if (b_summary)
	{
		pro("Bad ASCII chars/Total bytes = %d/%d", baddies, len);

		if (baddies)
		{
			pro
			(
				"First bad ASCII (decimal %d) at offset %d",
				firstbad_ascii, firstbad_offset
			);

			pro
			(
				"Coordinates: line %d; column %d",
				firstbad_row, firstbad_column
			);
		}
	}

	delete[] filepath;
	filepath = 0;

	return retval;
}
