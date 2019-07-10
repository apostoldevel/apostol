/*++

Library name:

  libdelphi

Module Name:

  Strings.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_STRINGS_HPP
#define DELPHI_STRINGS_HPP
//----------------------------------------------------------------------------------------------------------------------

typedef struct string_s {
    size_t  len;
    LPCTSTR str;
} string_t, SafeString, *PSafeString;
//----------------------------------------------------------------------------------------------------------------------

#define CreateString(str)    { sizeof(str) / sizeof(TCHAR) - sizeof(TCHAR), (LPCTSTR) (str) }
#define CreateNullString     { 0, nullptr }
#define SetString(str, text)                                               \
    (str)->len = sizeof(text) - sizeof(TCHAR); (str)->str = (LPTSTR) (text)
#define LengthString(str)    ((str).len + 1)
#define LengthPString(str)   ((str)->len + 1)
#define SizeString(str)      ((str).len * sizeof(TCHAR) + sizeof(TCHAR))
#define SizePString(str)     ((str)->len * sizeof(TCHAR) + sizeof(TCHAR))
//----------------------------------------------------------------------------------------------------------------------

#define CopyMemory(Dest, Src, Size)     memcpy(Dest, Src, Size)
#define MoveMemory(Dest, Src, Size)     memmove(Dest, Src, Size)
#define ZeroMemory(Dest, Size)          memset(Dest, 0, Size)
#define SecureZeroMemory                ZeroMemory

#define MemCopy(Dest, Src, Size)        (((LPTSTR) memcpy(Dest, Src, Size)) + (Size))
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Delphi {

    namespace Strings {

        char *ld_cpystrn(char *dst, const char *src, size_t n);
        char *ld_sprintf(char *buf, LPCSTR fmt, ...);
        char *ld_snprintf(char *buf, size_t max, LPCSTR fmt, ...);
        char *ld_vslprintf(char *buf, char *last, LPCSTR fmt, va_list args);
        char *ld_slprintf(char *buf, char *last, LPCSTR fmt, ...);
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchCopyA(LPSTR pszDest, size_t cchDest, LPCSTR pszSrc);
        HRESULT StringCchCatA(LPSTR pszDest, size_t cchDest, LPCSTR pszSrc);
        HRESULT StringCchLengthA(LPCSTR psz, size_t cchMax, size_t *pcchLength);
        HRESULT StringCchPrintfA(LPSTR pszDest, size_t cchDest, LPCSTR pszFormat, ...);
        HRESULT StringCchVPrintfA(LPSTR pszDest, size_t cchDest, LPCSTR pszFormat, va_list argList);
        HRESULT StringPCchPrintfA(LPSTR pszDest, size_t *pcchDest, LPCSTR pszFormat, ...);
        HRESULT StringPCchVPrintfA(LPSTR pszDest, size_t *pcchDest, LPCSTR pszFormat, va_list argList);

        HRESULT StringCchCopyW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszSrc);
        HRESULT StringCchCatW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszSrc);
        HRESULT StringCchLengthW(LPCWSTR psz, size_t cchMax, size_t *pcchLength);
        HRESULT StringCchPrintfW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszFormat, ...);
        HRESULT StringCchVPrintfW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszFormat, va_list argList);
        HRESULT StringPCchPrintfW(LPWSTR pszDest, size_t *pcchDest, LPCWSTR pszFormat, ...);
        HRESULT StringPCchVPrintfW(LPSTR pszDest, size_t *pcchDest, LPCWSTR pszFormat, va_list argList);

        //--------------------------------------------------------------------------------------------------------------
    }
}

using namespace Delphi::Strings;
}

#ifndef UNICODE

#define StringSize(len)         ((len) + 1)

#define AnsiCompare(ls, rs)     strcmp(ls, rs)
#define CompareString(ls, rs)   AnsiCompare(ls, rs)

#define StringCchLength         StringCchLengthA
#define StringCchPrintf         StringCchPrintfA
#define StringCchVPrintf        StringCchVPrintfA
#define StringPCchPrintf        StringPCchPrintfA
#define StringPCchVPrintf       StringPCchVPrintfA

#define StringCchCopy           StringCchCopyA
#define StringCchCat            StringCchCatA

#else

#define StringSize(len)         ((len) * sizeof(TCHAR) + sizeof(TCHAR))

#define WideCompare(ls, rs)     wcscmp(ls, rc)
#define CompareString(ls, rc)   WideCompare(ls, rc)

#define StringCchLength         StringCchLengthW
#define StringCchPrintf         StringCchPrintfW
#define StringCchVPrintf        StringCchVPrintfW
#define StringPCchPrintf        StringPCchPrintfW
#define StringPCchVPrintf       StringPCchVPrintfW

#define StringCchCopy           StringCchCopyW
#define StringCchCat            StringCchCatW

#endif

#endif