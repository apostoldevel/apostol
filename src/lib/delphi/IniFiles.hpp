/*++

Library name:

  libdelphi

Module Name:

  IniFiles.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_INIFILES_HPP
#define DELPHI_INIFILES_HPP

#define ARRAY_BOOLEAN_STRINGS { _T("false"), _T("true"), _T("0"), _T("1"), _T("no"), _T("yes"), _T("off"), _T("on") }

extern "C++" {

namespace Delphi {

    namespace IniFiles {

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomIniFile --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (Pointer Sender, LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszValue,
                LPCTSTR lpszDefault, int Line)> COnIniFileParseError;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCustomIniFile: public CObject {
        private:

            CString m_pszFileName;

            COnIniFileParseError m_OnIniFileParseError;

        protected:

            class CKeyLine: public CObject {
            private:
                int m_Line;
            public:
                explicit CKeyLine(int Line): CObject(), m_Line(Line) {};
                int Line() const noexcept { return m_Line; };
            };

            const TCHAR SectionNameSeparator = '\\';

            virtual void InternalReadSections(LPCTSTR lpszSectionName, CStrings *Strings,
                                              bool SubSectionNamesOnly, bool Recurse);

            virtual int GetKeyLine(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName) abstract;

            void DoIniFileParseError(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszValue, LPCTSTR lpszDefault);

        public:

            explicit CCustomIniFile(LPCTSTR lpszFileName);

            ~CCustomIniFile() override = default;

            bool SectionExists(LPCTSTR lpszSectionName);
            virtual bool ValueExists(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName);

            virtual CString ReadString(const CString &SectionName, const CString &KeyName, const CString &Default) abstract;

            virtual CString ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault) abstract;

            virtual void ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault,
                                CString &ReturnedString) abstract;

            virtual DWORD ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault,
                    LPTSTR lpszReturnedString, DWORD nSize) abstract;

            virtual BOOL WriteString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszString) abstract;
            virtual BOOL WriteString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, const CString &String) abstract;
            virtual BOOL WriteString(const CString &SectionName, const CString &KeyName, const CString &String) abstract;

            virtual INT ReadInteger(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, INT Default);
            virtual BOOL WriteInteger(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, UINT Value);

            virtual bool ReadBool(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, bool Default);
            virtual BOOL WriteBool(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, bool Value);

            virtual DOUBLE ReadFloat(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, DOUBLE Default);
            virtual BOOL WriteFloat(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, DOUBLE Value);

            virtual CDateTime ReadDateTime(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, CDateTime Default);
            virtual BOOL WriteDateTime(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, CDateTime Value);

            virtual void ReadSection(LPCTSTR lpszSectionName, CStrings *Strings) abstract;

            virtual void ReadSections(CStrings *Strings) abstract;
            virtual void ReadSections(LPCTSTR lpszSectionName, CStrings *Strings);
            virtual void ReadSubSections(LPCTSTR lpszSectionName, CStrings *Strings, bool Recurse);

            virtual void ReadSectionValues(LPCTSTR lpszSectionName, CStrings *Strings) abstract;

            virtual BOOL EraseSection(LPCTSTR lpszSectionName) abstract;
            virtual BOOL EraseIniFile() abstract;

            virtual void DeleteKey(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName) abstract;

            virtual void UpdateFile() abstract;

            const CString &FileName() { return m_pszFileName; };
            void FileName(LPCTSTR Value) { m_pszFileName = Value; };

            void OnIniFileParseError(COnIniFileParseError && Value) { m_OnIniFileParseError = Value; };

        }; // class CCustomIniFile

        //--------------------------------------------------------------------------------------------------------------

        //- CStringHash ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CStringHash;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CStringHash: public CObject {
            typedef CObject inherited;

        private:

            size_t m_BucketSize;

            CList **m_Buckets;

            void ClearList(CList *List);

        protected:

            struct CHashItem {
            public:
                CString Key;
                int Value;

                CHashItem& operator= (const CHashItem& Item) = default;
            };

            int Find(const CString &Key, CHashItem* &AItem);
            int Find(LPCTSTR Key, CHashItem* &AItem);

            virtual unsigned HashOf(const CString &Key);
            virtual unsigned HashOf(LPCTSTR Key);

        public:

            explicit CStringHash(size_t Size = 256);

            ~CStringHash() override;

            inline static class CStringHash* Create(size_t Size = 256) { return new CStringHash(Size); };

            void Add(const CString &Key, int Value);
            void Add(LPCTSTR Key, int Value);

            void Clear();

            void Remove(const CString &Key);
            void Remove(LPCTSTR Key);

            bool Modify(const CString &Key, int Value);
            bool Modify(LPCTSTR Key, int Value);

            int ValueOf(const CString &Key);
            int ValueOf(LPCTSTR Key);
        };

        //--------------------------------------------------------------------------------------------------------------

        //- CHashedStringList ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CHashedStringList;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CHashedStringList : public CStringList {
            typedef CStringList inherited;

        private:

            CStringHash *m_ValueHash;
            CStringHash *m_NameHash;

            bool m_ValueHashValid;
            bool m_NameHashValid;

            void UpdateValueHash();

            void UpdateNameHash();

        public:

            CHashedStringList(): CStringList() {
                m_ValueHash = nullptr;
                m_NameHash = nullptr;

                m_ValueHashValid = false;
                m_NameHashValid = false;
            };

            explicit CHashedStringList(bool OwnsObjects): CStringList(OwnsObjects) {
                m_ValueHash = nullptr;
                m_NameHash = nullptr;

                m_ValueHashValid = false;
                m_NameHashValid = false;
            };

            static class CHashedStringList* Create(bool OwnsObjects = false) {
                return new CHashedStringList(OwnsObjects);
            };

            ~CHashedStringList() override;

            int IndexOf(const CString &S) override;
            int IndexOf(LPCTSTR S) override;

            int IndexOfName(const CString &Name) override;
            int IndexOfName(LPCTSTR Name) override;

            void Changed();
        };

        //--------------------------------------------------------------------------------------------------------------

        //- CMemIniFile ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CMemIniFile;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CMemIniFile : public CCustomIniFile {
            typedef CCustomIniFile inherited;

        private:

            CStrings *m_Sections;

            bool m_Modified;
            bool m_AutoSave;
            bool m_CaseSensitive;

            CStrings *AddSection(LPCTSTR lpszSectionName);

            bool GetCaseSensitive();
            void SetCaseSensitive(bool Value);

            bool GetModified() { return m_Modified; };
            void SetModified(bool Value) { m_Modified = Value; };

            bool GetAutoSave() { return m_AutoSave; };
            void SetAutoSave(bool Value) { m_AutoSave = Value; };

        protected:

            void LoadValues();

            int GetKeyLine(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName) override;

        public:

            explicit CMemIniFile(LPCTSTR lpszFileName);

            ~CMemIniFile() override;

            CString ReadString(const CString &SectionName, const CString &KeyName, const CString &Default) override;

            CString ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault) override;

            void ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault,
                    CString &ReturnedString) override;

            DWORD ReadString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszDefault,
                             LPTSTR lpszReturnedString, DWORD nSize) override;

            BOOL WriteString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, LPCTSTR lpszString) override;
            BOOL WriteString(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName, const CString &String) override;
            BOOL WriteString(const CString &SectionName, const CString &KeyName, const CString &String) override;

            void Clear();

            void DeleteKey(LPCTSTR lpszSectionName, LPCTSTR lpszKeyName) override;

            BOOL EraseSection(LPCTSTR lpszSectionName) override;

            void GetStrings(CStrings *List);

            void ReadSection(LPCTSTR lpszSectionName, CStrings *Strings) override;

            void ReadSections(CStrings *Strings) override;

            void ReadSectionValues(LPCTSTR lpszSectionName, CStrings *Strings) override;

            void Rename(LPCTSTR lpszFileName, bool Reload);

            void SetStrings(CStrings *List);

            void UpdateFile() override;

            bool CaseSensitive() { return GetCaseSensitive(); };
            void CaseSensitive(bool Value) { SetCaseSensitive(Value); };

            bool Modified() { return GetModified(); };
            void Modified(bool Value) { SetModified(Value); };

            bool AutoSave() { return GetAutoSave(); };
            void AutoSave(bool Value) { SetAutoSave(Value); };

            inline void ReadSections(LPCTSTR lpszSectionName, CStrings *Strings) override {
                CCustomIniFile::ReadSections(lpszSectionName, Strings);
            }

            CStrings &Sections(int Index);
            const CStrings &Sections(int Index) const;

            CStrings &Sections(LPCTSTR Section);
            const CStrings &Sections(LPCTSTR Section) const;

            virtual CMemIniFile &operator=(const CStrings &Strings) {
                if (Strings.Count() > 0) {
                    SetStrings((CStrings *) &Strings);
                }
                return *this;
            }

            CStrings &operator[] (int Index) { return Sections(Index); }
            const CStrings &operator[] (int Index) const { return Sections(Index); }

            const CStrings &operator[] (LPCTSTR Section) const { return Sections(Section); }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CIniFile --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CIniFile : public CMemIniFile {
            typedef CMemIniFile inherited;

        public:

            explicit CIniFile(LPCTSTR lpszFileName): CMemIniFile(lpszFileName) {};

            ~CIniFile() override = default;

            inline static class CIniFile* Create(LPCTSTR lpszFileName) { return new CIniFile(lpszFileName); };

            BOOL EraseIniFile() override;

        };
    }
}

using namespace Delphi::IniFiles;
}


#endif //DELPHI_INIFILES_HPP
