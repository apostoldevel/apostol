/*++

Library name:

  libdelphi

Module Name:

  Strings.cpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "delphi.hpp"
#include "Strings.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Delphi {

    namespace Strings {

        static char *
        ld_sprintf_num(char *buf, LPCSTR last, uint64_t ui64, char zero, uintptr_t hexadecimal, uintptr_t width)
        {
            char         *p, temp[_INT64_LEN + 1];
            /*
             * we need temp[APP_INT64_LEN] only,
             * but icc issues the warning
             */
            size_t          len;
            uint32_t        ui32;
            static char   hex[] = "0123456789abcdef";
            static char   HEX[] = "0123456789ABCDEF";

            p = temp + _INT64_LEN;

            if (hexadecimal == 0) {

                if (ui64 <= (uint64_t) _MAX_UINT32_VALUE) {

                    /*
                     * To divide 64-bit numbers and to find remainders
                     * on the x86 platform gcc and icc call the libc functions
                     * [u]divdi3() and [u]moddi3(), they call another function
                     * in its turn.  On FreeBSD it is the qdivrem() function,
                     * its source code is about 170 lines of the code.
                     * The glibc counterpart is about 150 lines of the code.
                     *
                     * For 32-bit numbers and some divisors gcc and icc use
                     * a inlined multiplication and shifts.  For example,
                     * unsigned "i32 / 10" is compiled to
                     *
                     *     (i32 * 0xCCCCCCCD) >> 35
                     */

                    ui32 = (uint32_t) ui64;

                    do {
                        *--p = (char) (ui32 % 10 + '0');
                    } while (ui32 /= 10);

                } else {
                    do {
                        *--p = (char) (ui64 % 10 + '0');
                    } while (ui64 /= 10);
                }

            } else if (hexadecimal == 1) {

                do {

                    /* the "(uint32_t)" cast disables the BCC's warning */
                    *--p = hex[(uint32_t) (ui64 & 0xf)];

                } while (ui64 >>= 4);

            } else { /* hexadecimal == 2 */

                do {

                    /* the "(uint32_t)" cast disables the BCC's warning */
                    *--p = HEX[(uint32_t) (ui64 & 0xf)];

                } while (ui64 >>= 4);
            }

            /* zero or space padding */

            len = (temp + _INT64_LEN) - p;

            while (len++ < width && buf < last) {
                *buf++ = zero;
            }

            /* number safe copy */

            len = (temp + _INT64_LEN) - p;

            if (buf + len > last) {
                len = last - buf;
            }

            return MemCopy(buf, p, len);
        }
        //--------------------------------------------------------------------------------------------------------------
        
        char *
        ld_vslprintf(char *buf, char *last, LPCSTR fmt, va_list args)
        {
            char                  *p, zero;
            int                    d;
            double                 f;
            size_t                 len, slen;
            int64_t                i64;
            uint64_t               ui64, frac;
            uintptr_t              width, sign, hex, max_width, frac_width, scale, ms, n;
            string_t            *v;

            while (*fmt && buf < last) {

                /*
                 * "buf < last" means that we could copy at least one character:
                 * the plain character, "%%", "%c", and minus without the checking
                 */

                if (*fmt == '%') {

                    i64 = 0;
                    ui64 = 0;

                    zero = (char) ((*++fmt == '0') ? '0' : ' ');
                    width = 0;
                    sign = 1;
                    hex = 0;
                    max_width = 0;
                    frac_width = 0;
                    slen = (size_t) -1;

                    while (*fmt >= '0' && *fmt <= '9') {
                        width = width * 10 + (*fmt++ - '0');
                    }


                    for ( ;; ) {
                        switch (*fmt) {

                            case 'u':
                                sign = 0;
                                fmt++;
                                continue;

                            case 'm':
                                max_width = 1;
                                fmt++;
                                continue;

                            case 'X':
                                hex = 2;
                                sign = 0;
                                fmt++;
                                continue;

                            case 'x':
                                hex = 1;
                                sign = 0;
                                fmt++;
                                continue;

                            case '.':
                                fmt++;

                                while (*fmt >= '0' && *fmt <= '9') {
                                    frac_width = frac_width * 10 + (*fmt++ - '0');
                                }

                                break;

                            case '*':
                                slen = va_arg(args, size_t);
                                fmt++;
                                continue;

                            default:
                                break;
                        }

                        break;
                    }

                    switch (*fmt) {

                        case 'V':
                            v = va_arg(args, string_t *);

                            len = Min(((size_t) (last - buf)), v->len);
                            buf = MemCopy(buf, v->str, len);
                            fmt++;

                            continue;

                        case 's':
                            p = va_arg(args, char *);

                            if (slen == (size_t) -1) {
                                while (*p && buf < last) {
                                    *buf++ = *p++;
                                }

                            } else {
                                len = Min(((size_t) (last - buf)), slen);
                                buf = MemCopy(buf, p, len);
                            }

                            fmt++;

                            continue;

                        case 'O':
                            i64 = (int64_t) va_arg(args, size_t);
                            sign = 1;
                            break;

                        case 'P':
                            i64 = (int64_t) va_arg(args, pid_t);
                            sign = 1;
                            break;

                        case 'T':
                            i64 = (int64_t) va_arg(args, time_t);
                            sign = 1;
                            break;

                        case 'M':
                            ms = (uintptr_t) va_arg(args, uintptr_t);
                            if ((uintptr_t) ms == -1) {
                                sign = 1;
                                i64 = -1;
                            } else {
                                sign = 0;
                                ui64 = (uint64_t) ms;
                            }
                            break;

                        case 'z':
                            if (sign) {
                                i64 = (int64_t) va_arg(args, size_t);
                            } else {
                                ui64 = (uint64_t) va_arg(args, size_t);
                            }
                            break;

                        case 'i':
                            if (sign) {
                                i64 = (int64_t) va_arg(args, intptr_t);
                            } else {
                                ui64 = (uint64_t) va_arg(args, uintptr_t);
                            }

                            if (max_width) {
                                width = _INT_T_LEN;
                            }

                            break;

                        case 'd':
                            if (sign) {
                                i64 = (int64_t) va_arg(args, int);
                            } else {
                                ui64 = (uint64_t) va_arg(args, u_int);
                            }
                            break;

                        case 'l':
                            if (sign) {
                                i64 = (int64_t) va_arg(args, long);
                            } else {
                                ui64 = (uint64_t) va_arg(args, u_long);
                            }
                            break;

                        case 'D':
                            if (sign) {
                                i64 = (int64_t) va_arg(args, int32_t);
                            } else {
                                ui64 = (uint64_t) va_arg(args, uint32_t);
                            }
                            break;

                        case 'L':
                            if (sign) {
                                i64 = va_arg(args, int64_t);
                            } else {
                                ui64 = va_arg(args, uint64_t);
                            }
                            break;

                        case 'f':
                            f = va_arg(args, double);

                            if (f < 0) {
                                *buf++ = '-';
                                f = -f;
                            }

                            ui64 = (uint64_t) f;
                            frac = 0;

                            if (frac_width) {

                                scale = 1;
                                for (n = frac_width; n; n--) {
                                    scale *= 10;
                                }

                                frac = (uint64_t) lround(((f - (double) ui64) * scale + 0.5));

                                if (frac == scale) {
                                    ui64++;
                                    frac = 0;
                                }
                            }

                            buf = ld_sprintf_num(buf, last, ui64, zero, 0, width);

                            if (frac_width) {
                                if (buf < last) {
                                    *buf++ = '.';
                                }

                                buf = ld_sprintf_num(buf, last, frac, '0', 0, frac_width);
                            }

                            fmt++;

                            continue;

                        case 'p':
                            ui64 = (uintptr_t) va_arg(args, void *);
                            hex = 2;
                            sign = 0;
                            zero = '0';
                            width = 2 * sizeof(void *);
                            break;

                        case 'c':
                            d = va_arg(args, int);
                            *buf++ = (char) (d & 0xff);
                            fmt++;

                            continue;

                        case 'Z':
                            *buf++ = '\0';
                            fmt++;

                            continue;

                        case 'N':
                            *buf++ = '\n';
                            fmt++;

                            continue;

                        case '%':
                            *buf++ = '%';
                            fmt++;

                            continue;

                        default:
                            *buf++ = *fmt++;

                            continue;
                    }

                    if (sign) {
                        if (i64 < 0) {
                            *buf++ = '-';
                            ui64 = (uint64_t) -i64;

                        } else {
                            ui64 = (uint64_t) i64;
                        }
                    }

                    buf = ld_sprintf_num(buf, last, ui64, zero, hex, width);

                    fmt++;

                } else {
                    *buf++ = *fmt++;
                }
            }

            return buf;
        }
        //--------------------------------------------------------------------------------------------------------------

        char *ld_sprintf(char *buf, LPCSTR fmt, ...)
        {
            char   *p;
            va_list   args;

            va_start(args, fmt);
            p = ld_vslprintf(buf, (char *) -1, fmt, args);
            va_end(args);

            return p;
        }
        //--------------------------------------------------------------------------------------------------------------

        char *ld_slprintf(char *buf, char *last, LPCSTR fmt, ...) {

            char   *p;
            va_list   args;

            va_start(args, fmt);
            p = ld_vslprintf(buf, last, fmt, args);
            va_end(args);

            return p;
        }
        //--------------------------------------------------------------------------------------------------------------

        char *ld_snprintf(char *buf, size_t max, LPCSTR fmt, ...) {

            char   *p;
            va_list   args;

            va_start(args, fmt);
            p = ld_vslprintf(buf, buf + max, fmt, args);
            va_end(args);

            return p;
        }
        //--------------------------------------------------------------------------------------------------------------

        char *ld_cpystrn(char *dst, const char *src, size_t n) {

            if (n == 0) {
                return dst;
            }

            while (--n) {
                *dst = *src;

                if (*dst == '\0') {
                    return dst;
                }

                dst++;
                src++;
            }

            *dst = '\0';

            return dst;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchLengthA(LPCSTR psz, size_t cchMax, size_t *pcchLength) {

            *pcchLength = strlen(psz);

            return (*pcchLength <= cchMax) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchCopyA(LPSTR pszDest, size_t cchDest, LPCSTR pszSrc) {

            pszDest = strncpy(pszDest, pszSrc, cchDest);

            return (pszDest[cchDest] == '\0') ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchCatA(LPSTR pszDest, size_t cchDest, LPCSTR pszSrc) {

            pszDest = strncat(pszDest, pszSrc, cchDest);

            return (pszDest[cchDest] == '\0') ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchVPrintfA(LPSTR pszDest, size_t cchDest, LPCSTR pszFormat, va_list argList) {

            int i;

            i = vsnprintf(pszDest, cchDest, pszFormat, argList);

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringPCchVPrintfA(LPSTR pszDest, size_t *pcchDest, LPCSTR pszFormat, va_list argList) {

            int i;
            size_t cchDest = *pcchDest;

            i = vsnprintf(pszDest, cchDest, pszFormat, argList);

            if (i > 0 )
                *pcchDest = (size_t) i;
            else
                *pcchDest = 0;

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchPrintfA(LPSTR pszDest, size_t cchDest, LPCSTR pszFormat, ...) {

            int i;
            va_list args;

            va_start(args, pszFormat);
            i = vsnprintf(pszDest, cchDest, pszFormat, args);
            va_end(args);

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringPCchPrintfA(LPSTR pszDest, size_t *pcchDest, LPCSTR pszFormat, ...) {

            int i;
            va_list args;
            size_t cchDest = *pcchDest;

            va_start(args, pszFormat);
            i = vsnprintf(pszDest, cchDest, pszFormat, args);
            va_end(args);

            if (i > 0 )
                *pcchDest = (size_t) i;
            else
                *pcchDest = 0;

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchLengthW(LPCWSTR psz, size_t cchMax, size_t *pcchLength) {

            *pcchLength = wcslen(psz);

            return (*pcchLength <= cchMax) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchCopyW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszSrc) {

            pszDest = wcsncpy(pszDest, pszSrc, cchDest);

            return (pszDest[cchDest] == '\0') ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchCatW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszSrc) {

            pszDest = wcsncat(pszDest, pszSrc, cchDest);

            return (pszDest[cchDest] == '\0') ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchVPrintfW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszFormat, va_list argList) {

            int i;

            i = vswprintf(pszDest, cchDest, pszFormat, argList);

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringPCchVPrintfW(LPWSTR pszDest, size_t *pcchDest, LPCWSTR pszFormat, va_list argList) {

            int i;
            size_t cchDest = *pcchDest;

            i = vswprintf(pszDest, cchDest, pszFormat, argList);

            if (i > 0 )
                *pcchDest = (size_t) i;
            else
                *pcchDest = 0;

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringCchPrintfW(LPWSTR pszDest, size_t cchDest, LPCWSTR pszFormat, ...) {

            int i;
            va_list   args;

            va_start(args, pszFormat);
            i = vswprintf(pszDest, cchDest, pszFormat, args);
            va_end(args);

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        };
        //--------------------------------------------------------------------------------------------------------------

        HRESULT StringPCchPrintfW(LPWSTR pszDest, size_t *pcchDest, LPCWSTR pszFormat, ...) {

            int i;
            va_list   args;
            size_t cchDest = *pcchDest;

            va_start(args, pszFormat);
            i = vswprintf(pszDest, cchDest, pszFormat, args);
            va_end(args);

            if (i > 0 )
                *pcchDest = (size_t) i;
            else
                *pcchDest = 0;

            return ((i > 0) && (i < cchDest)) ? S_OK : S_FALSE;
        };
        //--------------------------------------------------------------------------------------------------------------
    }
}

using namespace Delphi::Strings;
}
