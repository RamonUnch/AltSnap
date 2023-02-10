/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2022                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "languages.h"
static struct strings *l10n_ini = NULL;
static struct strings *l10n = (struct strings *)&en_US;

/////////////////////////////////////////////////////////////////////////////
// Copies and remove the accelerators & sign. and txt between ( ).
static size_t lstrcpy_noaccel(TCHAR *__restrict__ dest, const TCHAR *__restrict__ source, size_t destlen)
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

static void lstrcpy_resolve(TCHAR *__restrict__ dest, const TCHAR *__restrict__ source)
{
    // Copy from source to dest, resolving \\n to \n
    for (; *source != '\0'; source++,dest++) {
        if (*source == '\\' && *(source+1) == 'n') {
            *dest = '\n';
            source++;
        } else {
            *dest = *source;
        }
    }
    *dest = '\0';
}

/////////////////////////////////////////////////////////////////////////////
//
static void LoadTranslation(const TCHAR *__restrict__ ini)
{
    size_t i;
    if (!ini) {
        l10n = (struct strings *)&en_US;
        return;
    } else if( INVALID_FILE_ATTRIBUTES == GetFileAttributes(ini) ) {
        return;
    }
    // if english is not seleced then we have to allocate l10_ini strings struct
    // and we have to read the ini file...
    DWORD ret;
    DWORD tsectionlen=16383;
    TCHAR *tsection = NULL;
    do {
         tsectionlen *=2;
         TCHAR *tmp = (TCHAR *)realloc(tsection, tsectionlen*sizeof(TCHAR));
         if(!tmp) { free(tsection); return; }
         tsection = tmp;
         ret = GetPrivateProfileSection(TEXT("Translation"), tsection, tsectionlen, ini);
    } while (ret == tsectionlen-2);
    if (!ret) return;

    if(!l10n_ini) l10n_ini = calloc(1, sizeof(struct strings));
    if(!l10n_ini) return; // Unable to allocate mem
    for (i=0; i < ARR_SZ(l10n_inimapping); i++) {
        // Get pointer to default English string to be used if ini entry doesn't exist
        const TCHAR *const def = ((TCHAR **)&en_US)[i];
        const TCHAR * txt = GetSectionOptionCStr(tsection, l10n_inimapping[i], def);

        TCHAR **deststr = &((TCHAR **)l10n_ini)[i];
        if (deststr == &l10n_ini->about_version) {
            // Append version number to version....
            TCHAR tmp[128];
            lstrcpy(tmp, txt);
            lstrcat(tmp, TEXT(" "APP_VERSION));
            txt = (const TCHAR*)tmp;
        }
        *deststr = realloc( *deststr, (lstrlen_resolved(txt)+1)*sizeof(TCHAR) );
        lstrcpy_resolve(*deststr, txt);
    }
    l10n = l10n_ini;
    free(tsection); // free the cached Translation section.
}
struct langinfoitem *langinfo = NULL;
int nlanguages;

/////////////////////////////////////////////////////////////////////////////
void ListAllTranslations()
{
    if (langinfo) return;

    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    TCHAR szDir[MAX_PATH], fpath[MAX_PATH];
    LPCTSTR txt;

    // First element
    langinfo = malloc(sizeof(struct langinfoitem ));
    if (!langinfo) return;
    langinfo[0].code = en_US.code;
    langinfo[0].lang_english = en_US.lang_english;
    langinfo[0].lang = en_US.lang;
    langinfo[0].author = en_US.author;
    langinfo[0].fn = NULL;
    nlanguages = 1;

    GetModuleFileName(NULL, szDir, ARR_SZ(szDir));
    PathRemoveFileSpecL(szDir);
    lstrcat(szDir, TEXT("\\Lang\\*.ini"));
    lstrcpy(fpath, szDir);
    TCHAR *end = fpath; // not the star!
    end += lstrlen(fpath)-5;
    hFind = FindFirstFile(szDir, &ffd);

    if (hFind != INVALID_HANDLE_VALUE) {
        int n=1;
        do {
            nlanguages++;
            lstrcpy(end, ffd.cFileName); // add filenale at the end of the path
            struct langinfoitem *tmp = realloc(langinfo, sizeof(struct langinfoitem) * nlanguages);
            if (!tmp) break;
            langinfo = tmp;

            // Preload section start
            TCHAR tsection[512];
            DWORD ret = GetPrivateProfileSection(TEXT("Translation"), tsection, ARR_SZ(tsection), fpath);
            if(!ret) continue;

            // Short language code such as en-US, fr-FR, it-IT etc.
            txt = GetSectionOptionCStr(tsection, "Code", TEXT(""));
            langinfo[n].code = calloc(lstrlen(txt)+1, sizeof(TCHAR));
            if (!langinfo[n].code) break;
            lstrcpy(langinfo[n].code, txt);

            // Language name in English
            txt = GetSectionOptionCStr(tsection, "LangEnglish", TEXT(""));
            langinfo[n].lang_english = calloc(lstrlen(txt)+1, sizeof(TCHAR));
            if (!langinfo[n].lang_english) break;
            lstrcpy(langinfo[n].lang_english, txt);

            // Language name in original language
            txt = GetSectionOptionCStr(tsection, "Lang", TEXT(""));
            langinfo[n].lang = calloc(lstrlen(txt)+1, sizeof(TCHAR));
            if (!langinfo[n].lang) break;
            lstrcpy(langinfo[n].lang, txt);

            // Author
            txt = GetSectionOptionCStr(tsection, "Author", TEXT(""));
            langinfo[n].author = calloc(lstrlen(txt)+1, sizeof(TCHAR));
            if (!langinfo[n].author) break;
            lstrcpy(langinfo[n].author, txt);

            // Full file path
            langinfo[n].fn = malloc(lstrlen(fpath)*sizeof(TCHAR)+4);
            if (!langinfo[n].fn) break;
            lstrcpy(langinfo[n].fn, fpath);

            n++;
        } while (FindNextFile(hFind, &ffd));

        FindClose(hFind);
    }
}

/////////////////////////////////////////////////////////////////////////////
void UpdateLanguage()
{
    TCHAR txt[16];
    GetPrivateProfileString(TEXT("General"), TEXT("Language"), TEXT("Auto"), txt, ARR_SZ(txt), inipath);

    // Determine which language should be used
    // based on current user's LCID
    #ifdef _UNICODE
    if (!lstrcmpi(txt, TEXT("Auto"))) {
        LCIDToLocaleNameL(GetUserDefaultLCID(), txt, ARR_SZ(txt), 0);
    }
    #endif // _UNICODE

    if (!lstrcmpi(txt, TEXT("en-US")))
        return; // Hardcoded language

    ListAllTranslations();
    int i;
    for (i=0; i < nlanguages; i++) {
        if (!lstrcmpi(txt, langinfo[i].code)) {
            LoadTranslation(langinfo[i].fn);
            break;
        }
    }
}
