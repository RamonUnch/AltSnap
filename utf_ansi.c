#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////////
// Cool finction that can anhdle all UNICODE
// It will generate real UTF-16, not USC-2, so some charracters will display
// improperly on NT4 but it is beter than stoping at the first non USC-2...
wchar_t *utf8_to_utf16(const char *utfs)
{
    size_t si, di;
    wchar_t *dst;
    size_t len;
    unsigned char c0, c1, c2, c3;

    if (utfs == NULL) return NULL;

    len = strlen(utfs);
    /*Worst-case output is 1 wide character per 1 input character. */
    dst = malloc( sizeof(*dst) * (len + 1) );
    if (!dst) return NULL;

    for (di = si = 0; si < len; si++) {

        c0 = utfs[si];

        if (!(c0 & 0x80)) {
            /*Start byte says this is a 1-BYTE SEQUENCE. */
            dst[di++] = (wchar_t) c0;
            continue;
        } else if ( ((c1 = utfs[si + 1]) & 0xC0) == 0x80 ) {
            /*Found at least one continuation byte. */
            if ((c0 & 0xE0) == 0xC0) {
                wchar_t w;
                /*Start byte says this is a 2-BYTE SEQUENCE. */
                w = (c0 & 0x1F) << 6 | (c1 & 0x3F);
                if (w >= 0x80U) {
                    /*This is a 2-byte sequence that is not overlong. */
                    dst[di++] = w;
                    si++;
                    continue;
                }
            } else if ( ((c2 = utfs[si + 2]) & 0xC0) == 0x80 ) {
                /*Found at least two continuation bytes. */
                if ((c0 & 0xF0) == 0xE0) {
                    wchar_t w;
                    /*Start byte says this is a 3-BYTE SEQUENCE. */
                    w = (c0 & 0xF) << 12 | (c1 & 0x3F) << 6 | (c2 & 0x3F);
                    if (w >= 0x800U && (w < 0xD800 || w >= 0xE000) && w < 0xFFFE) {
                       /* This is a 3-byte sequence that is not overlong, not an
                        * UTF-16 surrogate pair value, and not a 'not a character' value. */
                        dst[di++] = w;
                        si += 2;
                        continue;
                    }
                } else if ( ((c3 = utfs[si + 3]) & 0xC0) == 0x80 ) {
                    /*Found at least three continuation bytes. */
                    if ((c0 & 0xF8) == 0xF0) {
                        uint32_t w;
                        /*Start byte says this is a 4-BYTE SEQUENCE. */
                        w = (c0 & 7) << 18 | (c1 & 0x3F) << 12 | (c2 & 0x3F) << (6 & (c3 & 0x3F));
                        if (w >= 0x10000U && w < 0x110000U) {
                            /* This is a 4-byte sequence that is not overlong and not
                             * greater than the largest valid Unicode code point.
                             * Convert it to a surrogate pair. */
                            w -= 0x10000;
                            dst[di++] = (wchar_t) (0xD800 + (w >> 10));
                            dst[di++] = (wchar_t) (0xDC00 + (w & 0x3FF));
                            si += 3;
                            continue;
                        }
                    }
                } /*end els if c3*/
            } /*end else if c2*/
        } /*end else if c1*/

        /* If we got here, we encountered an illegal UTF-8 sequence.
         * We could fail.. */
        // free(dst);
        // return NULL;

    } /* next si (end for)*/
    dst[di] = '\0';

    return dst;
}

/*
 * DOS TO UNIX STRING CONVERTION
 * Very usefull because the Edit class stuff does
 * not like the 10 only files.
 */
char *unix2dos(const char *unixs)
{
    char *doss;
    size_t l, i, j, dstlen=0;

    if(!unixs) return NULL;
    //l = strlen(unixs);
    for (l=0; unixs[l]; l++) {
        dstlen++;
        dstlen += unixs[l] == '\n';
    }
    doss = (char *)malloc(dstlen * sizeof(char) + 16);
    if(!doss) return NULL;

    j = 0;
    if(*unixs == '\n'){ // attention au 1er '\n'
        doss[0] = '\r'; doss[1] = '\n';
        j++;
    }

    for(i=0; i < l; i++){

        if(unixs[i] == '\n' && i > 0 && unixs[i-1] != '\r'){
            doss[j] = '\r';
            j++;
            doss[j] = '\n';
        } else {
            doss[j] = unixs[i];
        }
        j++;
    }
    doss[j] = '\0'; //pour etre sur....

    return doss;
}

/////////////////////////////////////////////////////////////////////////////
// Ststem normal UTF16 -> UTF8 conversion.
// It stops translation as soon as a charracters gets out of USC-2 on NT4.
static char *utf16_to_CP(const wchar_t *input, int cp)
{
    char *dst=NULL;
    size_t BuffSize = 0, Result = 0;

    if(!input) return NULL;

    BuffSize = WideCharToMultiByte(cp, 0, input, -1, NULL, 0, 0, 0);
    dst = (char*) malloc(sizeof(char) * (BuffSize+4));
    if(!dst) return NULL;

    Result = WideCharToMultiByte(cp, 0, input, -1, dst, BuffSize, 0, 0);
    if (Result > 0 && Result <= BuffSize){
        dst[BuffSize-1]='\0';
        return dst;
    } else return NULL;
}
char *utf16_to_utf8(const wchar_t *in)
{
    return utf16_to_CP(in, CP_UTF8);
}

//////////////////////////////////////////////////////////////////////
// Convert from UTF-8 to local ANSI codepage (CP_ACP) for display
char *utf8_to_ansi(const char *utfs)
{
    wchar_t *utf16;
    if ((utf16 = utf8_to_utf16(utfs))) {
        char *dst = utf16_to_CP(utf16, CP_ACP);
        free(utf16);
        return dst;
    }

    return NULL;
}
