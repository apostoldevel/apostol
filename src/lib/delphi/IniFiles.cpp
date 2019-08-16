/*++

Library name:

  libdelphi

Module Name:

  IniFiles.cpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "delphi.hpp"
#include "IniFiles.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Delphi {

    namespace IniFiles {

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomIniFile --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCustomIniFile::CCustomIniFile(LPCTSTR lpszFileName): CObject()  {
            m_pszFileName = lpszFileName;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomIniFile::InternalReadSections(LPCTSTR lpszSectionName, CStrings *Strings, bool SubSectionNamesOnly,
                                                  bool Recurse) {
            size_t SLen, SectionLen, SectionEndOfs, I;
            CString SubSectionName;
            CStringList AllSections;

            ReadSections(&AllSections);
            StringCchLength(lpszSectionName, MaxInt, &SectionLen);

            // Adjust end offset of section name to account for separator when present.
            SectionEndOfs = SectionLen > 0 ? SectionLen + 1 : SectionLen;
            Strings->BeginUpdate();
            try {
                for (I = 0; I < AllSections.Count(); ++I) {
                    CString &S = AllSections[I];
                    SLen = S.Length();
                    if ((SectionLen == 0)
                        || (SubSectionNamesOnly && (SLen > SectionLen) &&
                            SameText(lpszSectionName, S.SubString(0, SectionLen).c_str()))
                        || (!SubSectionNamesOnly && (SLen >= SectionLen) &&
                            SameText(lpszSectionName, S.SubString(0, SectionLen).c_str()))) {
                        SubSectionName = S.SubString(SectionEndOfs, SLen + 1 - SectionEndOfs); // +1 to skip '=' ??
                        if (!Recurse && (SubSectionName.Find(SectionNameSeparator) != CString::npos))
                            continue;
                        if (SubSectionNamesOnly)
                            Strings->Add(SubSectionName);
                        else
                            Strings->Add(S.c_str());
                    }
                }
            } catch (...) {
            }
            Strings->EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CCustomIniFile::SectionExists(LPCTSTR lpszSectionName) {
            CStrings *S;
            S = new CStringList;
            try {
                ReadSection(lpszSectionName, S);
            } catch (...) {
            }
            delete S;
            return S->Count() > 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CCustomIniFile::ValueExists(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName) {
            CStrings *S;
            S = new CStringList;
            try {
                ReadSection(lpszSectionName, S);
            } catch (...) {
            }
            delete S;
            return S->IndexOf(lpszSectionName) > -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomIniFile::ReadSections(LPCTSTR lpszSectionName, CStrings *Strings) {
            InternalReadSections(lpszSectionName, Strings, false, true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomIniFile::ReadSubSections(LPCTSTR lpszSectionName, CStrings *Strings, bool Recurse) {
            InternalReadSections(lpszSectionName, Strings, true, Recurse);
        }
        //--------------------------------------------------------------------------------------------------------------

        INT CCustomIniFile::ReadInteger(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, INT Default) {
            long int val;

            TCHAR szString[_INT_T_LEN + 1] = {0};
            TCHAR szDefault[_INT_T_LEN + 1] = {0};

            IntToStr(Default, szDefault, _INT_T_LEN);

            if (ReadString(lpszSectionName, lpszKeyName, szDefault, szString, _INT_T_LEN)) {
                if (Val(szString, &val))
                    return (INT) val;
            }

            DoIniFileParseError(lpszSectionName, lpszKeyName, szString, szDefault);

            return Default;
        }
        //--------------------------------------------------------------------------------------------------------------

        BOOL CCustomIniFile::WriteInteger(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, UINT Value) {
            TCHAR szString[_INT_T_LEN + 1] = {0};
            return WriteString(lpszSectionName, lpszKeyName, IntToStr(Value, szString, _INT_T_LEN));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CCustomIniFile::ReadBool(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, bool Default) {

            LPCTSTR LBoolStr[] = ARRAY_BOOLEAN_STRINGS;

            TCHAR szString[MAX_BUFFER_SIZE / 4 + 1] = {0};
            if (ReadString(lpszSectionName, lpszKeyName, LBoolStr[(int) Default], szString, MAX_BUFFER_SIZE / 4)) {
                for (unsigned int i = 0; i < chARRAY(LBoolStr); ++i) {
                    if (SameText(LBoolStr[i], szString))
                        return Odd(i);
                }
            }

            DoIniFileParseError(lpszSectionName, lpszKeyName, szString, LBoolStr[(int) Default]);

            return Default;
        }
        //--------------------------------------------------------------------------------------------------------------

        BOOL CCustomIniFile::WriteBool(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, bool Value) {
            LPCTSTR LBoolStr[] = ARRAY_BOOLEAN_STRINGS;
            return WriteString(lpszSectionName, lpszKeyName, LBoolStr[(int) Value]);
        }
        //--------------------------------------------------------------------------------------------------------------

        DOUBLE CCustomIniFile::ReadFloat(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, DOUBLE Default) {
            double val;

            TCHAR szString[MAX_BUFFER_SIZE / 4 + 1] = {0};
            TCHAR szDefault[MAX_BUFFER_SIZE / 4  + 1] = {0};

            FloatToStr(Default, szDefault, MAX_BUFFER_SIZE / 4 );

            if (ReadString(lpszSectionName, lpszKeyName, szDefault, szString, MAX_BUFFER_SIZE / 4)) {
                if (Val(szString, &val))
                    return val;
            }

            DoIniFileParseError(lpszSectionName, lpszKeyName, szString, szDefault);

            return Default;
        }
        //--------------------------------------------------------------------------------------------------------------

        BOOL CCustomIniFile::WriteFloat(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, DOUBLE Value) {
            TCHAR szString[MAX_BUFFER_SIZE / 4 + 1] = {0};
            return WriteString(lpszSectionName, lpszKeyName, FloatToStr(Value, szString, MAX_BUFFER_SIZE / 4));
        }
        //--------------------------------------------------------------------------------------------------------------

        CDateTime CCustomIniFile::ReadDateTime(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, CDateTime Default) {
            TCHAR szString[MAX_BUFFER_SIZE / 4 + 1] = {0};
            TCHAR szDefault[MAX_BUFFER_SIZE / 4  + 1] = {0};

            DateTimeToStr(Default, szDefault, MAX_BUFFER_SIZE / 4 );

            if (ReadString(lpszSectionName, lpszKeyName, szDefault, szString, MAX_BUFFER_SIZE / 4 ))
                return StrToDateTimeDef(szString, Default);

            DoIniFileParseError(lpszSectionName, lpszKeyName, szString, szDefault);

            return Default;
        }
        //--------------------------------------------------------------------------------------------------------------

        BOOL CCustomIniFile::WriteDateTime(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, CDateTime Value) {
            TCHAR szString[MAX_BUFFER_SIZE / 4 + 1] = {0};
            return WriteString(lpszSectionName, lpszKeyName, DateTimeToStr(Value, szString, MAX_BUFFER_SIZE / 4));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomIniFile::DoIniFileParseError(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszValue,
                LPCTSTR lpszDefault) {
            if (m_OnIniFileParseError) {
                int Line = GetKeyLine(lpszSectionName, lpszKeyName);
                m_OnIniFileParseError((Pointer) this, lpszSectionName, lpszKeyName, lpszValue, lpszDefault, Line);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CStringHash -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CStringHash::CStringHash(size_t Size): CObject() {
            m_BucketSize = Size;
            m_Buckets = new CList* [Size];
            for (int I = 0; I < Size; ++I)
                m_Buckets[I] = new CList();
        }
        //--------------------------------------------------------------------------------------------------------------

        CStringHash::~CStringHash() {
            Clear();
            for (int I = 0; I < m_BucketSize; ++I) {
                FreeAndNil(m_Buckets[I]);
            }
            delete [] m_Buckets;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStringHash::Find(const CString &Key, CHashItem* &AItem) {
            int Hash, pos = 0;
            CHashItem *Item;
            Hash = HashOf(Key) % (unsigned) m_BucketSize;
            if (m_Buckets[Hash] != nullptr) {
                while (pos < m_Buckets[Hash]->Count()) {
                    Item = (CHashItem *) m_Buckets[Hash]->Items(pos);
                    if (Item->Key == Key) {
                        AItem = Item;
                        return pos;
                    }
                    pos++;
                }
            }
            return -1;
        }
        //-------------------------------------------------------------------------------------------------------------

        int CStringHash::Find(LPCTSTR Key, CHashItem* &AItem) {
            int Hash, pos = 0;
            CHashItem *Item;
            Hash = HashOf(Key) % (unsigned) m_BucketSize;
            if (m_Buckets[Hash] != nullptr) {
                while (pos < m_Buckets[Hash]->Count()) {
                    Item = (CHashItem *) m_Buckets[Hash]->Items(pos);
                    if (Item->Key == Key) {
                        AItem = Item;
                        return pos;
                    }
                    pos++;
                }
            }
            return -1;
        }
        //-------------------------------------------------------------------------------------------------------------

        unsigned CStringHash::HashOf(const CString &Key) {
            unsigned Result = 0;
            for (size_t I = 0; I < Key.Length(); ++I)
                Result = ((Result << 2) | (Result >> (sizeof(Result) * 8 - 2))) ^ Key[I];
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        unsigned CStringHash::HashOf(LPCTSTR Key) {
            unsigned Result = 0;
            size_t Length;
            chVERIFY(SUCCEEDED(StringCchLength(Key, _INT_T_LEN, &Length)));
            for (size_t I = 0; I < Length; ++I)
                Result = ((Result << 2) | (Result >> (sizeof(Result) * 8 - 2))) ^ Key[I];
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringHash::Add(const CString &Key, int Value) {
            int Hash;
            CHashItem *Item;
            Hash = HashOf(Key) % (unsigned) m_BucketSize;
            Item = new CHashItem;
            Item->Key = Key;
            Item->Value = Value;
            m_Buckets[Hash]->Add(Item);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringHash::Add(LPCTSTR Key, int Value) {
            int Hash;
            CHashItem *Item;
            Hash = HashOf(Key) % (unsigned) m_BucketSize;
            Item = new CHashItem;
            Item->Key = Key;
            Item->Value = Value;
            m_Buckets[Hash]->Add(Item);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringHash::ClearList(CList *List) {
            for (int I = List->Count() - 1; I >= 0; --I) {
                delete (CHashItem *) List->Items(I);
            }
            List->Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringHash::Clear() {
            for (int I = 0; I < m_BucketSize; ++I) {
                ClearList(m_Buckets[I]);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringHash::Remove(const CString &Key) {
            int Hash, Pos;
            CHashItem *Item = nullptr;
            Pos = Find(Key, Item);
            if (Pos >= 0) {
                Hash = HashOf(Key) % (unsigned) m_BucketSize;
                m_Buckets[Hash]->Delete(Pos);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringHash::Remove(LPCTSTR Key) {
            int Hash, Pos;
            CHashItem *Item = nullptr;
            Pos = Find(Key, Item);
            if (Pos >= 0) {
                Hash = HashOf(Key) % (unsigned) m_BucketSize;
                m_Buckets[Hash]->Delete(Pos);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CStringHash::Modify(const CString &Key, int Value) {
            int Hash, Pos;
            CHashItem *Item;
            Pos = Find(Key, Item);
            if (Pos >= 0) {
                Hash = HashOf(Key) % (unsigned) m_BucketSize;
                Item->Value = Value;
                m_Buckets[Hash]->Items(Pos, Item);
                return true;
            }
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CStringHash::Modify(LPCTSTR Key, int Value) {
            int Hash, Pos;
            CHashItem *Item;
            Pos = Find(Key, Item);
            if (Pos >= 0) {
                Hash = HashOf(Key) % (unsigned) m_BucketSize;
                Item->Value = Value;
                m_Buckets[Hash]->Items(Pos, Item);
                return true;
            }
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStringHash::ValueOf(const CString &Key) {
            int Pos;
            CHashItem *Item;
            Pos = Find(Key, Item);
            if (Pos >= 0)
                return Item->Value;
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStringHash::ValueOf(LPCTSTR Key) {
            int Pos;
            CHashItem *Item;
            Pos = Find(Key, Item);
            if (Pos >= 0)
                return Item->Value;
            return -1;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CHashedStringList -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CHashedStringList::~CHashedStringList() {
            delete m_ValueHash;
            delete m_NameHash;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CHashedStringList::UpdateValueHash() {
            if (m_ValueHashValid)
                return;

            if (m_ValueHash == nullptr)
                m_ValueHash = CStringHash::Create();
            else
                m_ValueHash->Clear();

            for (int I = 0; I < Count(); ++I)
                m_ValueHash->Add(CStringList::Strings(I), I);

            m_ValueHashValid = true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CHashedStringList::UpdateNameHash() {
            size_t P;

            if (m_NameHashValid)
                return;

            if (m_NameHash == nullptr)
                m_NameHash = CStringHash::Create();
            else
                m_NameHash->Clear();

            for (int I = 0; I < Count(); ++I) {
                CString &Key = Get(I);
                P = Key.Find(NameValueSeparator());
                if (P != CString::npos) {
                    m_NameHash->Add(Key.SubString(0, P), I);
                }
            }

            m_NameHashValid = true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CHashedStringList::Changed() {
            inherited Changed();
            m_ValueHashValid = false;
            m_NameHashValid = false;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CHashedStringList::IndexOf(const CString &S) {
            UpdateValueHash();
            return m_ValueHash->ValueOf(S);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CHashedStringList::IndexOf(LPCTSTR S) {
            UpdateValueHash();
            return m_ValueHash->ValueOf(S);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CHashedStringList::IndexOfName(const CString &Name) {
            UpdateNameHash();
            return m_NameHash->ValueOf(Name);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CHashedStringList::IndexOfName(LPCTSTR Name) {
            UpdateNameHash();
            return m_NameHash->ValueOf(Name);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CMemIniFile -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CMemIniFile::CMemIniFile(LPCTSTR lpszFileName) : CCustomIniFile(lpszFileName) {
            m_Modified = false;
            m_AutoSave = false;
            m_CaseSensitive = true;

            m_Sections = CHashedStringList::Create(true);
            LoadValues();
        }
        //--------------------------------------------------------------------------------------------------------------

        CMemIniFile::~CMemIniFile() {
            if (AutoSave() && Modified())
                UpdateFile();
            delete m_Sections;
        }
        //--------------------------------------------------------------------------------------------------------------

        CStrings *CMemIniFile::AddSection(LPCTSTR lpszSectionName) {
            CHashedStringList *Result = CHashedStringList::Create(true); // OwnsObject = true
            try {
                m_Sections->AddObject(lpszSectionName, Result);
                ((CHashedStringList *) m_Sections)->Changed();
            } catch (...) {
                delete Result;
                throw;
            };
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CMemIniFile::GetCaseSensitive() {
            return m_CaseSensitive; // because Linux
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::LoadValues() {

            CStringList *List;

            try {
                if (!FileName().IsEmpty() && FileExists(FileName().c_str())) {
                    List = CStringList::Create();
                    List->Delimiter('\n');
                    try {
                        List->LoadFromFile(FileName().c_str());
                        SetStrings(List);
                    } catch (...) {
                        delete List;
                        throw;
                    }
                    delete List;
                } else
                    Clear();
            } catch (...) {
                Modified(false);
                throw;
            }
            Modified(false);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::SetCaseSensitive(bool Value) {
            if (m_CaseSensitive != Value)
                m_CaseSensitive = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::Clear() {
            Modified(Modified() || (m_Sections->Count() > 0));
            m_Sections->Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::DeleteKey(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName) {
            int I, J;
            CStrings *Strings;

            I = m_Sections->IndexOf(lpszSectionName);
            if (I >= 0) {
                Strings = (CStrings *) m_Sections->Objects(I);
                J = Strings->IndexOfName(lpszKeyName);
                if (J >= 0) {
                    Strings->Delete(J);
                    Modified(true);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        BOOL CMemIniFile::EraseSection(LPCTSTR lpszSectionName) {
            int I;
            I = m_Sections->IndexOf(lpszSectionName);
            if (I >= 0) {
                m_Sections->Delete(I);
                Modified(true);
                return TRUE;
            }
            return FALSE;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::GetStrings(CStrings *List) {
            List->BeginUpdate();
            try {
                for (int I = 0; I < m_Sections->Count(); ++I) {
                    List->Add(_T("[") + m_Sections->Strings(I) + _T("]"));
                    List->AddStrings(*((CStrings *) m_Sections->Objects(I)));
                    List->Add(_T(""));
                }
            } catch (...) {
                List->EndUpdate();
                throw;
            }
            List->EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::ReadSection(LPCTSTR lpszSectionName, CStrings *Strings) {
            int I, J;
            CStrings *SectionStrings;

            Strings->BeginUpdate();
            try {
                Strings->Clear();
                I = m_Sections->IndexOf(lpszSectionName);
                if (I >= 0) {
                    SectionStrings = (CStrings *) m_Sections->Objects(I);
                    for (J = 0; J < SectionStrings->Count(); ++J)
                        Strings->Add(SectionStrings->Names(J));
                }
            } catch (...) {
                Strings->EndUpdate();
                throw;
            }
            Strings->EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::ReadSections(CStrings *Strings) {
            Strings->SetStrings(*m_Sections);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::ReadSectionValues(LPCTSTR lpszSectionName, CStrings *Strings) {
            int I;

            Strings->BeginUpdate();
            try {
                Strings->Clear();
                I = m_Sections->IndexOf(lpszSectionName);
                if (I >= 0)
                    Strings->SetStrings(*(CStrings *) m_Sections->Objects(I));
            } catch (...) {
                Strings->EndUpdate();
                throw;
            }
            Strings->EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CMemIniFile::ReadString(const CString &SectionName, const CString &KeyName, const CString &Default) {
            int I;
            size_t Length = KeyName.Length();
            CStrings *Strings;

            I = m_Sections->IndexOf(SectionName);
            if (I >= 0) {
                Strings = (CStrings *) m_Sections->Objects(I);
                I = Strings->IndexOfName(KeyName);
                if (I >= 0) {
                    return Strings->Strings(I).SubString(Length + 1);
                }
            }

            return Default;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CMemIniFile::ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault) {
            int I;
            size_t Length;
            CString S(lpszDefault);
            CStrings *Strings;

            chVERIFY(SUCCEEDED(StringCchLength(lpszKeyName, _INT_T_LEN, &Length)));

            I = m_Sections->IndexOf(lpszSectionName);
            if (I >= 0) {
                Strings = (CStrings *) m_Sections->Objects(I);
                I = Strings->IndexOfName(lpszKeyName);
                if (I >= 0) {
                    S = Strings->Strings(I).SubString(Length + 1);
                }
            }

            return S;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault,
                CString &ReturnedString) {
            int I;
            size_t Length;
            CStrings *Strings;

            chVERIFY(SUCCEEDED(StringCchLength(lpszKeyName, _INT_T_LEN, &Length)));

            ReturnedString = lpszDefault;

            I = m_Sections->IndexOf(lpszSectionName);
            if (I >= 0) {
                Strings = (CStrings *) m_Sections->Objects(I);
                I = Strings->IndexOfName(lpszKeyName);
                if (I >= 0) {
                    ReturnedString = Strings->Strings(I).SubString(Length + 1);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        DWORD CMemIniFile::ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault,
                                      LPTSTR lpszReturnedString, DWORD nSize) {
            int I;
            size_t  Length;
            CStrings *Strings;
            CString S;

            chVERIFY(SUCCEEDED(StringCchLength(lpszKeyName, _INT_T_LEN, &Length)));

            I = m_Sections->IndexOf(lpszSectionName);
            if (I >= 0) {
                Strings = (CStrings *) m_Sections->Objects(I);
                I = Strings->IndexOfName(lpszKeyName);
                if (I >= 0) {
                    S = Strings->Strings(I).SubString(Length + 1);
                    nSize = Min(S.Length(), nSize);
                    return S.Copy(lpszReturnedString, nSize);
                }
            }

            chVERIFY(SUCCEEDED(StringCchCopy(lpszReturnedString, nSize, lpszDefault)));

            return nSize;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::Rename(LPCTSTR lpszFileName, bool Reload) {
            Modified(Modified() || (FileName().Compare(lpszFileName) == 0));
            FileName(lpszFileName);
            if (Reload)
                LoadValues();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::SetStrings(CStrings *List) {
            int I;
            size_t J;
            CString S, D;
            CStrings *Strings;

            Clear();
            Strings = nullptr;

            for (I = 0; I < List->Count(); ++I) {
                S = List->Strings(I).Trim();
                if (!S.IsEmpty() && ((S.front() != '#') && (S.front() != ';'))) {
                    if ((S.front() == '[') && (S.back() == ']')) {
                        Strings = AddSection(S.SubString(1, S.Length() - 2).Trim().c_str());
                    } else if (Strings != nullptr) {
                        J = S.Find('=');
                        if (J != CString::npos) {
                            Strings->AddObject(S.SubString(0, J).Trim() + '=' + S.SubString(J + 1).Trim(),
                                               new CKeyLine(I + 1));
                        } else
                            Strings->AddObject(S, new CKeyLine(I + 1));
                    }
                }
            }
            Modified(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMemIniFile::UpdateFile() {
            CStringList *List;
            List = CStringList::Create();
            try {
                GetStrings(List);
                List->SaveToFile(FileName().c_str());
            } catch (...) {
            }
            delete List;
            Modified(false);
        }
        //--------------------------------------------------------------------------------------------------------------

        BOOL CMemIniFile::WriteString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszString) {
            int I;

            CString S;
            CStrings *Strings;

            I = m_Sections->IndexOf(lpszSectionName);
            if (I >= 0) {
                Strings = (CStrings *) m_Sections->Objects(I);
            } else
                Strings = AddSection(lpszSectionName);

            S = lpszKeyName;
            S += "=";
            S += lpszString;

            I = Strings->IndexOfName(lpszKeyName);
            if (I >= 0)
                Strings->Strings(I, S);
            else
                Strings->Add(S);

            Modified(true);

            return TRUE;
        }
        //--------------------------------------------------------------------------------------------------------------

        BOOL CMemIniFile::WriteString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, const CString &String) {
            return WriteString(lpszSectionName, lpszKeyName, String.c_str());
        };
        //--------------------------------------------------------------------------------------------------------------

        BOOL CMemIniFile::WriteString(const CString &SectionName, const CString &KeyName, const CString &String) {
            return WriteString(SectionName.c_str(), KeyName.c_str(), String.c_str());
        };
        //--------------------------------------------------------------------------------------------------------------

        int CMemIniFile::GetKeyLine(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName) {
            int I;
            CKeyLine *KeyLine;
            CStrings *Strings;

            I = m_Sections->IndexOf(lpszSectionName);
            if (I >= 0) {
                Strings = (CStrings *) m_Sections->Objects(I);
                I = Strings->IndexOfName(lpszKeyName);
                if (I >= 0) {
                    KeyLine = (CKeyLine *) Strings->Objects(I);
                    return KeyLine->Line();
                }
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        CStrings &CMemIniFile::Sections(int Index) {
            if ((Index < 0) || (Index >= m_Sections->Count()))
                throw ExceptionFrm(SListIndexError, Index);

            auto Strings = (CHashedStringList *) m_Sections->Objects(Index);
            return *Strings;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CStrings &CMemIniFile::Sections(int Index) const {
            if ((Index < 0) || (Index >= m_Sections->Count()))
                throw ExceptionFrm(SListIndexError, Index);

            auto Strings = (CHashedStringList *) m_Sections->Objects(Index);
            return *Strings;
        }
        //--------------------------------------------------------------------------------------------------------------

        CStrings &CMemIniFile::Sections(LPCTSTR Section) {
            return Sections(m_Sections->IndexOf(Section));
        }
        //--------------------------------------------------------------------------------------------------------------

        const CStrings &CMemIniFile::Sections(LPCTSTR Section) const {
            return Sections(m_Sections->IndexOf(Section));
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CIniFile --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        BOOL CIniFile::EraseIniFile() {
            return FALSE;
        }
    }
}
}

