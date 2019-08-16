/*++

Library name:

  libdelphi

Module Name:

  SysUtils.cpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "delphi.hpp"
#include "SysUtils.hpp"

extern "C++" {

namespace Delphi {

    namespace SysUtils {

        CDayTable MonthDays[] = { {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
                                  {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31} };
        //--------------------------------------------------------------------------------------------------------------

#ifdef _DEBUG

        LIB_DELPHI void DebugSystemError(LPCTSTR lpFunctionName) {

            LPTSTR lpMsgBuf;
            TCHAR S[MAX_ERROR_STR + sizeof(TCHAR)] = {0};

            if (DefaultLocale.Locale() == LC_GLOBAL_LOCALE) {
                lpMsgBuf = ::strerror_r(errno, S, MAX_ERROR_STR);
            } else {
                lpMsgBuf = ::strerror_l(errno, DefaultLocale.Locale());
            }

            DebugMessage(_T("[%04i] %s: %s"), ::getpid(), lpFunctionName, lpMsgBuf);
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void DebugMessage(LPCTSTR lpFormat, ...) {

            TCHAR szBuffer[MAX_ERROR_STR + sizeof(TCHAR)] = {0};

            if (lpFormat != nullptr) {
                va_list args;
                va_start(args, lpFormat);
                chVERIFY(SUCCEEDED(StringCchVPrintf(szBuffer, MAX_ERROR_STR, lpFormat, args)));
                va_end(args);
            }

            ::write(STDERR_FILENO, (LPTSTR) szBuffer, strlen(szBuffer));
        };

#else

        LIB_DELPHI void DebugSystemError(LPCTSTR lpFunctionName)
        {
            return;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void DebugMessage(LPCTSTR lpValue, ...)
        {
            return;
        };
        //--------------------------------------------------------------------------------------------------------------

#endif
        LIB_DELPHI size_t MemoryPos(char *ASubStr, char *ABuffer, size_t ASize) {

            size_t LResult = 0;
            size_t LSize, LSearchLength, LS1;

            char LChar;
            char *LPS, *LPM;

            chVERIFY(SUCCEEDED(StringCchLengthA(ASubStr, MAX_BUFFER_SIZE, &LSize)));

            if ((LSize == 0) || (LSize > ASize))
                return 0;

            LChar = *ASubStr;  // first char
            LPM = ABuffer;
            LS1 = LSize - 1;
            LSearchLength = ASize - LS1; // MemorySize - LS + 1

            if (LS1 == 0) // optimization for freq used LF
            {
                while (LSearchLength > 0 && LResult == 0) {
                    if (*LPM == LChar)
                        LResult = LPM - ABuffer + 1;

                    LPM++;
                    LSearchLength--;
                }
            } else {
                LPS = new CHAR[LSize + 1];
                ::SecureZeroMemory(LPS, LSize + 1);

                try {
                    while (LSearchLength > 0 && LResult == 0) {
                        if (*LPM == LChar) {
                            ::CopyMemory(LPS, LPM, LSize);

                            if (AnsiCompareText(ASubStr, LPS) == 0)
                                LResult = LPM - ABuffer + 1;
                        }

                        LPM++;
                        LSearchLength--;
                    }
                }
                catch (...) {
                    delete[] LPS;
                    throw;
                }
                delete[] LPS;
            }

            return LResult;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI BOOL OSCheck(BOOL RetVal) {
            if (!RetVal)
                throw EOSError(errno);
            return RetVal;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool CreateDir(LPCSTR lpPathName, mode_t Mode) {
            return mkdir(lpPathName, Mode) == 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ForceDirectories(LPCSTR lpPathName, mode_t Mode) {
            LPSTR   p;
            char    ch;
            int     err;

            err = 0;

            p = (LPSTR) lpPathName + 1;

            for ( /* void */ ; *p; p++) {
                ch = *p;

                if (ch != '/') {
                    continue;
                }

                *p = '\0';

                if (!CreateDir(lpPathName, Mode)) {
                    err = errno;

                    switch (err) {
                        case EEXIST:
                            err = 0;
                        case EACCES:
                            break;

                        default:
                            return err;
                    }
                }

                *p = '/';
            }

            return err == 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI int AnsiCompareText(LPCSTR S1, LPCSTR S2) {
            return strcmp(S1, S2);
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI int WideCompareText(const wchar_t *S1, const wchar_t *S2) {
            return wcscmp(S1, S2);
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool AnsiSameText(LPCSTR S1, LPCSTR S2) {
            return AnsiCompareText(S1, S2) == 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool WideSameText(const wchar_t *S1, const wchar_t *S2) {
            return WideCompareText(S1, S2) == 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void UpperCase(char *Dest, size_t Len, LPCSTR Source) {
            char Ch;
            size_t L = Len;

            while (L != 0) {
                Ch = *Source;
                if (Ch >= 'a' && Ch <= 'z')
                    Ch -= 32;
                *Dest = Ch;
                Source++;
                Dest++;
                L--;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void LowerCase(char *Dest, size_t Len, LPCSTR Source) {
            char Ch;
            size_t L = Len;

            while (L != 0) {
                Ch = *Source;
                if (Ch >= 'A' && Ch <= 'Z')
                    Ch += 32;
                *Dest = Ch;
                Source++;
                Dest++;
                L--;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool IsCtl(int ch) {
            return (ch >= 0 && ch <= 31) || ch == 127;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool IsNumeral(int ch) {
            return ch >= 48 && ch <= 57;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool IsChar(int ch) {
            return (ch >= 48 && ch <= 57) || (ch >= 65 && ch <= 90) || (ch >= 97 && ch <= 122) || ch == 95;
        }
        //--------------------------------------------------------------------------------------------------------------

        CTimeStamp DateTimeToTimeStamp(CDateTime DateTime) {
            CTimeStamp Result;
            long int LTemp, LTemp2;
            LTemp = DateTime * MSecsPerDay;
            LTemp2 = round(LTemp / MSecsPerDay);
            Result.Date = (int) (DateDelta + LTemp2);
            Result.Time = (int) (labs(LTemp) % MSecsPerDay);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void DivMod(int Dividend, int Divisor, int &Result, int &Remainder) {
            Result = Dividend / Divisor;
            Remainder = Dividend % Divisor;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool DecodeDateFully(const CDateTime DateTime, int &Year, int &Month, int &Day, int &DOW) {

            const long int D1 = 365;
            const long int D4 = D1 * 4 + 1;
            const long int D100 = D4 * 25 - 1;
            const long int D400 = D100 * 4 + 1;

            bool Result = false;

            long int T;
            int Y, M, D, I;

//            PDayTable DayTable;

            T = DateTimeToTimeStamp(DateTime).Date;
            if (T <= 0) {
                Year = 0;
                Month = 0;
                Day = 0;
                DOW = 0;
            } else {
                DOW = (int) (T % 7) + 1;
                T--;
                Y = 1;
                while (T >= D400) {
                    T -= D400;
                    Y += 400;
                }
                DivMod(T, D100, I, D);
                if (I == 4) {
                    I--;
                    D += D100;
                }
                Y += (I * 100);
                DivMod(D, D4, I, D);
                Y += (I * 4);
                DivMod(D, D1, I, D);
                if (I == 4) {
                    I--;
                    D += D1;
                }
                Y += I;
                Result = IsLeapYear(Y);
//                DayTable = &MonthDays[(int) Result];
                M = 1;
                while (true) {
                    I = MonthDays[Result][M-1];
                    if (D < I) break;
                    D -= I;
                    M++;
                }
                Year = Y;
                Month = M;
                Day = D + 1;
            }

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void DecodeDate(CDateTime DateTime, int &Year, int &Month, int &Day) {
            int Dummy;
            DecodeDateFully(DateTime, Year, Month, Day, Dummy);
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void DecodeTime(CDateTime DateTime, int &Hour, int &Min, int &Sec, int &MSec) {
            int MinCount, MSecCount;
            DivMod(DateTimeToTimeStamp(DateTime).Time, SecsPerMin * MSecsPerSec, MinCount, MSecCount);
            DivMod(MinCount, MinsPerHour, Hour, Min);
            DivMod(MSecCount, MSecsPerSec, Sec, MSec);
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool IsLeapYear(int Year) {
            return (Year % 4 == 0) && ((Year % 100 != 0) || (Year % 400 == 0));
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool TryEncodeDate(int Year, int Month, int Day, CDateTime &Date) {
            int I;
            bool LY = IsLeapYear(Year);

            if ((Year >= 1) && (Year <= 9999) && (Month >= 1) && (Month <= 12) && (Day >= 1) &&
                (Day <= MonthDays[LY][Month - 1])) {
                for (I = 0; I < Month - 1; ++I)
                    Day += MonthDays[LY][I];

                I = Year - 1;
                Date = I * 365 + div(I, 4).quot - div(I, 100).quot + div(I, 400).quot + Day - DateDelta;

                return true;
            }

            return false;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool TryEncodeTime(int Hour, int Min, int Sec, int MSec, CDateTime &Time) {
            if ((Hour < HoursPerDay) && (Min < MinsPerHour) && (Sec < SecsPerMin) && (MSec < MSecsPerSec)) {
                Time = (CDateTime) ((Hour * MinsPerHour * SecsPerMin * MSecsPerSec) +
                                 (Min * SecsPerMin * MSecsPerSec) +
                                 (Sec * MSecsPerSec) + MSec) / MSecsPerDay;
                return true;
            }
            return false;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime EncodeTime(int Hour, int Min, int Sec, int MSec) {
            CDateTime Result;
            if (!TryEncodeTime(Hour, Min, Sec, MSec, Result))
                throw Exception::Exception(_T("Invalid argument to time encode"));
            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime EncodeDate(int Year, int Month, int Day) {
            CDateTime Result;
            if (!TryEncodeDate(Year, Month, Day, Result))
                throw Exception::Exception(_T("Invalid argument to date encode"));
            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime SystemTimeToDateTime(const struct tm *tm, struct timespec *ts) {

            CDateTime Result = EncodeDate(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

            if (Result >= 0)
                Result = Result + EncodeTime(tm->tm_hour, tm->tm_min, tm->tm_sec, (int) (ts->tv_nsec / 1000000));
            else
                Result = Result - EncodeTime(tm->tm_hour, tm->tm_min, tm->tm_sec, (int) (ts->tv_nsec / 1000000));

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime SystemTimeToDateTime(const struct tm *tm, int msec) {

            CDateTime Result = EncodeDate(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);

            if (Result >= 0)
                Result = Result + EncodeTime(tm->tm_hour, tm->tm_min, tm->tm_sec, msec);
            else
                Result = Result - EncodeTime(tm->tm_hour, tm->tm_min, tm->tm_sec, msec);

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime Now() {

            struct timeval TV = {0};
            struct tm UT = {0};

            gettimeofday(&TV, nullptr);

            localtime_r(&TV.tv_sec, &UT);

            return SystemTimeToDateTime(&UT, (int) (TV.tv_usec / 1000));
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI unsigned long MsEpoch() {
            struct timeval tv{0};
            gettimeofday(&tv, nullptr);
            return tv.tv_sec * 1000 + tv.tv_usec / 1000;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI time_t FileAge(LPCTSTR lpszFileName) {
            struct stat sb = {0};
            if (stat(lpszFileName, &sb) == 0)
                return sb.st_mtime;
            return -1;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool DirectoryExists(LPCTSTR lpszDirectory){
            struct stat sb = {0};
            if (stat(lpszDirectory, &sb) == 0)
                return S_ISDIR(sb.st_mode);
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool FileExists(LPCTSTR lpszFileName, bool FollowLink) {
            struct stat sb = {0};

            if (lstat(lpszFileName, &sb) != 0)
                return false;//throw EOSError(errno, _T("Call lstat error: "));

            if (S_ISLNK(sb.st_mode)) {
                if (!FollowLink) {
                    return true;
                } else {
                    if (stat(lpszFileName, &sb) == 0) {
                        return !S_ISDIR(sb.st_mode);
                    }
                    return false;
                }
            }

            return !S_ISDIR(sb.st_mode);
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double Round(double x) {
            return (x > 0.0) ? floor(x + 0.5) : ceil(x - 0.5);
        }
        //--------------------------------------------------------------------------------------------------------------


        LIB_DELPHI char *ExtractFileDir(char *Dest, LPCSTR Src) {
            char *str = strcpy(Dest, Src);
            size_t Pos = 0;
            while (*str)
                if (*(++str) == '/')
                    Pos = str - Dest;
            Dest[Pos] = 0;
            return Dest;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI char *ExtractFilePath(char *Dest, LPCSTR Src) {
            char *str = strcpy(Dest, Src);
            size_t Pos = 0;
            while (*str)
                if (*(++str) == '/')
                    Pos = str - Dest;
            Dest[Pos + 1] = 0;
            return Dest;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI char *ExtractFileName(char *Dest, LPCSTR Src) {
            char *str = strcpy(Dest, Src);
            while (*str)
                if (*str++ == '/')
                    Dest = str;
            return Dest;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI char *ExtractFileExt(char *Dest, LPCSTR Src) {
            char *str = strcpy(Dest, Src);
            while (*str)
                if (*(++str) == '.')
                    Dest = str;
            return Dest;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI char *ChangeFileExt(char *Dest, LPCSTR Src, LPCSTR Ext) {
            char *str = strcpy(Dest, Src);
            size_t Pos = 0;
            while (*str)
                if (*(++str) == '.')
                    Pos = str - Dest;
            Dest[Pos] = 0;
            return strcat(Dest, Ext);
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI unsigned long HexToDecA(LPCSTR pszHex)
        {
            return strtoul(pszHex, nullptr, 16);
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI unsigned long HexToDecW(LPCWSTR pszHex)
        {
            return wcstoul(pszHex, nullptr, 16);
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void ByteToHexStrA(LPSTR pszDest, size_t cchDest, LPCBYTE pbSrc, size_t cbSrc, char chDelim)
        {
            int Delta = (chDelim == 0 ? 2 : 3);
            for ( size_t i = 0; i < cbSrc; ++i )
                chVERIFY(SUCCEEDED(StringCchPrintfA(pszDest + i * Delta, cchDest - i * Delta, "%02X%c", pbSrc[i], chDelim)));
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void ByteToHexStrW(LPWSTR pszDest, size_t cchDest, LPCBYTE pbSrc, size_t cbSrc, char chDelim)
        {
            int Delta = (chDelim == 0 ? 2 : 3);
            for ( size_t i = 0; i < cbSrc; ++i )
                chVERIFY(SUCCEEDED(StringCchPrintfW(pszDest + i * Delta, cchDest - i * Delta, L"%02X%c", pbSrc[i], chDelim)));
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void HexStrToByteA(LPBYTE pbDest, size_t cbDest, LPCSTR pszSrc, size_t cchSrc, char chDelim)
        {
            size_t Pos = 0;
            size_t Delta = (chDelim == 0 ? 2 : 3);

            auto pszHex = new CHAR[Delta + 1];
            ::SecureZeroMemory(pszHex, Delta + 1);

            while ( cchSrc > 0 && *pszSrc != 0 )
            {
                ::CopyMemory(pszHex, pszSrc, Delta);
                if ( Pos < cbDest )
                    pbDest[Pos] = (BYTE) HexToDecA(pszHex);

                pszSrc += Delta;
                cchSrc += 2;
                Pos++;
            }

            delete [] pszHex;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void HexStrToByteW(LPBYTE pbDest, size_t cbDest, LPCWSTR pszSrc, size_t cchSrc, char chDelim)
        {
            size_t Pos = 0;
            int Delta = (chDelim == 0 ? 2 : 3);

            auto pszHex = new WCHAR[Delta + 1];
            ::SecureZeroMemory(pszHex, (Delta + 1) * sizeof(WCHAR));

            while ( cchSrc > 0 && *pszSrc != 0 )
            {
                ::CopyMemory(pszHex, pszSrc, Delta * sizeof(WCHAR));
                if ( Pos < cbDest )
                    pbDest[Pos] = (BYTE) HexToDecW(pszHex);

                pszSrc += Delta;
                cchSrc += 2;
                Pos++;
            }

            delete [] pszHex;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool WideToChar(const wchar_t *WideCharSrc, char *CharDst, int cbMultiByte, bool OEM) {
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool CharToWide(LPCSTR CharSrc, wchar_t *WideCharDst, int cbMultiByte, bool OEM) {
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool Even(unsigned int X) {
            return (X % 2) == 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool Odd(unsigned int X) {
            return !Even(X);
        }
        //--------------------------------------------------------------------------------------------------------------

        inline void ErrorStrToIntA(long int val, LPCSTR S) {
            if (val == 0)
                throw ExceptionFrm(SInvalidInteger, S);
            if (((val == LONG_MIN) || (val == LONG_MAX)) && errno == ERANGE)
                throw EOSError(errno, SOutOfTheRange, S);
        }
        //--------------------------------------------------------------------------------------------------------------

        inline void ErrorStrToIntW(long int val, LPCWSTR S) {
            if (val == 0)
                throw ExceptionFrm(SInvalidInteger, S);
            if (((val == LONG_MIN) || (val == LONG_MAX)) && errno == ERANGE)
                throw EOSError(errno, SOutOfTheRange, S);
        }
        //--------------------------------------------------------------------------------------------------------------

        inline void ErrorStrToFloatA(double val, LPCSTR S) {
            if (val == 0.0)
                throw ExceptionFrm(SInvalidDouble, S);
            if (((val == LONG_MIN) || (val == LONG_MAX)) && errno == ERANGE)
                throw EOSError(errno, SOutOfTheRange, S);
        }
        //--------------------------------------------------------------------------------------------------------------

        inline void ErrorStrToFloatW(double val, LPCWSTR S) {
            if (val == 0.0)
                throw ExceptionFrm(SInvalidDouble, S);
            if (((val == LONG_MIN) || (val == LONG_MAX)) && errno == ERANGE)
                throw EOSError(errno, SOutOfTheRange, S);
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, long int *val) {
            char *temp;
            bool Result = true;
            errno = 0;
            *val = strtol(S, &temp, 0);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValW(LPCWSTR S, long int *val) {
            wchar_t *temp;
            bool Result = true;
            errno = 0;
            *val = wcstol(S, &temp, 0);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, float *val) {
            char *temp;
            bool Result = true;
            errno = 0;
            *val = strtof(S, &temp);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValW(LPCWSTR S, float *val) {
            wchar_t *temp;
            bool Result = true;
            errno = 0;
            *val = wcstof(S, &temp);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, double *val) {
            char *temp;
            bool Result = true;
            errno = 0;
            *val = strtod(S, &temp);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValW(LPCWSTR S, double *val) {
            wchar_t *temp;
            bool Result = true;
            errno = 0;
            *val = wcstod(S, &temp);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, long double *val) {
            char *temp;
            bool Result = true;
            errno = 0;
            *val = strtold(S, &temp);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValW(LPCWSTR S, long double *val) {
            wchar_t *temp;
            bool Result = true;
            errno = 0;
            *val = wcstold(S, &temp);
            if (temp == S || *temp != '\0' ||
                ((*val == LONG_MIN || *val == LONG_MAX) && errno == ERANGE))
                Result = false;
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long int StrToIntA(LPCSTR S) {
            long int val;
            if (!ValA(S, &val))
                ErrorStrToIntA(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long int StrToIntW(LPCWSTR S) {
            long int val;
            if (!ValW(S, &val))
                ErrorStrToIntW(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long int StrToIntDefA(LPCSTR S, long int Default) {
            long int val;
            if (!ValA(S, &val))
                return Default;
            return val;
        };
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long int StrToIntDefW(LPCWSTR S, long int Default) {
            long int val;
            if (!ValW(S, &val))
                return Default;
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPSTR IntToStrA(int Value, LPSTR Str, size_t Size, int Base) {
            switch (Base) {
                case 8:
                    snprintf(Str, Size, "%o", Value);
                    break;
                case 16:
                    snprintf(Str, Size, "%x", Value);
                    break;

                default:
                    snprintf(Str, Size, "%d", Value);
                    break;
            }

            return Str;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPWSTR IntToStrW(int Value, LPWSTR Str, size_t Size, int Base) {
            switch (Base) {
                case 8:
                    swprintf(Str, Size, L"%o", Value);
                    break;

                case 16:
                    swprintf(Str, Size, L"%x", Value);
                    break;

                default:
                    swprintf(Str, Size, L"%d", Value);
                    break;
            }

            return Str;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPSTR FloatToStrA(double Value, LPSTR Str, size_t Size, LPCSTR Format) {
            snprintf(Str, Size, Format, Value);
            return Str;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPWSTR FloatToStrW(double Value, LPWSTR Str, size_t Size, LPCWSTR Format) {
            swprintf(Str, Size, Format, Value);
            return Str;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI float StrToFloatA(LPCSTR S) {
            float val;
            if (!ValA(S, &val))
                ErrorStrToFloatA(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI float StrToFloatW(LPCWSTR S) {
            float val;
            if (!ValW(S, &val))
                ErrorStrToFloatW(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI float StrToFloatDefA(LPCSTR S, float Default) {
            float val;
            if (!ValA(S, &val))
                return Default;
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI float StrToFloatDefW(LPCWSTR S, float Default) {
            float val;
            if (!ValW(S, &val))
                return Default;
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double StrToDoubleA(LPCSTR S) {
            double val;
            if (!ValA(S, &val))
                ErrorStrToFloatA(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double StrToDoubleW(LPCWSTR S) {
            double val;
            if (!ValW(S, &val))
                ErrorStrToFloatW(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double StrToDoubleDefA(LPCSTR S, double Default) {
            double val;
            if (!ValA(S, &val))
                return Default;
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double StrToDoubleDefW(LPCWSTR S, double Default) {
            double val;
            if (!ValW(S, &val))
                return Default;
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long double StrToDecimalA(LPCSTR S) {
            long double val;
            if (!ValA(S, &val))
                ErrorStrToFloatA(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long double StrToDecimalW(LPCWSTR S) {
            long double val;
            if (!ValW(S, &val))
                ErrorStrToFloatW(val, S);
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long double StrToDecimalDefA(LPCSTR S, long double Default) {
            long double val;
            if (!ValA(S, &val))
                return Default;
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long double StrToDecimalDefW(LPCWSTR S, long double Default) {
            long double val;
            if (!ValW(S, &val))
                return Default;
            return val;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPSTR DateTimeToStrA(CDateTime Value, LPSTR Str, size_t Size, LPCSTR Format) {
            struct tm TM = {0};
            int MSec;
            DecodeDate(Value, TM.tm_year, TM.tm_mon, TM.tm_mday);
            DecodeTime(Value, TM.tm_hour, TM.tm_min, TM.tm_sec, MSec);
            TM.tm_isdst = -1;
            TM.tm_year -= 1900;
            TM.tm_mon -= 1;
            strftime(Str, Size, Format, &TM);
            return Str;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPWSTR DateTimeToStrW(CDateTime Value, LPWSTR Str, size_t Size, LPCWSTR Format) {
            struct tm TM = {0};
            int MSec;
            DecodeDate(Value, TM.tm_year, TM.tm_mon, TM.tm_mday);
            DecodeTime(Value, TM.tm_hour, TM.tm_min, TM.tm_sec, MSec);
            TM.tm_isdst = -1;
            TM.tm_year -= 1900;
            TM.tm_mon -= 1;
            wcsftime(Str, Size, Format, &TM);
            return Str;
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime StrToDateTimeA(LPCSTR S, LPCSTR Format) {
            struct tm TM = {0};
            struct timespec TS = {0};
            if (sscanf(S, Format, &TM.tm_year, &TM.tm_mon, &TM.tm_mday, &TM.tm_hour, &TM.tm_min, &TM.tm_sec) == EOF)
                throw ExceptionFrm(SInvalidDateTime, S, Format);
            TM.tm_isdst = -1;
            TM.tm_year -= 1900;
            TM.tm_mon -= 1;
            return SystemTimeToDateTime(&TM, &TS);
        }
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime StrToDateTimeW(LPCWSTR S, LPCWSTR Format) {
            struct tm TM = {0};
            struct timespec TS = {0};
            TM.tm_isdst = -1;
            if (swscanf(S, Format, &TM.tm_year, &TM.tm_mon, &TM.tm_mday, &TM.tm_hour, &TM.tm_min, &TM.tm_sec) == EOF)
                throw ExceptionFrm(SInvalidDateTime, S, Format);
            TM.tm_isdst = -1;
            TM.tm_year -= 1900;
            TM.tm_mon -= 1;
            return SystemTimeToDateTime(&TM, &TS);
        }
        //--------------------------------------------------------------------------------------------------------------

        CDateTime StrToDateTimeDefA(LPCSTR S, CDateTime Default, LPCSTR Format) {
            struct tm TM = {0};
            struct timespec TS = {0};
            TM.tm_isdst = -1;
            if (sscanf(S, Format, &TM.tm_year, &TM.tm_mon, &TM.tm_mday, &TM.tm_hour, &TM.tm_min, &TM.tm_sec) == EOF)
                return Default;
            TM.tm_isdst = -1;
            TM.tm_year -= 1900;
            TM.tm_mon -= 1;
            return SystemTimeToDateTime(&TM, &TS);
        }
        //--------------------------------------------------------------------------------------------------------------

        CDateTime StrToDateTimeDefW(LPCWSTR S, CDateTime Default, LPCWSTR Format) {
            struct tm TM = {0};
            struct timespec TS = {0};
            TM.tm_isdst = -1;
            if (swscanf(S, Format, &TM.tm_year, &TM.tm_mon, &TM.tm_mday, &TM.tm_hour, &TM.tm_min, &TM.tm_sec) == EOF)
                return Default;
            TM.tm_isdst = -1;
            TM.tm_year -= 1900;
            TM.tm_mon -= 1;
            return SystemTimeToDateTime(&TM, &TS);
        }
    }
}
}