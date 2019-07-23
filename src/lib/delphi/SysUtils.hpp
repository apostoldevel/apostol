/*++

Library name:

  libdelphi

Module Name:

  SysUtils.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_SYSUTILS_HPP
#define DELPHI_SYSUTILS_HPP

#define HoursPerDay      24
#define MinsPerHour      60
#define SecsPerMin       60
#define MSecsPerSec      1000

#define MinsPerDay       (HoursPerDay * MinsPerHour)
#define SecsPerDay       (MinsPerDay * SecsPerMin)
#define MSecsPerDay      (SecsPerDay * MSecsPerSec)

#define DateDelta        693594
#define MaxInt (int)     (2147483647)
//----------------------------------------------------------------------------------------------------------------------

#define SFormatDateTimeA    "%04d/%02d/%02d %02d:%02d:%02d"
#define SFormatDateTimeW    L"%04d/%02d/%02d %02d:%02d:%02d"

#define SInvalidInteger     _T("Invalid conversion string \"%s\" to integer")
#define SInvalidDouble      _T("Invalid conversion string \"%s\" to double")
#define SInvalidDateTime    _T("Invalid conversion string \"%s\" to datetime by format: \"%s\"")
#define SOutOfTheRange      _T("A number is out of range after conversion string \"%s\"")

extern "C++" {

namespace Delphi {

    namespace SysUtils {

        typedef double CDateTime;
        typedef WORD CDayTable[12];
        //typedef CDayTable *PDayTable;
        //--------------------------------------------------------------------------------------------------------------

        typedef struct TimeStamp {
            long int Time;
            long int Date;
        } CTimeStamp;
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool CreateDir(LPCSTR lpPathName, mode_t Mode = 0700);
        LIB_DELPHI bool ForceDirectories(LPCSTR lpPatchName, mode_t Mode = 0700);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void DebugSystemError(LPCTSTR lpFunctionName);
        LIB_DELPHI void DebugMessage(LPCTSTR lpValue, ...);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI size_t MemoryPos(char *ASubStr, char *ABuffer, size_t ASize);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool WideToChar(const wchar_t *WideCharSrc, char *CharDst, int cbMultiByte = 0, bool OEM = false);
        LIB_DELPHI bool CharToWide(LPCSTR CharSrc, wchar_t *WideCharDst, int cbMultiByte = 0, bool OEM = false);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI BOOL OSCheck(BOOL RetVal);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI int AnsiCompareText(LPCSTR S1, LPCSTR S2);
        LIB_DELPHI bool AnsiSameText(LPCSTR S1, LPCSTR S2);

        LIB_DELPHI int WideCompareText(const wchar_t *S1, const wchar_t *S2);
        LIB_DELPHI bool WideSameText(const wchar_t *S1, const wchar_t *S2);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double Round(double x);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool Even(unsigned int X);
        LIB_DELPHI bool Odd(unsigned int X);
        LIB_DELPHI bool IsCtl(int ch);
        LIB_DELPHI bool IsNumeral(int ch);
        LIB_DELPHI bool IsChar(int ch);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void UpperCase(char *Dest, size_t Len, LPCSTR Source);
        LIB_DELPHI void LowerCase(char *Dest, size_t Len, LPCSTR Source);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void DecodeDate(CDateTime DateTime, int &Year, int &Month, int &Day);
        LIB_DELPHI void DecodeTime(CDateTime DateTime, int &Hour, int &Min, int &Sec, int &MSec);
        LIB_DELPHI bool IsLeapYear(int Year);
        LIB_DELPHI bool TryEncodeTime(int Hour, int Min, int Sec, int MSec, CDateTime &Time);
        LIB_DELPHI CDateTime EncodeTime(int Hour, int Min, int Sec, int MSec);
        LIB_DELPHI bool TryEncodeDate(int Year, int Month, int Day, CDateTime &Date);
        LIB_DELPHI CDateTime EncodeDate(int Year, int Month, int Day);
        //LIB_DELPHI CDateTime SystemTimeToDateTime(const struct tm *tm, struct timespec *ts);
        LIB_DELPHI CDateTime SystemTimeToDateTime(const struct tm *tm, int msec);
        LIB_DELPHI CDateTime Now();
        LIB_DELPHI unsigned long MsEpoch();
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI time_t FileAge(LPCTSTR lpszFileName);
        LIB_DELPHI bool DirectoryExists(LPCTSTR lpszDirectory);
        LIB_DELPHI bool FileExists(LPCTSTR lpszFileName, bool FollowLink = true);

        //--------------------------------------------------------------------------------------------------------------
        LIB_DELPHI char *ExtractFileDir(char *Dest, LPCSTR Src);
        LIB_DELPHI char *ExtractFilePath(char *Dest, LPCSTR Src);
        LIB_DELPHI char *ExtractFileName(char *Dest, LPCSTR Src);
        LIB_DELPHI char *ExtractFileExt(char *Dest, LPCSTR Src);
        LIB_DELPHI char *ChangeFileExt(char *Dest, LPCSTR Src, LPCSTR Ext);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI unsigned long HexToDecA(LPCSTR pszHex);
        LIB_DELPHI unsigned long HexToDecW(LPCWSTR pszHex);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void ByteToHexStrA(LPSTR pszDest, size_t cchDest, LPCBYTE pbSrc, size_t cbSrc, char chDelim = 0);
        LIB_DELPHI void ByteToHexStrW(LPWSTR pszDest, size_t cchDest, LPCBYTE pbSrc, size_t cbSrc, char chDelim = 0);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI void HexStrToByteA(LPBYTE pbDest, size_t cbDest, LPCSTR pszSrc, size_t cchSrc, char chDelim = 0);
        LIB_DELPHI void HexStrToByteW(LPBYTE pbDest, size_t cbDest, LPCWSTR pszSrc, size_t cchSrc, char chDelim = 0);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPSTR IntToStrA(int Value, LPSTR Str, size_t Size, int Base = 10);
        LIB_DELPHI LPWSTR IntToStrW(int Value, LPWSTR Str, size_t Size, int Base = 10);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPSTR FloatToStrA(double Value, LPSTR Str, size_t Size, LPCSTR Format = "%f");
        LIB_DELPHI LPWSTR FloatToStrW(double Value, LPWSTR Str, size_t Size, LPCWSTR Format = L"%f");
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, long int *val);
        LIB_DELPHI bool ValW(LPCWSTR S, long int *val);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, float *val);
        LIB_DELPHI bool ValW(LPCWSTR S, float *val);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, double *val);
        LIB_DELPHI bool ValW(LPCWSTR S, double *val);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI bool ValA(LPCSTR S, long double *val);
        LIB_DELPHI bool ValW(LPCWSTR S, long double *val);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long int StrToIntA(LPCSTR S);
        LIB_DELPHI long int StrToIntW(LPCWSTR S);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long int StrToIntDefA(LPCSTR S, long int Default);
        LIB_DELPHI long int StrToIntDefW(LPCWSTR S, long int Default);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI float StrToFloatA(LPCSTR S);
        LIB_DELPHI float StrToFloatW(LPCWSTR S);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI float StrToFloatDefA(LPCSTR S, float Default);
        LIB_DELPHI float StrToFloatDefW(LPCWSTR S, float Default);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double StrToDoubleA(LPCSTR S);
        LIB_DELPHI double StrToDoubleW(LPCWSTR S);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI double StrToDoubleDefA(LPCSTR S, double Default);
        LIB_DELPHI double StrToDoubleDefW(LPCWSTR S, double Default);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long double StrToDecimalA(LPCSTR S);
        LIB_DELPHI long double StrToDecimalW(LPCWSTR S);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI long double StrToDecimalDefA(LPCSTR S, long double Default);
        LIB_DELPHI long double StrToDecimalDefW(LPCWSTR S, long double Default);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI LPSTR DateTimeToStrA(CDateTime Value, LPSTR Str, size_t Size, LPCSTR Format = "%Y-%m-%d %H:%M:%S");
        LIB_DELPHI LPWSTR DateTimeToStrW(CDateTime Value, LPWSTR Str, size_t Size, LPCWSTR Format = L"%Y-%m-%d %H:%M:%S");
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime StrToDateTimeA(LPCSTR S, LPCSTR Format = SFormatDateTimeA);
        LIB_DELPHI CDateTime StrToDateTimeW(LPCWSTR S, LPCWSTR Format = SFormatDateTimeW);
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDateTime StrToDateTimeDefA(LPCSTR S, CDateTime Default, LPCSTR Format = SFormatDateTimeA);
        LIB_DELPHI CDateTime StrToDateTimeDefW(LPCWSTR S, CDateTime Default, LPCWSTR Format = SFormatDateTimeW);
        //--------------------------------------------------------------------------------------------------------------

#ifdef UNICODE

    #define Val                 ValW

    #define IntToStr            IntToStrW
    #define FloatToStr          FloatToStrW

    #define StrToInt            StrToIntW
    #define StrToIntDef         StrToIntDefW

    #define StrToFloat          StrToFloatW
    #define StrToFloatDef       StrToFloatDefW

    #define StrToDouble         StrToDoubleW
    #define StrToDoubleDef      StrToDoubleDefW

    #define StrToDecimal        StrToDecimalW
    #define StrToDecimalDef     StrToDecimalDefW

    #define DateTimeToStr       DateTimeToStrW

    #define StrToDateTime       StrToDateTimeW
    #define StrToDateTimeDef    StrToDateTimeDefW

    #define HexToDec            HexToDecW
    #define ByteToHexStr        ByteToHexStrW
    #define HexStrToByte        HexStrToByteW

    #define CompareText         WideCompareText
    #define SameText            WideSameText

#else

    #define Val                 ValA

    #define IntToStr            IntToStrA
    #define FloatToStr          FloatToStrA

    #define StrToInt            StrToIntA
    #define StrToIntDef         StrToIntDefA

    #define StrToFloat          StrToFloatA
    #define StrToFloatDef       StrToFloatDefA

    #define StrToDouble         StrToDoubleA
    #define StrToDoubleDef      StrToDoubleDefA

    #define StrToDecimal        StrToDecimalA
    #define StrToDecimalDef     StrToDecimalDefA

    #define StrToDateTimeDef    StrToDateTimeDefA

    #define DateTimeToStr       DateTimeToStrA
    #define StrToDateTime       StrToDateTimeA

    #define HexToDec            HexToDecA
    #define ByteToHexStr        ByteToHexStrA
    #define HexStrToByte        HexStrToByteA

    #define CompareText         AnsiCompareText
    #define SameText            AnsiSameText
#endif

    }; // namespace SysUtils
//--------------------------------------------------------------------------------------------------------------------
}; // namespace Delphi

using namespace Delphi::SysUtils;
}
#endif //DELPHI_SYSUTILS_HPP