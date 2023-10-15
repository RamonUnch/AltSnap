#ifndef NANOLIBC_H
#define NANOLIBC_H

#include <windows.h>

#define AT_LEAST static
#ifdef __GNUC__
#define flatten __attribute__((flatten))
#define xpure __attribute__((const))
#define pure __attribute__((pure))
#define noreturn __attribute__((noreturn))
#define fastcall __attribute__((fastcall))
#define ainline __attribute__((always_inline))
#define mallocatrib __attribute__((malloc, freeL))
#define allnonnull __attribute__((nonnull))
#define nonnull1(x) __attribute__((nonnull (x)))
#define nonnull2(x, y) __attribute__((nonnull (x, y)))
#define ASSUME(x) do { if (!(x)) __builtin_unreachable(); } while (0)
#define UNREACHABLE() __builtin_unreachable()
#else
#define flatten
#define xpure
#define pure
#define noreturn
#define fastcall
#define ainline
#define mallocatrib
#define allnonnull
#define nonnull1(x)
#define nonnull2(x, y)
#define __restrict__
#define inline
#define ASSUME(x)
#define UNREACHABLE()
#endif
/* return +/-1 if x is +/- and 0 if x == 0 */
static xpure int sign(int x)
{
    return (x > 0) - (x < 0);
}

//#if defined(__GNUC__) && (defined(__i386__) && !defined(__x86_64__)) && !defined(CLANG)
//static __attribute__(( regparm(1) )) xpure
//int Iabs(int x)
//{
//    __asm__ (
//        "cdq \n"
//        "xor %%edx, %%eax \n"
//        "sub %%edx, %%eax \n"
//    : "=eax" (x)
//    : "eax" (x)
//    : "%edx"
//    );
//    return x;
//}
//#define abs(x) Iabs(x)
//#else
//#define abs(x) ((x)>0? (x): -(x))
//#endif // x86

#define abs(x) ((x)>0? (x): -(x))
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

static void str2tchar(TCHAR *w, const char *s)
{
    while((*w++ = *s++));
}

static void str2tchar_s(TCHAR *w, size_t N, const char *s)
{
    const TCHAR *wmax = w + N-1;
    while(w < wmax && (*w++ = *s++));
}

static void mem00(void *dst, size_t count)
{
    register char * a = (char *)dst;
    while (count--)
        *a++ = 0;
}

#ifdef CLANG

void * __cdecl memset(void *dst, int s, size_t count)
{
    register char * a = dst;
    count++;
    while (--count)
        *a++ = s;
    return dst;
}
/* in case */
__cdecl size_t strlen(const char *str)
{
    const char *ptr;
    for (ptr=str; *ptr != '\0'; ptr++);
    return ptr-str;
}
void * __cdecl memcpy(void *dst, const void * __restrict__ src, size_t n)
{
    size_t i;
    char *d = (char *)dst;
    const char *s = (char *)src;

    for (i=0; i<n; i++)
        d++ = s++;
    return dst;
}
#endif

static allnonnull wchar_t *wcsuprL(wchar_t *s)
{
    while (*s) {
        wchar_t  x = *s - 'a';
        *s -= (x < 26u) << 5;
        s++;
    }
    return s;
}
static allnonnull wchar_t *wcslwrL(wchar_t *s)
{
    while (*s) {
        wchar_t  x = *s - 'A';
        *s += (x < 26u) << 5;
        s++;
    }
    return s;
}

static nonnull1(1) pure wchar_t *wcschrL(wchar_t *__restrict__ str, const wchar_t c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}
#define wcschr wcschrL

static nonnull1(1) pure char *strchrL(char *__restrict__ str, const char c)
{
    while(*str != c) {
        if(!*str) return NULL;
        str++;
    }
    return str;
}
#define strchr strchrL


static allnonnull pure int atoiL(const char *s)
{
    long int v=0;
    int ssign=1;
    while (*s == ' ') s++; /*  ||  (unsigned int)(*s - 9) < 5u */

    switch (*s) {
    case '-': ssign=-1; /* fall through */
    case '+': ++s;
    }
    while ((unsigned)(*s - '0') < 10u) {
        v = v * 10 + (*s - '0');
        ++s;
    }
    return ssign*v;
}
#define atoi atoiL

static pure allnonnull int wtoiL(const wchar_t *s)
{
    long int v=0;
    int ssign=1;
    while (*s == ' ') s++; /*  ||  (unsigned int)(*s - 9) < 5u */

    switch (*s) {
    case '-': ssign=-1; /* fall through */
    case '+': ++s;
    }
    while ((unsigned)(*s - '0') < 10u) {
        v = v * 10 + (*s - '0');
        ++s;
    }
    return ssign*v;
}
#define _wtoi wtoiL
static nonnull1(1) void reverseW(wchar_t *str, int length)
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
static nonnull1(2) wchar_t *itowL(unsigned num, wchar_t *str, int base)
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
    reverseW(str, i);

    return str;
}
#define _itow itowL
static nonnull1(1) inline void reverseA(char *str, int length)
{
    int start = 0;
    int end = length -1;
    while (start < end) {
        char tmp;
        tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}
static nonnull1(2) char *itoaL(unsigned num, char *str, int base)
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
        str[i++] = (rem > 9)? (rem-10) + 'A' : rem + '0';
        /* str[i++] = rem + '0' + (rem > 9) * ('A' - '0' - 10);*//* branchless version */

        num = num/base;
    }

    /* If number is negative, append '-' */
    if (isNegative)
        str[i++] = '-';

    str[i] = '\0'; /* Append string terminator */

    /* Reverse the string */
    reverseA(str, i);

    return str;
}
#define _itoa itoaL

#define INT_DIGITS 11
#define UINT_DIGITS 10
static const TCHAR *Int2lStr(TCHAR str[AT_LEAST INT_DIGITS+1], int n)
{
    int i = 0;
    BOOL minus;
    minus = (n<0);
    str[INT_DIGITS] = TEXT('\0');

    for( i=INT_DIGITS-1; ; --i )
    {
        str[i] = TEXT('0') + (minus ? -1*(n%10) : n%10);
        n /= 10;
        if( n==0 )
            break;
    }

    if( minus )
        str[--i] = TEXT('-');

    return str+i;
}

static const wchar_t *Uint2lStrW(wchar_t str[AT_LEAST UINT_DIGITS+1], unsigned n)
{
    int i = 0;
    str[UINT_DIGITS] = TEXT('\0');

    for( i=UINT_DIGITS-1; ; --i )
    {
        str[i] = TEXT('0') + (wchar_t)(n%10);
        if( n < 10 )
            break;
        n /= 10;
    }

    return str+i;
}
static const char *Uint2lStrA(char str[AT_LEAST UINT_DIGITS+1], unsigned n)
{
    int i = 0;
    str[UINT_DIGITS] = TEXT('\0');

    for( i=UINT_DIGITS-1; ; --i )
    {
        str[i] = TEXT('0') + (char)(n%10);
        if( n < 10 )
            break;
        n /= 10;
    }

    return str+i;
}

#ifdef WIN64
#define LPTR_HDIGITS 16
#else
#define LPTR_HDIGITS 8
#endif
static const TCHAR *LPTR2Hex(TCHAR str[AT_LEAST LPTR_HDIGITS+1], UINT_PTR n)
{
    int i = 0;
    str[LPTR_HDIGITS] = TEXT('\0');

    for( i=LPTR_HDIGITS-1; ; --i )
    {
        TCHAR rem = n & 15; // MD 16
        str[i] = (rem > 9)? (rem-10) + 'A' : rem + '0';
        /*str[i++] = rem + '0' + (rem > 9) * ('A' - '0' - 10); *//* branchless version */

        n >>=4 ; // Divide by 16.
        if (n==0)
            break;
    }

    return str+i;
}


static allnonnull pure size_t wcslenL(const wchar_t *__restrict__ const str)
{
    const wchar_t *ptr;
    for (ptr=str; *ptr != '\0'; ptr++);
    return ptr-str;
}
#define wcslen wcslenL

static allnonnull pure size_t strlenL(const char * const str)
{
    const char *ptr;
    for (ptr=str; *ptr != '\0'; ptr++);
    return ptr-str;
}

static int pure allnonnull wcscmpL(const wchar_t *__restrict__ a, const wchar_t *__restrict__ b)
{
    while(*a && *a == *b) { a++; b++; }
    return *a - *b;
}
#define wcscmp wcscmpL

/* Reverse of the next function */
static nonnull1(1) pure int lstrcmp_rstar(const TCHAR *__restrict__ a, const TCHAR *__restrict__ b)
{
    const TCHAR *oa = a, *ob=b;
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
/* stops comp at the '*' in the b param.
 * this is a kind of mini regexp that has no performance hit.
 * It also returns 0 (equal) if the b param is NULL */
static nonnull1(1) pure int lstrcmp_star(const TCHAR *__restrict__ a, const TCHAR *__restrict__ b)
{
    if(!b) return 0;
    if(*b == L'*') return lstrcmp_rstar(a, b);

    while(*a && *a == *b) { a++; b++; }
    return (*a != *b) & (*b != '*');
}
#define tolower(x) ( (x) | ('A'<(x) && (x)<'Z') << 5 )
#define tolowerevil(x) ( (x) | 1 << 5 )

/* Returns 0 if both strings start the same */
static nonnull1(1) int pure lstrcmpi_samestart(const TCHAR *__restrict__ a, const TCHAR *__restrict__ b)
{
    while(*a && tolower(*a) == tolower(*b)) { a++; b++; }
    return (*a != *b) && (*b != '\0');
}

static allnonnull int pure lstrcmp_samestart(const TCHAR *__restrict__ a, const TCHAR *__restrict__ b)
{
    while(*a && *a == *b) { a++; b++; }
    return (*a != *b) && (*b != '\0');
}

static allnonnull wchar_t *wcscpyL(wchar_t *__restrict__ dest, const wchar_t *__restrict__ in)
{
    wchar_t *ret = dest;
    while ((*dest++ = *in++));
    return ret;
}
#define wcscpy wcscpyL

static nonnull2(1,2) wchar_t *wcsncpyL(wchar_t *__restrict__ dest, const wchar_t *__restrict__ src, size_t n)
{
    wchar_t *orig = dest;
    for (; dest < orig+n && (*dest=*src); ++src, ++dest) ;
    for (; dest < orig+n; ++dest) *dest=0;
    return orig;
}
#define wcsncpy wcsncpyL

static allnonnull char *strcpyL(char *__restrict__ dest, const char *__restrict__ in)
{
    char *ret = dest;
    while ((*dest++ = *in++));
    return ret;
}
#define strcpy strcpyL

static allnonnull pure int stricmpL(const char* s1, const char* s2)
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
#define stricmp stricmpL

static allnonnull pure int wcsicmpL(const wchar_t* s1, const wchar_t* s2)
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

static allnonnull pure int strtotcharicmp(const TCHAR* s1, const char* s2)
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

allnonnull wchar_t *wcscatL(wchar_t *__restrict__ dest, const wchar_t *__restrict__ src)
{
    wchar_t *orig=dest;
    for (; *dest; ++dest) ;    /* go to end of dest */
    for (; (*dest=*src); ++src,++dest) ;    /* then append from src */
    return orig;
}
allnonnull char *strcatL(char *__restrict__ dest, const char *__restrict__ src)
{
    char *orig=dest;
    for (; *dest; ++dest) ;    /* go to end of dest */
    for (; (*dest=*src); ++src,++dest) ;    /* then append from src */
    return orig;
}

static nonnull2(1,3) char *strcat_sL(char *__restrict__ dest, const size_t N, const char *__restrict__ src)
{
    char *orig=dest;
    const char *dmax=dest+N-1; /* keep space for a terminating NULL */
    for (; dest<dmax &&  *dest ; ++dest);             /* go to end of dest */
    for (; dest<dmax && (*dest=*src); ++src,++dest);  /* then append from src */
    *dest='\0'; /* ensure result is NULL terminated */
    return orig;
}
#define strcat_s strcat_sL

nonnull2(1,3) wchar_t *wcscat_sL(wchar_t *__restrict__ dest, const size_t N, const wchar_t *__restrict__ src)
{
    wchar_t *orig=dest;
    const wchar_t *dmax=dest+N-1; /* keep space for a terminating NULL */
    for (; dest<dmax &&  *dest ; ++dest);             /* go to end of dest */
    for (; dest<dmax && (*dest=*src); ++src,++dest);  /* then append from src */
    *dest='\0'; /* ensure result is NULL terminated */
    return orig;
}
#define wcscat_s wcscat_sL

static nonnull2(1,3) TCHAR *lstrcpy_s(TCHAR *__restrict__ dest, const size_t N, const TCHAR *__restrict__ src)
{
    TCHAR *orig=dest;
    const TCHAR *dmax=dest+N-1; /* keep space for a terminating NULL */
    for (; dest<dmax && (*dest=*src); ++src,++dest);  /* then append from src */
    *dest='\0'; /* ensure result is NULL terminated */
    return orig;
}

static allnonnull pure int strcmpL(const char *X, const char *Y)
{
    while (*X && *X == *Y) {
        X++;
        Y++;
    }
    return *(const unsigned char*)X - *(const unsigned char*)Y;
}
#define strcmp strcmpL

static allnonnull const char *lstrstrA(const char *haystack, const char *needle)
{
    size_t i,j;
    for (i=0; haystack[i]; ++i) {
        for (j=0; haystack[i+j]==needle[j] && needle[j]; ++j) ;
        if (!needle[j]) return &haystack[i];
    }
    return NULL;
}
static allnonnull const wchar_t *lstrstrW(const wchar_t *haystack, const wchar_t *needle)
{
    size_t i,j;
    for (i=0; haystack[i]; ++i) {
        for (j=0; haystack[i+j]==needle[j] && needle[j]; ++j) ;
        if (!needle[j]) return &haystack[i];
    }
    return NULL;
}

#define itostrA itoaL
#define itostrW itowL
#define strtoiA atoiL
#define strtoiW wtoiL
#define wcsstr lstrstrW
#define strstr lstrstrA
#define lstrcpyA strcpy
#define lstrcpyW wcscpy
#define lstrcatA strcat
#define lstrcat_sA strcat_s
#define lstrcat_sW wcscat_s
#define lstrcatW wcscat
#define lstrlenA strlenL
#define lstrlenW wcslenL
#define lstrcmpA strcmp
#define lstrcmpW wcscmp
#define lstrcmpiA stricmp
#define lstrcmpiW wcsicmp
#define lstrchrA strchrL
#define lstrchrW wcschrL
#ifdef UNICODE
#define lstrstr lstrstrW
#define lstrchr lstrchrW
#define itostr itostrW
#define strtoi strtoiW
#define lstrcat_s lstrcat_sW
#define _itot itowL
#define Uint2lStr Uint2lStrW
#else
#define lstrstr lstrstrA
#define lstrchr lstrchrA
#define itostr itostrA
#define strtoi strtoiA
#define lstrcat_s lstrcat_sA
#define _itot itoaL
#define Uint2lStr Uint2lStrA
#endif

static inline unsigned h2u(const TCHAR c)
{
    if      (c >= '0' && c <= '9') return c - '0';
    else if (c >= 'A' && c <= 'F') return c-'A'+10;
    else if (c >= 'a' && c <= 'f') return c-'a'+10;
    else return 0;
}

/* stops at the end of the string or at a space*/
static allnonnull pure unsigned lstrhex2u(const TCHAR *s)
{
    unsigned ret=0;
    while(*s && *s != L' ')
       ret = ret << 4 | h2u(*s++) ;

    return ret;
}

static void *reallocL(void *mem, size_t sz)
{
//    if (rand()%256 < 200) return NULL;
    if(!mem) return HeapAlloc(GetProcessHeap(), 0, sz);
    return HeapReAlloc(GetProcessHeap(), 0, mem, sz);
}
#define realloc reallocL

static mallocatrib void *mallocL(size_t sz)
{
//    if (rand()%256 < 200) return NULL;
    return HeapAlloc(GetProcessHeap(), 0, sz);
}
#define malloc mallocL

static mallocatrib void *callocL(size_t sz, size_t mult)
{
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sz*mult);
}
#define calloc callocL

static BOOL freeL(void *mem)
{
    if(mem) return HeapFree(GetProcessHeap(), 0, mem);
    return FALSE;
}
#define free freeL

#endif
