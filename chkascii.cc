#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <string>

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

void callback(long cb)
{
    typedef void (*pfx)(void);

    if (cb)
    {
        pfx ptr = (pfx) cb;
        (*ptr)();
    }
}

void assert_z(long arg, int line, long cb = 0)
{
    if (arg)
    {
        pre("Runtime check <== 0> failed at line %d", line);

        if (cb)
        {
            callback(cb);
        }

        exit(1);
    }
}

void assert_nz(long arg, int line, long cb = 0)
{
    if (! arg)
    {
        pre("Runtime check <!= 0> failed at line %d", line);

        if (cb)
        {
            callback(cb);
        }

        exit(1);
    }
}

void assert_ptr(const void* arg, int line, long cb = 0)
{
    long ptr = (long) arg;

    if (ptr < sizeof(void*))
    {
        pre("Runtime check <pointer> failed at line %d", line);

        if (cb)
        {
            callback(cb);
        }

        exit(1);
    }
}

bool alldigits(const char* p, bool immediate_exit)
{
    int len = 0;
    int max = strlen(p);

    for (len = 0; len < max; len++)
    {
        if (! isdigit(p[len]))
        {
            pre("Bad digital buffer");

            if (immediate_exit)
            {
                exit(1);
            }
            else
            {
                return false;
            }
        }
    }

    return true;
}

const int BLOCKSIZE = 512;

struct asciiaccept
{
    int ascii;
    asciiaccept* next;
};

void usage()
{
    pre("Usage: chkascii [--accept=<ASCII1>[,<ASCII2>]] <file>");
    exit(1);
}

// Parse a string of the form LEAD=sz1[,sz2] to extract substrings sz<*>
int parse(const char* p, std::string** args)
{
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
    *args = new std::string[commas+1];

    for (int i = 0; i < commas+1; i++)
    {
        assert_ptr(p3, __LINE__);
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

    return (commas+1);
}

asciiaccept* newnode(asciiaccept** pp, const std::string* psz)
{
    assert_ptr(pp, __LINE__);
    assert_ptr(psz, __LINE__);

    char buffer[256];

    strcpy(buffer, (*psz).c_str());
    buffer[sizeof(buffer)-1] = 0;
    assert_nz(*buffer, __LINE__, (long) &usage);
    alldigits(buffer, true);

    *pp = new asciiaccept;
    (*pp)->next = 0;
    (*pp)->ascii = atoi(buffer);

    return *pp;
}

bool chkchar(int ch, int* plf, const asciiaccept* pacc)
{
    bool b_bad = true;
    const int specials[] = { 9, 10 };

    if ((ch > 31) && (ch < 127))
    {
        b_bad = false;
    }
    else
    {
        if ((plf) && (ch == 10))
        {
             (*plf)++;
        }

        for (int i = 0; i < sizeof(specials)/sizeof(specials[0]); i++)
        {
            if (ch == specials[i])
            {
                b_bad = false;
                break;
            }
        }

        while (pacc)
        {
            if (ch == pacc->ascii)
            {
                b_bad = false;
                break;
            }

            pacc = pacc->next;
        }
    }

    return b_bad;
}

int main(int argc, const char* argv[])
{
    if ((argc < 2) || (argc > 3))
    {
        usage();
    }

    const char* ACCEPT = "--accept=";
    asciiaccept* pHead = 0;

    int accindex = 0;
    int fileindex = 1;

    if (argc == 3)
    {
        accindex = 1;
        fileindex = 2;

        assert_z
        (
            strncmp(argv[accindex], ACCEPT, strlen(ACCEPT)),
            __LINE__,
            (long) &usage
        );

        asciiaccept* pNode = 0;
        char argbuffer[16];
        std::string* psz;
        int z = 0;

        int accs = parse(argv[accindex], &psz);
        assert_nz(accs, __LINE__, (long) &usage);
        pHead = newnode(&pNode, &psz[z++]);

        while (z < accs)
        {
            pNode = newnode(&(pNode->next), &psz[z++]);
        }

        delete[] psz;
    }

    struct stat filestat;
    unsigned char block[BLOCKSIZE];
    char filepath[256];

    strcpy(filepath, argv[fileindex]);
    filepath[sizeof(filepath) - 1] = 0;
    int result = stat(filepath, &filestat);

    assert_z(result, __LINE__, (long) &usage);
    assert_nz(S_ISREG(filestat.st_mode), __LINE__, (long) &usage);

    const int len = filestat.st_size;
    int fd = open(filepath, O_RDONLY);

    int i = 0;
    int lf = 0;
    int loops = 0;
    int to_read = 0;

    while (i < len)
    {
        int j = 0;
        int k = 0;

        loops++;
        to_read = (len - i < BLOCKSIZE) ? len - i : BLOCKSIZE;
        memset(block, 0, sizeof(block));

        while (j < to_read)
        {
            j += read(fd, (unsigned char*) (block + j), (to_read - j));
        }

        for (k = 0; k < to_read; k++)
        {
            unsigned char ch = block[k];
            bool is_bad = chkchar(ch, &lf, pHead);

            if (is_bad)
            {
                int X = ((loops - 1)*(sizeof(block))) + k + 1;
                pre("Bad ascii (%d) at offset %d (line %d)", ch, X, lf+1);
                return -1;
            }
        }

        i += to_read;
    }

    close(fd);

    if (i)
    {
        assert_nz(to_read, __LINE__);
        unsigned char last = block[to_read - 1];

        if (last != 10)
        {
            pre("<file> is NOEOL");
            return 1;
        }
    }

    return 0;
}
