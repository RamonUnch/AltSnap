/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (C) 2015    Stefan Sundin                                   *
 * This program is free software: you can redistribute it and/or modify  *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation, either version 3 or later.              *
 * Modified By Raymond Gillibert in 2020                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "languages.h"
struct strings *l10n = &en_US;

/////////////////////////////////////////////////////////////////////////////
// Copies and remove the accelerators & sign. and txt between ( ).
static size_t wcscpy_noaccel(wchar_t *__restrict__ dest, wchar_t *__restrict__ source, size_t destlen)
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
static pure size_t wcslen_resolved(wchar_t *__restrict__ str)
{
    // Return the length of str, having resolved escape sequences
    wchar_t *ptr;
    int num_escape_sequences = 0;
    for (ptr=str; *ptr != '\0'; ptr++) {
        if (*ptr == '\\' && *(ptr+1) != '\0') {
            ptr++;
            num_escape_sequences++;
        }
    }
    return ptr-str-num_escape_sequences;
}

static void wcscpy_resolve(wchar_t *__restrict__ dest, wchar_t *__restrict__ source)
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
#define txt_len 1024
static void LoadTranslation(const wchar_t *__restrict__ ini)
{
    wchar_t txt[txt_len];
    size_t i;
    if (!ini) {
        l10n = &en_US;
        return;
    } else if( INVALID_FILE_ATTRIBUTES == GetFileAttributes(ini) ) {
        return;
    }
    for (i=0; i < ARR_SZ(l10n_mapping); i++) {
        // Get pointer to default English string to be used if ini entry doesn't exist
        wchar_t *def = *(wchar_t**) ((void*)&en_US + ((void*)l10n_mapping[i].str - (void*)&l10n_ini));
        GetPrivateProfileString(L"Translation", l10n_mapping[i].name, def, txt, txt_len, ini);
        if (l10n_mapping[i].str == &l10n_ini.about_version) {
            wcscat(txt, L" ");
            wcscat(txt, TEXT(APP_VERSION));
        }
        *l10n_mapping[i].str = realloc( *l10n_mapping[i].str, (wcslen_resolved(txt)+1)*sizeof(wchar_t) );
        wcscpy_resolve(*l10n_mapping[i].str, txt);
    }
    l10n = &l10n_ini;
}
struct langinfoitem *langinfo;
int nlanguages;

/////////////////////////////////////////////////////////////////////////////
void ListAllTranslations()
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATA ffd;
    wchar_t szDir[MAX_PATH], fpath[MAX_PATH], txt[256];

    // First element
    langinfo = malloc(sizeof(struct langinfoitem ));
    langinfo[0].code = en_US.code;
    langinfo[0].lang_english = en_US.lang_english;
    langinfo[0].lang = en_US.lang;
    langinfo[0].author = en_US.author;
    langinfo[0].fn = NULL;
    nlanguages = 1;

    GetModuleFileName(NULL, szDir, ARR_SZ(szDir));
    PathRemoveFileSpecL(szDir);
    wcscat(szDir, L"\\Lang\\*.ini");
    wcscpy(fpath, szDir);
    wchar_t *end = fpath; // not the star!
    end += wcslen(fpath)-5;
    hFind = FindFirstFile(szDir, &ffd);

    if (hFind != INVALID_HANDLE_VALUE) {
        int n=1;
        do {
            nlanguages++;
            wcscpy(end, ffd.cFileName); // add filenale at the end of the path
            langinfo = realloc(langinfo, sizeof(struct langinfoitem) * nlanguages);

            GetPrivateProfileString(L"Translation", L"Code", L"", txt, ARR_SZ(txt), fpath);
            langinfo[n].code = calloc(wcslen(txt)+1, sizeof(wchar_t));
            wcscpy(langinfo[n].code, txt);

            GetPrivateProfileString(L"Translation", L"LangEnglish", L"", txt, ARR_SZ(txt), fpath);
            langinfo[n].lang_english = calloc(wcslen(txt)+1, sizeof(wchar_t));
            wcscpy(langinfo[n].lang_english, txt);

            GetPrivateProfileString(L"Translation", L"Lang", L"", txt, ARR_SZ(txt), fpath);
            langinfo[n].lang = calloc(wcslen(txt)+1, sizeof(wchar_t));
            wcscpy(langinfo[n].lang, txt);

            GetPrivateProfileString(L"Translation", L"Author", L"", txt, ARR_SZ(txt), fpath);
            langinfo[n].author = calloc(wcslen(txt)+1, sizeof(wchar_t));
            wcscpy(langinfo[n].author, txt);

            langinfo[n].fn = malloc(wcslen(fpath)*sizeof(wchar_t)+4);
            wcscpy(langinfo[n].fn, fpath);

            n++;
        } while (FindNextFile(hFind, &ffd));

        FindClose(hFind);
    }
}

/////////////////////////////////////////////////////////////////////////////
void UpdateLanguage()
{
    wchar_t txt[8];
    GetPrivateProfileString(L"General", L"Language", L"en-US", txt, ARR_SZ(txt), inipath);

    int i;
    for (i=0; i < nlanguages; i++) {
        if (!wcsicmp(txt, langinfo[i].code)) {
            LoadTranslation(langinfo[i].fn);
            break;
        }
    }
}
