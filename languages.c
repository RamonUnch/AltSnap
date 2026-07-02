/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2022                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
#include "languages.h"

static struct strings *l10n_ini = NULL;
static const struct strings *l10n = &en_US;

/////////////////////////////////////////////////////////////////////////////
// Copies and remove the accelerators & sign. and txt between ( ).
static size_t lstrcpy_noaccel(TCHAR *__restrict__ dest, size_t destlen, const TCHAR *__restrict__ source)
{
    size_t i=0, j=0;
    while(i < destlen && source[i]) {
        dest[j] = source[i];
        if (source[i] == '(') {
            // If we found a '(' go to the closing one ')'.
            while(i < destlen && source[i] && source[i] != ')') i++;
            i++;
            i += source[i] == ' '; // Remove space after ) if there is one.
        } else {
            j += source[i] != '&';
            i++;
        }
    }
    dest[j] = '\0';
    return j;
}
/////////////////////////////////////////////////////////////////////////////
static pure size_t lstrlen_resolved(const TCHAR *__restrict__ str)
{
    // Return the length of str, having resolved escape sequences
    const TCHAR *ptr;
    int num_escape_sequences = 0;
    for (ptr=str; *ptr != '\0'; ptr++) {
        if (*ptr == '\\' && *(ptr+1) != '\0') {
            ptr++;
            num_escape_sequences++;
        }
    }
    return ptr-str-num_escape_sequences;
}

static void lstrcpy_resolve(TCHAR *__restrict__ dest, size_t dstlen, const TCHAR *__restrict__ source)
{
    // Copy from source to dest, resolving \\n to \n
    for (; --dstlen && *source != '\0'; source++,dest++) {
        if (*source == '\\' && *(source+1) == 'n') {
            *dest = '\n';
            source++;
        } else {
            *dest = *source;
        }
    }
    *dest = '\0';
}

static void lstrcpy_encode(TCHAR *__restrict__ dst, size_t dstlen, const TCHAR *__restrict__ src)
{
    // Copy from source to dest, encoding '\n' to '\' 'n'
    for (; --dstlen && *src != '\0'; src++,dst++) {
        if (dstlen && *src == '\n') {
            *dst++ = '\\'; *dst = 'n';
            --dstlen;
        } else {
            *dst = *src;
        }
    }
    *dst = '\0';
}
/////////////////////////////////////////////////////////////////////////////
//
static void LoadTranslationOrTT(const TCHAR *__restrict__ ini, const TCHAR * __restrict__ section_name, int offset)
{
    // if english is not seleced then we have to allocate l10_ini strings struct
    // and we have to read the ini file...
    //RGTICTAC tt; RGTic(&tt);
    DWORD ret;
    DWORD tsectionlen=16383;
    TCHAR *tsection = NULL;
    do {
         tsectionlen *=2;
         TCHAR *tmp = (TCHAR *)realloc(tsection, tsectionlen*sizeof(TCHAR));
         if(!tmp) { free(tsection); return; }
         tsection = tmp;
         ret = GetPrivateProfileSection(section_name, tsection, tsectionlen, ini);
    } while (ret == tsectionlen-2);

    if (!ret || !*tsection) { free(tsection); return; }

    if(!l10n_ini) { l10n_ini = (struct strings *)calloc(1, sizeof(struct strings)); }
    if(!l10n_ini) return; // Unable to allocate mem

    TCHAR const *ini_map[ARR_SZ(l10n_inimapping)];
    RGIniMapSection(tsection, ini_map, l10n_inimapping, ARR_SZ(l10n_inimapping));

    for (size_t i=0; i < ARR_SZ(l10n_inimapping); i++) {
        // Get pointer to default English string to be used if ini entry doesn't exist
        const TCHAR *const def_val = ((TCHAR **)&en_US)[i*2+offset];
        const TCHAR *txt = ini_map[i] ? ini_map[i] : def_val;
        if (!txt)
            continue; // default value may be NULL...

        TCHAR buf[128];
        TCHAR **deststr = &((TCHAR **)l10n_ini)[i*2+offset];
        if (deststr == &l10n_ini->AboutVersion) {
            // Append version number to version....
            lstrcpy_s(buf, ARR_SZ(buf), txt);
            lstrcat_s(buf, ARR_SZ(buf), TEXT(" ") TEXT(APP_VERSION));
            txt = (const TCHAR*)buf;
        }
        size_t destlen = lstrlen(txt) + 1;
        TCHAR *t = (TCHAR *)realloc( *deststr, destlen * sizeof(TCHAR) );
        if (!t) continue;
        *deststr = t;
        lstrcpy_resolve(*deststr, destlen, txt);
    }
    l10n = l10n_ini;
    free(tsection); // free the cached Translation section.
    //LOGA("LoadTranslationOrTT in %u us", (unsigned)RGTac(&tt));
}

static void LoadTranslation(const TCHAR *__restrict__ ini)
{
    if (!ini) {
        l10n = (struct strings *)&en_US;
        return;
    } else if( INVALID_FILE_ATTRIBUTES == GetFileAttributes(ini) ) {
        return;
    }

    LoadTranslationOrTT(ini, TEXT("Translation"), 0);
    LoadTranslationOrTT(ini, TEXT("ToolTips"),    1);
}

struct langinfoitemList {
    struct langinfoitem *it;
    size_t num;
    size_t cap;
} langinfo = { NULL, 0, 0 };
/////////////////////////////////////////////////////////////////////////////
void ListAllTranslations()
{
    if (langinfo.it) return;

    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    TCHAR szDir[MAX_PATH], fpath[MAX_PATH*2];
    LPCTSTR txt;

    // First element
    struct langinfoitem *lnfo = (struct langinfoitem *)ListAppend( &langinfo, NULL, sizeof(*lnfo) );
    if (!lnfo) return;
    lstrcpy_s(lnfo->code, ARR_SZ(lnfo->code), en_US.Code);
    lnfo->lang_english = en_US.LangEnglish;
    lnfo->lang = en_US.Lang;
    lnfo->author = en_US.Author;
    lnfo->fn = NULL;

    GetModuleFileName(NULL, szDir, ARR_SZ(szDir));
    PathRemoveFileSpecL(szDir);
    lstrcat_s(szDir, ARR_SZ(szDir), TEXT("\\Lang\\*.ini"));
    lstrcpy_s(fpath, ARR_SZ(fpath), szDir);
    TCHAR *end = fpath; // not the star!
    end += lstrlen(fpath)-5;
    hFind = FindFirstFile(szDir, &ffd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            lstrcpy(end, ffd.cFileName); // add filenale at the end of the path
            lnfo = (struct langinfoitem *)ListAppend( &langinfo, NULL, sizeof(*lnfo) );
            if (!lnfo) break;

            // Preload section start
            TCHAR tsection[512];
            DWORD ret = GetPrivateProfileSection(TEXT("Translation"), tsection, ARR_SZ(tsection), fpath);
            if(!ret) continue;

            // Short language code such as en-US, fr-FR, it-IT etc.
            txt = GetSectionOptionCStr(tsection, "Code", TEXT(""));
            lstrcpy_s(lnfo->code, ARR_SZ(lnfo->code), txt);

            // Language name in English
            txt = GetSectionOptionCStr(tsection, "LangEnglish", TEXT(""));
            lnfo->lang_english = lstrdup(txt);

            // Language name in original language
            txt = GetSectionOptionCStr(tsection, "Lang", TEXT(""));
            lnfo->lang = lstrdup(txt);

            // Author
            txt = GetSectionOptionCStr(tsection, "Author", TEXT(""));
            lnfo->author = lstrdup(txt);

            // Full file path
            lnfo->fn = lstrdup(fpath);
        } while (FindNextFile(hFind, &ffd));

        FindClose(hFind);
    }
}

static void Generate_en_US_base_txt()
{
    HANDLE h = CreateFileA( "en_US.txt", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if( h == INVALID_HANDLE_VALUE )
        return;

    DWORD dummy = 0;
    TCHAR key[100], val[1000];
    const TCHAR **const def_strings = ((const TCHAR **)&en_US);
    WriteFile(h, "\xFF\xFE", 2, &dummy, NULL); // UTF-16 LE BOM
    WriteFile(h, TEXT("; Base AltSnap Translation\r\n[Translation]\r\n"), 43 * sizeof(TCHAR), &dummy, NULL);

    for (size_t i = 0; i < ARR_SZ(l10n_inimapping); i++) {

        TCHAR *k = key;
        const char *p = l10n_inimapping[i];
        while ((*k++ = *p++));

        const TCHAR *vv = def_strings[i*2];
        lstrcpy_encode(val, ARR_SZ(val), vv);

        WriteFile(h, key, lstrlen(key) * sizeof(TCHAR), &dummy, NULL);
        WriteFile(h, TEXT("="), sizeof(TCHAR), &dummy, NULL);
        WriteFile(h, val, lstrlen(val) * sizeof(TCHAR), &dummy, NULL);
        WriteFile(h, TEXT("\r\n"), 2 * sizeof(TCHAR), &dummy, NULL);
    }
    CloseHandle(h);
}

#ifdef UNICODE
/////////////////////////////////////////////////////////////////////////////
// Helper function to get
static int GetCUserLanguage_xx_XX(wchar_t txt[AT_LEAST 16])
{
    txt[0] = L'\0';
    return LCIDToLocaleNameL(GetUserDefaultLCID(), txt, 16, 0);
}
#endif
/////////////////////////////////////////////////////////////////////////////
void UpdateLanguage()
{
    TCHAR txt[16];
    GetPrivateProfileString(TEXT("General"), TEXT("Language"), TEXT("Auto"), txt, ARR_SZ(txt), inipath);

    // Determine which language should be used
    // based on current user's LCID
    #ifdef _UNICODE
    if (!lstrcmpi(txt, TEXT("Auto"))) {
        GetCUserLanguage_xx_XX(txt);
    }
    #endif // _UNICODE

    if (!lstrcmpi(txt, TEXT("en-US"))) {
        return; // Hardcoded language
    }

    ListAllTranslations();
    for (size_t i=0; i < langinfo.num; i++) {
        if (!lstrcmpi(txt, langinfo.it[i].code)) {
            LoadTranslation(langinfo.it[i].fn);
            break;
        }
    }
}

void FreeAllLangRelated()
{
    if (langinfo.it) {
        for (size_t i=1; i < langinfo.num; i++) {
            lstrfree(langinfo.it[i].lang_english);
            lstrfree(langinfo.it[i].lang);
            lstrfree(langinfo.it[i].author);
            lstrfree(langinfo.it[i].fn);
        }
        ListFree(&langinfo);
        langinfo.it = NULL;
        langinfo.num = langinfo.cap = 0;
    }

    if (l10n_ini) {
        for (size_t i=0; i < sizeof(struct strings) / sizeof(TCHAR*); i++) {
            TCHAR **deststr = &((TCHAR **)l10n_ini)[i];
            free(*deststr);
        }
        free(l10n_ini);
        l10n_ini = NULL;
    }
    l10n = &en_US;
}
