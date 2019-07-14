/*++

Library name:

  libdelphi

Module Name:

  Variant.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_VARIANT_HPP
#define DELPHI_VARIANT_HPP

#define varInteger              VarRec.VInteger
#define varBoolean              VarRec.VBoolean
#define varChar                 VarRec.VChar
#define varDouble               VarRec.VDouble
#define varString               VarRec.VString
#define varPointer              VarRec.VPointer
#define varPChar                VarRec.VPChar
#define varObject               VarRec.VObject
#define varUnsigned             VarRec.VUnsigned
#define varWideChar             VarRec.VWideChar
#define varPWideChar            VarRec.VPWideChar
#define varAnsiString           VarRec.VAnsiString
#define varFloat                VarRec.VFloat
#define varVariant              VarRec.VVariant
#define varUInt64               VarRec.VUInt64
#define varWideString           VarRec.VWideString
#define varInt64                VarRec.VInt64
#define varUnicodeString        VarRec.VUnicodeString

#ifdef UNICODE
#define varStr                  VarRec.VWideString
#define varCString              VarRec.VUnicodeString
#else
#define varStr                  VarRec.VAnsiString
#define varCString              VarRec.VString
#endif

#define vasInteger               varInteger
#define vasBoolean               varBoolean
#define vasChar                  varChar
#define vasDouble                varDouble
#define vasString                varString
#define vasPointer               varPointer
#define vasPChar                 varPChar
#define vasObject                varObject
#define vasUnsigned              varUnsigned
#define vasWideChar              varWideChar
#define vasPWideChar             varPWideChar
#define vasAnsiString            varAnsiString
#define vasFloat                 varFloat
#define vasVariant               varVariant
#define vasUInt64                varUInt64
#define vasWideString            varWideString
#define vasInt64                 varInt64
#define vasUnicodeString         varUnicodeString

#define vasStr                   varStr
#define vasCString               varCString

extern "C++" {

namespace Delphi {

    namespace Variant {

        typedef enum VarType {
            vtEmpty         = -1,
            vtInteger       = 0,
            vtBoolean       = 1,
            vtChar          = 2,
            vtDouble        = 3,
            vtString        = 4,
            vtPointer       = 5,
            vtPChar         = 6,
            vtObject        = 7,
            vtUnsigned      = 8,
            vtWideChar      = 9,
            vtPWideChar     = 10,
            vtAnsiString    = 11,
            vtFloat         = 12,
            vtVariant       = 13,
            vtUInt64        = 14,
            vtWideString    = 15,
            vtInt64         = 16,
            vtUnicodeString = 17
        } CVarType;

        typedef struct VarData {

            CVarType VType;

            union VarRec {

                int VInteger;
                bool VBoolean;
                char VChar;
                double VDouble;
                CString *VString;
                Pointer VPointer;
                char *VPChar;
                CObject *VObject;
                unsigned VUnsigned;
                wchar_t VWideChar;
                wchar_t *VPWideChar;
                const char *VAnsiString;
                float VFloat;
                VarData *VVariant;
                u_int64_t VUInt64;
                const wchar_t *VWideString;
                int64_t VInt64;
                CString *VUnicodeString;

            } VarRec;

            VarData(): VarRec({0}) {

                VType = vtEmpty;

                varInteger = 0;
                varBoolean = false;
                varChar = '\0';
                varDouble = 0.0;
                varString = nullptr;
                varPointer = nullptr;
                varPChar = nullptr;
                varObject = nullptr;
                varUnsigned = 0;
                varWideChar = L'\0';
                varPWideChar = nullptr;
                varAnsiString = nullptr;
                varFloat = 0;
                varVariant = nullptr;
                varUInt64 = 0;
                varWideString = nullptr;
                varInt64 = 0;
                varUnicodeString = nullptr;
            }

            VarData(const VarData& Value): VarData() {
                VType = Value.VType;
                VarRec = Value.VarRec;
            }

            explicit VarData(nullptr_t Value): VarData() {
                VType = vtPointer;
                varPointer = Value;
            }

            explicit VarData(int Value): VarData() {
                VType = vtInteger;
                varInteger = Value;
            }

            explicit VarData(bool Value): VarData() {
                VType = vtBoolean;
                varBoolean = Value;
            }

            explicit VarData(char Value): VarData() {
                VType = vtChar;
                varChar = Value;
            }

            explicit VarData(double Value): VarData() {
                VType = vtDouble;
                varDouble = Value;
            }

            explicit VarData(CString *Value): VarData() {
                VType = vtString;
                varString = Value;
            }

            explicit VarData(Pointer Value): VarData() {
                VType = vtPointer;
                varPointer = Value;
            }

            explicit VarData(char *Value): VarData() {
                VType = vtPChar;
                varPChar = Value;
            }

            explicit VarData(CObject *Value): VarData() {
                VType = vtObject;
                varObject = Value;
            }

            explicit VarData(unsigned Value): VarData() {
                VType = vtUnsigned;
                varUnsigned = Value;
            }

            explicit VarData(wchar_t Value): VarData() {
                VType = vtWideChar;
                varWideChar = Value;
            }

            explicit VarData(wchar_t *Value): VarData() {
                VType = vtPWideChar;
                varPWideChar = Value;
            }

            explicit VarData(const char *Value): VarData() {
                VType = vtAnsiString;
                varAnsiString = Value;
            }

            explicit VarData(float Value): VarData() {
                VType = vtFloat;
                varFloat = Value;
            }

            explicit VarData(VarData *Value): VarData() {
                VType = vtVariant;
                varVariant = Value;
            }

            explicit VarData(u_int64_t Value): VarData() {
                VType = vtUInt64;
                varUInt64 = Value;
            }

            explicit VarData(const wchar_t *Value): VarData() {
                VType = vtWideString;
                varWideString = Value;
            }

            explicit VarData(int64_t Value): VarData() {
                VType = vtInt64;
                varInt64 = Value;
            }

            VarData& operator= (nullptr_t Value) {
                VType = vtPointer;
                varPointer = Value;
                return *this;
            }

            VarData& operator= (int Value) {
                VType = vtInteger;
                varInteger = Value;
                return *this;
            }

            VarData& operator= (bool Value) {
                VType = vtBoolean;
                varBoolean = Value;
                return *this;
            }

            VarData& operator= (char Value) {
                VType = vtChar;
                varChar = Value;
                return *this;
            }

            VarData& operator= (double Value) {
                VType = vtDouble;
                varDouble = Value;
                return *this;
            }

            VarData& operator= (CString *Value) {
#ifdef UNICODE
                VType = vtUnicodeString;
#else
                VType = vtString;
#endif
                varCString = Value;
                return *this;
            }

            VarData& operator= (Pointer Value) {
                VType = vtPointer;
                varPointer = Value;
                return *this;
            }

            VarData& operator= (char *Value) {
                VType = vtPChar;
                varPChar = Value;
                return *this;
            }

            VarData& operator= (CObject *Value) {
                VType = vtObject;
                varObject = Value;
                return *this;
            }

            VarData& operator= (unsigned Value) {
                VType = vtUnsigned;
                varUnsigned = Value;
                return *this;
            }

            VarData& operator= (wchar_t Value) {
                VType = vtWideChar;
                varWideChar = Value;
                return *this;
            }

            VarData& operator= (wchar_t *Value) {
                VType = vtPWideChar;
                varPWideChar = Value;
                return *this;
            }

            VarData& operator= (LPCSTR Value) {
                VType = vtAnsiString;
                varAnsiString = Value;
                return *this;
            }

            VarData& operator= (float Value) {
                VType = vtFloat;
                varFloat = Value;
                return *this;
            }

            VarData& operator= (VarData *Value) {
                VType = vtVariant;
                varVariant = Value;
                return *this;
            }

            VarData& operator= (u_int64_t Value) {
                VType = vtUInt64;
                varUInt64 = Value;
                return *this;
            }

            VarData& operator= (const wchar_t *Value) {
                VType = vtWideString;
                varWideString = Value;
                return *this;
            }

            VarData& operator= (int64_t Value) {
                VType = vtInt64;
                varInt64 = Value;
                return *this;
            }

            VarData& operator= (const VarData& Value) {
                if (this != &Value) {
                    VType = Value.VType;
                    VarRec = Value.VarRec;
                }
                return *this;
            };

        } CVariant, *PVariant;
    }
}

using namespace Delphi::Variant;
}

#endif //DELPHI_VARIANT_HPP
