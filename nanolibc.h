#ifndef NANOLIBC_H
#define NANOLIBC_H

#define flatten __attribute__((flatten))
#define xpure __attribute__((const))
#define pure __attribute__((pure))
#define noreturn __attribute__((noreturn))
#define fastcall __attribute__((fastcall))
#define ainline __attribute__((always_inline))

/* return +/-1 if x is +/- and 0 if x == 0 */
static xpure int sign(int x)
{
    return (x > 0) - (x < 0);
}

#if defined(__x86_64__) || defined(__i386__)
static xpure int Iabs(int x)
{
    __asm__ (
        "cdq \n"
        "xor %%edx, %%eax \n"
        "sub %%edx, %%eax \n"
    : "=eax" (x)
    : "eax" (x)
    : "%edx"
    );
    return x;
}
#define abs(x) Iabs(x)
#endif

/* Function to set the kth bit of n */
static int setBit(int n, int k)
{
    return (n | (1 << (k)));
}

/* Function to clear the kth bit of n */
static int clearBit(int n, int k)
{
    return (n & (~(1 << (k))));
}

/* Function to toggle the kth bit of n */
static int toggleBit(int n, int k)
{
    return (n ^ (1 << (k)));
}

static void str2wide(wchar_t *w, const char *s)
{
    while((*w++ = *s++));
}

void *memsetL(void *dst, int s, size_t count)
{
    register char * a = dst;
    count++;
    while (--count)
        *a++ = s;
    return dst;
}
/* #define memset memsetL */

static wchar_t *wcsuprL(wchar_t *s)
{
    while (*s) {
        wchar_t  x = *s - 'a';
        *s -= (x < 26u) << 5;
        s++;
    }
    return s;
}
static wchar_t *wcslwrL(wchar_t *s)
{
    while (*s) {
        wchar_t  x = *s - 'A';
        *s += (x < 26u) << 5;
        s++;
    }
    return s;
}

static wchar_t *wcschrL(wchar_t *__restrict__ str, const wchar_t c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}
#define wcschr wcschrL

static const char *strchrL(const char *__restrict__ str, const char c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}
#define strchr strchrL

static int wtoiL(const wchar_t *s)
{
    long int v=0;
    int sign=1;
    while (*s == ' ') s++; /*  ||  (unsigned int)(*s - 9) < 5u */

    switch (*s) {
    case '-': sign=-1; /* fall through */
    case '+': ++s;
    }
    while ((unsigned)(*s - '0') < 10u) {
        v = v * 10 + (*s - '0');
        ++s;
    }
    return sign*v;
}
#define _wtoi wtoiL
static inline void reverse(wchar_t *str, int length)
{
    int start = 0;
    int end = length -1;
    while (start < end) {
        wchar_t tmp;
        tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}
static wchar_t *itowL(unsigned num, wchar_t *str, int base)
{
    int i = 0;
    int isNegative = 0;

    /* Handle 0 explicitely */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    /* In standard itoa(), negative numbers are handled only with
     * base 10. Otherwise numbers are considered unsigned. */
    if ((int)num < 0 && base == 10) {
        isNegative = 1;
        num = -(int)num;
    }

    /* Process individual digits */
    while (num != 0) {
        int rem = num % base;
        /* str[i++] = (rem > 9)? (rem-10) + 'A' : rem + '0'; */
        str[i++] = rem + '0' + (rem > 9) * ('A' - '0' - 10); /* branchless version */

        num = num/base;
    }

    /* If number is negative, append '-' */
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0'; /* Append string terminator */

    /* Reverse the string */
    reverse(str, i);

    return str;
}
#define _itow itowL

static size_t wcslenL(const wchar_t *__restrict__ str)
{
    const wchar_t *ptr;
    for (ptr=str; *ptr != '\0'; ptr++);
    return ptr-str;
}
#define wcslen wcslenL

static int wcscmpL(const wchar_t *__restrict__ a, const wchar_t *__restrict__ b)
{
    while(*a && *a == *b) { a++; b++; }
    return *a - *b;
}
#define wcscmp wcscmpL

/* stops comp at the '*' in the b param.
 * this is a kind of mini regexp that has no performance hit.
 * It also returns 0 (equal) if the b param is NULL */
static int wcscmp_star(const wchar_t *__restrict__ a, const wchar_t *__restrict__ b)
{
    if(!b) return 0;
//    if (*b == '*') return 1; /* Should not start with '*' */
    while(*a && *a == *b) { a++; b++; }
    return (*a != *b) & (*b != '*');
}
/* Reverse of the above function */
static int wcscmp_rstar(const wchar_t *__restrict__ a, const wchar_t *__restrict__ b)
{
    const wchar_t *oa = a, *ob=b;
    if(!b) return 0;

    while(*a++) ;
    a--;
    while(*b++) ;
    b--;
    if(*ob != '*' && a-oa != b-ob)
        return 1;

    while(a > oa && b > ob && *a == *b) { a--; b--; }

    return (*a != *b) & (*b != '*');
}


static wchar_t *wcscpyL(wchar_t *__restrict__ dest, const wchar_t *__restrict__ in)
{
    wchar_t *ret = dest;
    while ((*dest++ = *in++));
    return ret;
}
#define wcscpy wcscpyL

static wchar_t *wcsncpyL(wchar_t *__restrict__ dest, const wchar_t *__restrict__ src, size_t n)
{
    wchar_t *orig = dest;
    for (; dest < orig+n && (*dest=*src); ++src, ++dest) ;
    for (; dest < orig+n; ++dest) *dest=0;
    return orig;
}
#define wcsncpy wcsncpyL

static char *strcpyL(char *__restrict__ dest, const char *__restrict__ in)
{
    char *ret = dest;
    while ((*dest++ = *in++));
    return ret;
}
#define strcpy strcpyL

static int wcsicmpL(const wchar_t* s1, const wchar_t* s2)
{
    unsigned x1, x2;

    while (1) {
        x2 = *s2 - 'A';
        x2 |= (x2 < 26u) << 5; /* Add 32 if UPPERCASE. */

        x1 = *s1 - 'A';
        x1 |= (x1 < 26u) << 5;

        s1++; s2++;
        if (x2 != x1)
            break;
        if (x1 == (unsigned)-'A')
            break;
    }
    return x1 - x2;
}
#define wcsicmp wcsicmpL

static int wscsicmp(const wchar_t* s1, const char* s2)
{
    unsigned x1, x2;

    while (1) {
        x2 = *s2 - 'A';
        x2 |= (x2 < 26u) << 5; /* Add 32 if UPPERCASE. */

        x1 = *s1 - 'A';
        x1 |= (x1 < 26u) << 5;

        s1++; s2++;
        if (x2 != x1)
            break;
        if (x1 == (unsigned)-'A')
            break;
    }
    return x1 - x2;
}
#define wcsicmp wcsicmpL

wchar_t *wcscat(wchar_t *__restrict__ dest, const wchar_t *__restrict__ src)
{
    wchar_t *orig=dest;
    for (; *dest; ++dest) ;	/* go to end of dest */
    for (; (*dest=*src); ++src,++dest) ;	/* then append from src */
    return orig;
}

static int strcmpL(const char *X, const char *Y)
{
    while (*X && *X == *Y) {
        X++;
        Y++;
    }
    return *(const unsigned char*)X - *(const unsigned char*)Y;
}
#define strcmp strcmpL

static const wchar_t *wcsstrL(const wchar_t *haystack, const wchar_t *needle)
{
    size_t i,j;
    for (i=0; haystack[i]; ++i) {
        for (j=0; haystack[i+j]==needle[j] && needle[j]; ++j) ;
        if (!needle[j]) return &haystack[i];
    }
    return NULL;
}
#define wcsstr wcsstrL

static const char *strstrL(const char *haystack, const char *needle)
{
    size_t i,j;
    for (i=0; haystack[i]; ++i) {
        for (j=0; haystack[i+j]==needle[j] && needle[j]; ++j) ;
        if (!needle[j]) return &haystack[i];
    }
    return NULL;
}
#define strstr strstrL

static inline unsigned h2u(const wchar_t c)
{
    if      (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'A' && c <= 'F') return c-'A'+10;
    else if (c >= 'a' && c <= 'f') return c-'a'+10;
    else return 0;
}

/* stops at the end of the string or at a space*/
static unsigned whex2u(const wchar_t *s)
{
    unsigned ret=0;
    while(*s && *s != L' ')
       ret = ret << 4 | h2u(*s++) ;

    return ret;
}

static void *reallocL(void *mem, size_t sz)
{
    if(!mem) return HeapAlloc(GetProcessHeap(), 0, sz);
    return HeapReAlloc(GetProcessHeap(), 0, mem, sz);
}
#define realloc reallocL

static void *mallocL(size_t sz)
{
    return HeapAlloc(GetProcessHeap(), 0, sz);
}
#define malloc mallocL

static void *callocL(size_t sz, size_t mult)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz*mult);
}
#define calloc callocL

static BOOL freeL(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}
#define free freeL

#endif
