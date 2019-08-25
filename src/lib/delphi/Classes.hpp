/*++

Library name:

  libdelphi

Module Name:

  Classes.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_CLASSES_HPP
#define DELPHI_CLASSES_HPP

extern "C++" {

namespace Delphi {

    namespace Classes {

        #define MaxBufSize 0xF000
        #define MAX_ERROR_STR 2048
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CObject;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CObject *Sender)> CNotifyEvent;
        //--------------------------------------------------------------------------------------------------------------

        typedef DWORD (*LPTHREAD_START_ROUTINE) (LPVOID lpThreadParameter);
        //--------------------------------------------------------------------------------------------------------------

        typedef LPTHREAD_START_ROUTINE TThreadFunc;
        //--------------------------------------------------------------------------------------------------------------

        class CHeap;
        class CSysError;
        //--------------------------------------------------------------------------------------------------------------

        extern LIB_DELPHI pid_t MainThreadID;
        //--------------------------------------------------------------------------------------------------------------

        class CDefaultLocale;
        //--------------------------------------------------------------------------------------------------------------

        extern LIB_DELPHI CDefaultLocale DefaultLocale;
        //--------------------------------------------------------------------------------------------------------------

        extern LIB_DELPHI CHeap *GHeap;
        extern LIB_DELPHI CSysError *GSysError;
        //--------------------------------------------------------------------------------------------------------------

        typedef unsigned (*PTHREAD_START)(void *);
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CObject {
        public:

            CObject() = default;
            virtual ~CObject() = default;

            inline static class CObject *Create() { return new CObject(); };
            inline virtual void Destroy() { delete this; };

            void Free() { if (this != nullptr) Destroy(); };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CHeap -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CHeap {
        private:

            HANDLE m_hHandle;

            int m_Options;

            size_t m_InitialSize;
            size_t m_MaximumSize;

        protected:

            void Create();
            void Destroy();

        public:

            CHeap();

            virtual ~CHeap();

            inline class CHeap *CreateHeap() { return GHeap = new CHeap(); };

            inline void DestroyHeap() { delete GHeap; };

            void Initialize();

            Pointer Alloc(unsigned long ulFlags, size_t ulSize);

            Pointer ReAlloc(unsigned long ulFlags, Pointer lpMem, size_t ulNewSize, size_t ulOldSize);

            Pointer Free(unsigned long ulFlags, Pointer lpMem, size_t ulSize);

            unsigned long Size(unsigned long ulFlags, Pointer lpMem);

            HANDLE GetHandle() { return m_hHandle; }

            int GetOptions() { return m_Options; }

            void SetOptions(int Value) { m_Options = Value; }

            size_t GetInitialSize() { return m_InitialSize; }

            void SetInitialSize(size_t Value) { m_InitialSize = Value; }

            size_t GetMaximumSize() { return m_MaximumSize; }

            void SetMaximumSize(size_t Value) { m_MaximumSize = Value; }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CDefaultLocale --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CDefaultLocale {
        private:

            locale_t m_Locale;

            LPCSTR m_LocaleName;

            int m_Category;

            int m_CategoryMask;

        public:

            explicit CDefaultLocale(locale_t ALocale = LC_GLOBAL_LOCALE) noexcept;

            explicit CDefaultLocale(LPCSTR Locale);
            ~CDefaultLocale();

            locale_t Locale() { return m_Locale; };

            LPCSTR LocaleName() { return m_LocaleName; };

            int Category() { return m_Category; };
            void Category(int Value) { m_Category = Value; };

            int CategoryMask() { return m_CategoryMask; };
            void CategoryMask(int Value) { m_CategoryMask = Value; };

            void SetLocale(LPCSTR Locale);
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CHeapComponent --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CHeapComponent {
        public:
            CHeapComponent();
            ~CHeapComponent();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CList -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        enum CDuplicates {
            dupIgnore, dupAccept, dupError
        };
        //--------------------------------------------------------------------------------------------------------------

        #define MaxListSize 134217727
        #define HEAP_ZERO_MEMORY    1
        //--------------------------------------------------------------------------------------------------------------

        #define SListCapacityError  _T("List capacity out of bounds (%d)")
        #define SListCountError     _T("List count out of bounds (%d)")
        #define SListIndexError     _T("List index out of bounds (%d)")
        //--------------------------------------------------------------------------------------------------------------

        typedef Pointer *PPointerList;

        typedef int (*ListSortCompare)(Pointer Item1, Pointer Item2);
        //--------------------------------------------------------------------------------------------------------------

        enum ListNotification {
            lnAdded, lnExtracted, lnDeleted
        };

        enum ListAssignOp {
            laCopy, laAnd, laOr, laXor, laSrcUnique, laDestUnique
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CList: public CObject, public CHeapComponent {
        private:

            PPointerList m_pList;

            int m_nCount;
            int m_nCapacity;

        protected:

            Pointer Get(int Index);
            void Put(int Index, Pointer Item);

            virtual void Grow();
            virtual void Notify(Pointer Ptr, ListNotification Action);

        public:

            CList();

            ~CList() override;

            inline static class CList *Create() { return new CList(); };

            int Add(Pointer Item);

            virtual void Clear();

            void Delete(int Index);

            void Exchange(int Index1, int Index2);

            CList *Expand();

            Pointer Extract(Pointer Item);

            Pointer First();

            int IndexOf(Pointer Item);

            void Insert(int Index, Pointer Item);

            Pointer Last();

            void Move(int CurIndex, int NewIndex);

            int Remove(Pointer Item);

            void Pack();

            void Sort(ListSortCompare Compare);

            void Assign(CList *ListA, ListAssignOp AOperator = laCopy, CList *ListB = nullptr);

            int GetCapacity() const noexcept { return m_nCapacity; }
            void SetCapacity(int NewCapacity);

            int GetCount() const noexcept { return m_nCount; }
            void SetCount(int NewCount);

            int Capacity() const noexcept { return GetCapacity(); }
            int Count() const noexcept { return GetCount(); }

            PPointerList GetList() { return m_pList; }

            Pointer Items(int Index) { return Get(Index); }
            void Items(int Index, Pointer Value) { Put(Index, Value); }

            Pointer operator[](int Index) { return Get(Index); }

        }; // CList

        //--------------------------------------------------------------------------------------------------------------

        enum COperation { opInsert, opRemove };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPersistent -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CPersistent: public CObject
        {
        private:

            CPersistent *m_Owner;

            int m_Tag;

        protected:

            virtual CPersistent *GetOwner() { return m_Owner; };
            virtual CPersistent *GetOwner() const { return m_Owner; };

        public:

            explicit CPersistent(CPersistent *AOwner): CObject(), m_Tag(0), m_Owner(AOwner) {};

            ~CPersistent() override = default;

            int Tag() { return m_Tag; }
            void Tag(int Value) { m_Tag = Value; }

        }; // CPersistent

        //--------------------------------------------------------------------------------------------------------------

        //-- CCollection -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCollectionItem;
        //--------------------------------------------------------------------------------------------------------------

        enum CCollectionNotification { cnAdded, cnExtracting, cnDeleting };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCollection: public CPersistent
        {
            friend CCollectionItem;

        private:

            CList *m_Items;

            int m_UpdateCount;
            int m_NextId;

            int GetCount();

            void InsertItem(CCollectionItem *Item);
            void RemoveItem(CCollectionItem *Item);

        protected:

            virtual void Added(CCollectionItem *Item);
            virtual void Deleting(CCollectionItem *Item);
            virtual void Notify(CCollectionItem *Item, CCollectionNotification Action);

            int GetAttrCount();
            void Changed();

            virtual CCollectionItem *GetItem(int Index);
            virtual void SetItem(int Index, CCollectionItem *Value);

            virtual void SetItemName(CCollectionItem *Item);
            virtual void Update(CCollectionItem *Item);

            int GetNextId() { return m_NextId; };
            int GetUpdateCount() { return m_UpdateCount; };

        public:

            explicit CCollection(CPersistent *AOwner);

            ~CCollection() override;

            CPersistent *Owner();

            virtual void BeginUpdate();
            virtual void EndUpdate();

            virtual void Clear();
            virtual void Delete(int Index);

            CCollectionItem *FindItemId(int Id);

            int Count() { return GetCount(); };

            virtual CCollectionItem *Items(int Index) { return GetItem(Index); }
            virtual void Items(int Index, CCollectionItem *Value) { SetItem(Index, Value); }

            virtual CCollectionItem *operator[] (int Index) { return Items(Index); };

        }; // CCollection

        //--------------------------------------------------------------------------------------------------------------

        //-- CCollectionItem -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCollectionItem: public CPersistent
        {
            typedef CPersistent inherited;
            friend CCollection;

        private:

            CCollection *m_Collection;
            int m_Id;

            int GetIndex();

        protected:

            void Changed(bool AllItems);
            CPersistent *GetOwner() override;
            virtual void SetCollection(CCollection *Value);
            virtual void SetIndex(int Value);

        public:

            explicit CCollectionItem(CCollection *Collection);

            ~CCollectionItem() override;

            CCollection *Collection() { return m_Collection; };
            void Collection(CCollection *Value) { SetCollection(Value); };

            int Id() { return m_Id; };

            int Index() { return GetIndex(); };
            void Index(int Value) { SetIndex(Value); };

        }; // CCollectionItem

        //--------------------------------------------------------------------------------------------------------------

        //-- CStream ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define soFromBeginning   SEEK_SET
        #define soFromCurrent     SEEK_CUR
        #define soFromEnd         SEEK_END
        //--------------------------------------------------------------------------------------------------------------

        enum CSeekOrigin { soBeginning = SEEK_SET, soCurrent = SEEK_CUR, soEnd = SEEK_END };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CStream: public CHeapComponent
        {
        private:

            size_t GetPosition();
            void SetPosition(size_t Pos);

        protected:

            virtual size_t GetSize();
            virtual void SetSize(size_t NewSize);

        public:

            CStream(): CHeapComponent() {};
            virtual ~CStream() = default;

            virtual size_t Read(void *Buffer, size_t Count) abstract;
            virtual size_t Write(const void *Buffer, size_t Count) abstract;

            virtual size_t Seek(off_t Offset, unsigned short Origin) abstract;
            virtual size_t Seek(off_t Offset, CSeekOrigin Origin);

            void ReadBuffer(void *Buffer, size_t Count);
            void WriteBuffer(const void *Buffer, size_t Count);

            size_t CopyFrom(CStream *Source, size_t Count);

            size_t Position() { return GetPosition(); };
            void Position(size_t Value) { SetPosition(Value); };

            size_t Size() { return GetSize(); };
            void Size(size_t Value) { SetSize(Value); };

        }; // CStream

        //--------------------------------------------------------------------------------------------------------------

        //-- CHandleStream ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define INVALID_HANDLE_VALUE            (-1)
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CHandleStream : public CStream
        {
            typedef CStream inherited;

        protected:

            int m_Handle;

            void CreateHandle(int AHandle);

            void SetSize(size_t NewSize) override;

        public:

            explicit CHandleStream(int AHandle): inherited(), m_Handle(INVALID_HANDLE_VALUE) { CreateHandle(AHandle); };

            ~CHandleStream() override = default;

            size_t Read(void *Buffer, size_t Count) override;

            size_t Write(const void *Buffer, size_t Count) override;

            size_t Seek(off_t Offset, unsigned short Origin) override;

        }; // CHandleStream

        //--------------------------------------------------------------------------------------------------------------

        //-- CFileStream -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define OF_CREATE                       (O_CREAT | O_WRONLY | O_TRUNC)
        #define FILE_ATTRIBUTE_NORMAL           0644
        //--------------------------------------------------------------------------------------------------------------

        #define fmOpenRead                      (O_RDONLY)
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CFileStream : public CHandleStream
        {
            typedef CHandleStream inherited;

        public:

            CFileStream(LPCTSTR lpszFileName, unsigned short Mode);

            ~CFileStream() override;

            inline static class CFileStream *CreateAs(LPCTSTR lpszFileName, unsigned short Mode) {
                return new CFileStream(lpszFileName, Mode);
            };

        }; // CFileStream

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomMemoryStream ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CMemoryStream;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCustomMemoryStream: public CStream
        {
            friend CMemoryStream;

        private:

            Pointer m_Memory;

            size_t m_Size;
            size_t m_Position;

        protected:

            void SetPointer(Pointer Ptr, size_t Size);

        public:

            CCustomMemoryStream();

            ~CCustomMemoryStream() override = default;

            inline size_t Seek(off_t Offset, CSeekOrigin Origin) override { return CStream::Seek(Offset, Origin); };

            size_t Read(Pointer Buffer, size_t Count) override;

            size_t Seek(off_t Offset, unsigned short Origin) override;

            void SaveToStream(CStream *Stream);
            void SaveToFile(LPCTSTR lpszFileName);

            virtual Pointer Memory() { return m_Memory; }

        }; // CCustomMemoryStream

        //--------------------------------------------------------------------------------------------------------------

        //-- CMemoryStream ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CMemoryStream: public CCustomMemoryStream
        {
            typedef CCustomMemoryStream inherited;

        private:

            size_t m_Capacity;

            void SetCapacity(size_t NewCapacity);

        protected:

            virtual void *Realloc(size_t &NewCapacity);

            size_t Capacity() { return m_Capacity; };
            void Capacity(size_t Value) { SetCapacity(Value); };

        public:

            CMemoryStream();

            ~CMemoryStream() override;

            inline static class CMemoryStream *Create() { return new CMemoryStream; };

            virtual void Clear();

            void LoadFromStream(CStream *Stream);
            void LoadFromFile(LPCTSTR lpszFileName);

            void SetSize(size_t NewSize) override;

            size_t Write(const void *Buffer, size_t Count) override;

        }; // CMemoryStream

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomStringStream ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CStringStream;
        class CCustomString;
        class CString;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCustomStringStream: public CStream
        {
            friend CStringStream;
            friend CCustomString;
            friend CString;

        private:

            LPWSTR m_wData;
            LPTSTR m_Data;

            size_t m_Size;
            size_t m_Position;

        protected:

            void SetData(LPTSTR Data, size_t Size);

            size_t GetSize() const noexcept { return m_Size; };

        public:

            CCustomStringStream();

            ~CCustomStringStream() override = default;

            inline size_t Seek(off_t Offset, CSeekOrigin Origin) override { return CStream::Seek(Offset, Origin); };

            size_t Read(Pointer Buffer, size_t Count) override;

            size_t Seek(off_t Offset, unsigned short Origin) override;

            void SaveToStream(CStream *Stream);
            void SaveToFile(LPCTSTR lpszFileName);

            LPTSTR Data() { return m_Data; };
            LPCTSTR Data() const noexcept { return m_Data; };

            LPTSTR StrPos() { return m_Data + m_Position; };
            LPCTSTR StrPos() const noexcept { return m_Data + m_Position; };

        }; // CCustomStringStream

        //--------------------------------------------------------------------------------------------------------------

        //-- CStringStream ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CStringStream: public CCustomStringStream
        {
            typedef CCustomStringStream inherited;

        private:

            size_t m_Capacity;

            void SetCapacity(size_t NewCapacity);

        protected:

            virtual LPTSTR Realloc(size_t &NewCapacity);

            void SetSize(size_t NewSize) override;

            void Capacity(size_t Value) { SetCapacity(Value); };

        public:

            CStringStream();

            ~CStringStream() override;

            virtual void Clear();

            virtual void SetLength(size_t NewLength) abstract;

            void LoadFromStream(CStream *Stream);
            void LoadFromFile(LPCTSTR lpszFileName);
            void LoadFromFile(const CString& FileName);

            size_t Write(const void *Buffer, size_t Count) override;

            size_t Capacity() const noexcept { return m_Capacity; };

        }; // CStringStream

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomString ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CCustomString: public CStringStream {
            typedef CStringStream inherited;

        private:

            size_t m_Length;

            void WriteStr(LPCTSTR Str, size_t Length = 0);

            void SetSize(size_t NewSize) override;

        protected:

            size_t GetLength() const noexcept { return m_Length; };

            void SetStr(LPCTSTR Str, size_t Length = 0);
            void AddStr(LPCTSTR Str, size_t Length = 0);

            void SetChar(TCHAR C, size_t Length = 1);
            void AddChar(TCHAR C, size_t Length = 1);

            LPTSTR Str() {
                if Assigned(m_Data)
                    m_Data[m_Length] = '\0';
                return m_Data;
            };

            LPCSTR AnsiStr() const noexcept {
                if Assigned(m_Data)
                    m_Data[m_Length] = '\0';
                return m_Data;
            };

            LPCWSTR WideStr() const noexcept {
                if Assigned(m_wData)
                    m_wData[m_Length] = L'\0';
                return m_wData;
            };

            LPCTSTR Str() const noexcept {
                if Assigned(m_Data)
                    m_Data[m_Length] = '\0';
                return m_Data;
            };

        public:

            CCustomString();

            void Clear() override;

            void SetLength(size_t NewLength) override;

            size_t Length() const noexcept { return GetLength(); };

            size_t Size() const noexcept { return GetSize(); };

            TCHAR GetChar(size_t Index);
            const TCHAR GetChar(size_t Index) const;

            bool IsEmpty() const noexcept { return m_Data == nullptr; };

            size_t Truncate();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CString ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define MaxFormatStringLength 2048
        //--------------------------------------------------------------------------------------------------------------

        class CString: public CCustomString {
            typedef TCHAR value_type;
            typedef size_t size_type;
            typedef value_type& reference;
            typedef const value_type& const_reference;

        private:

            size_t m_MaxFormatSize;

        public:

            CString();

            CString(const CString& S);

            CString(LPCTSTR Str, size_t Length = 0);

            CString(TCHAR C);

            explicit CString(size_t Length, TCHAR C);

            void Create(const CString& S);
            void Create(LPCTSTR Str, size_t Length = 0);
            void Create(TCHAR C);
            void Create(size_t Length, TCHAR C);

            void Append(const CString& S);
            void Append(LPCTSTR Str, size_t Length = 0);
            void Append(TCHAR C);
            void Append(size_t Length, TCHAR C);

            size_t Copy (LPTSTR Str, size_t Len, size_t Pos = 0) const;

            void Format(LPCTSTR pszFormat, ...);
            void Format(LPCTSTR pszFormat, va_list argList);

            size_t MaxFormatSize() { return m_MaxFormatSize; }
            void MaxFormatSize(size_t Value) { m_MaxFormatSize = Value; }

            size_t Find(const CString& S, size_t Pos = 0) const;
            size_t Find(LPCTSTR Str, size_t Pos = 0) const;
            size_t Find(LPCTSTR Str, size_t Pos, size_t Length) const;
            size_t Find(TCHAR C, size_t Pos = 0) const;

            value_type GetFront() { return GetChar(0); };
            value_type GetFront() const { return GetChar(0); };

            value_type GetBack() { return GetChar(Length() - 1); };
            value_type GetBack() const { return GetChar(Length() - 1); };

            int Compare(const CString& S) const;
            int Compare(const std::string& S) const;
            int Compare(LPCTSTR Str) const;

            CString Trim(TCHAR TrimChar = ' ') const;
            CString TrimLeft(TCHAR TrimChar = ' ') const;
            CString TrimRight(TCHAR TrimChar = ' ') const;

            CString SubString(size_t StartIndex, size_t Length = npos) const;

            CString Upper() const;
            CString Lower() const;

            void clear() { Clear(); };

            bool empty() const noexcept { return IsEmpty(); };

            size_type size() const noexcept { return GetSize(); };
            size_type length() const noexcept { return Length(); };
            size_type capacity() const noexcept { return Capacity(); };

            value_type front() const { return GetFront(); };
            value_type back() const { return GetBack(); };

            value_type at(size_t Index) const { return GetChar(Index); };

            CString substr (size_t pos = 0, size_t len = npos) const { return SubString(pos, len); };

            LPCTSTR data() const noexcept { return Data(); }

            LPCTSTR c_str() const noexcept { return Str(); }

            LPCSTR c_a_str() const noexcept { return AnsiStr(); }
            LPCWSTR c_w_str() const noexcept { return WideStr(); }

            template <class T>
            int compare(T Value) const { return Compare(Value); }

            template <class T>
            void append (T Value) { Append(Value); };

            template <class T>
            size_t find (T Value, size_t Pos = 0) { return Find(Value, Pos); };
            size_t find (LPCTSTR Str, size_t Pos, size_t Length) { return Find(Str, Pos, Length); };

            CString& operator= (const CString& S) {
                if (this != &S) {
                    Clear();
                    Create(S);
                }
                return *this;
            };

            CString& operator= (LPCTSTR Value) {
                Clear();
                Create(Value);
                return *this;
            };

            CString& operator= (TCHAR Value) {
                Clear();
                Create(Value);
                return *this;
            };

            CString& operator= (int Value) {
                Clear();
                TCHAR szValue[_INT_T_LEN + 1] = {0};
                Create(IntToStr(Value, szValue, _INT_T_LEN));
                return *this;
            };

            CString& operator= (float Value) {
                Clear();
                TCHAR szValue[_INT_T_LEN + 1] = {0};
                Create(FloatToStr(Value, szValue, _INT_T_LEN));
                return *this;
            };

            CString& operator= (double Value) {
                Clear();
                TCHAR szValue[_INT_T_LEN + 1] = {0};
                Create(FloatToStr(Value, szValue, _INT_T_LEN));
                return *this;
            };

            CString& operator+= (const CString& S) {
                if (this != &S) Append(S);
                return *this;
            };

            friend CString operator+= (const CString& LS, const CString& RS) {
                CString S(LS);
                S.Append(RS);
                return S;
            };

            CString& operator+= (LPCTSTR Value) {
                Append(Value);
                return *this;
            };

            CString& operator+= (TCHAR Value) {
                Append(1, Value);
                return *this;
            };

            template <typename T>
            friend CString operator+= (T LS, const CString& RS) {
                CString S(LS);
                S.Append(RS);
                return S;
            };

            template <typename T>
            friend CString operator+= (const CString& LS, T RS) {
                CString S(LS);
                S.Append(RS);
                return S;
            };

            friend CString operator+ (const CString& LS, const CString& RS) {
                CString S(LS);
                S.Append(RS);
                return S;
            };

            friend CString operator+ (LPCTSTR LS, const CString& RS) {
                CString S(LS);
                S.Append(RS);
                return S;
            };

            friend CString operator+ (TCHAR LS, const CString& RS) {
                CString S(LS);
                S.Append(RS);
                return S;
            };

            friend CString operator+ (const CString& LS, LPCTSTR RS) {
                CString S(LS);
                S.Append(RS);
                return S;
            };

            friend CString operator+ (const CString& LS, TCHAR RS) {
                CString S(LS);
                S.Append(1, RS);
                return S;
            };

            template <class T>
            inline bool operator== (T Value) { return Compare(Value) == 0; };

            template <class T>
            inline bool operator== (T Value) const { return Compare(Value) == 0; };

            template <class T>
            inline bool operator!= (T Value) { return Compare(Value) != 0; };

            template <class T>
            inline bool operator!= (T Value) const { return Compare(Value) != 0; };

            template <class T>
            inline bool operator< (T Value) { return Compare(Value) < 0; };

            template <class T>
            inline bool operator< (T Value) const { return Compare(Value) < 0; };

            template <class T>
            inline bool operator> (T Value) { return Compare(Value) > 0; };

            template <class T>
            inline bool operator> (T Value) const { return Compare(Value) > 0; };

            template <class T>
            inline bool operator<= (T Value) { return Compare(Value) <= 0; };

            template <class T>
            inline bool operator<= (T Value) const { return Compare(Value) <= 0; };

            template <class T>
            inline bool operator>= (T Value) { return Compare(Value) >= 0; };

            template <class T>
            inline bool operator>= (T Value) const { return Compare(Value) >= 0; };

            CString& operator<< (const CString& S) {
                if (this != &S) Append(S);
                return *this;
            };

            CString& operator<< (const std::string& s) {
                Append(s.c_str());
                return *this;
            };

            CString& operator<< (int Value) {
                TCHAR szValue[_INT_T_LEN + 1] = {0};
                IntToStr(Value, szValue, _INT_T_LEN);
                Append(szValue);
                return *this;
            };

            CString& operator<< (size_t Value) {
                TCHAR szValue[_INT_T_LEN + 1] = {0};
                IntToStr(Value, szValue, _INT_T_LEN, 16);
                Append(szValue);
                return *this;
            };

            template <class T>
            CString& operator<< (T Value) {
                Append(Value);
                return *this;
            };

            template <class T>
            friend CString& operator>> (T LS, CString& RS) {
                RS.Append(LS);
                return RS;
            };

            friend tostream& operator<< (tostream& Out, const CString& S) {
                return Out << S.c_str();
            };

            friend tistream& operator>> (tistream& In, CString& S) {
                TCHAR C;
                while (In.get(C) && C != '\n')
                    S.Append(C);
                return In;
            };

            reference operator[] (size_type Index) { return m_Data[Index]; };
            const_reference operator[] (size_type Index) const { return m_Data[Index]; };

            static const size_type npos = static_cast<size_t>(-1);
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CStrings --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define sLineBreak          _T("\n")

        #define sDelimiter          _T(',')
        #define sQuoteChar          _T('"')
        #define sNameValueSeparator _T('=')

        class CStrings;
        //--------------------------------------------------------------------------------------------------------------

        class CStringsEnumerator;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CStrings: public CPersistent
        {
            typedef CPersistent inherited;
            typedef LPCTSTR reference;

        private:

            CString m_Text;

            LPCTSTR m_LineBreak;

            TCHAR m_Delimiter;
            TCHAR m_QuoteChar;
            TCHAR m_NameValueSeparator;

            bool m_StrictDelimiter;
            int m_UpdateCount;

            virtual const CString &GetName(int Index) const abstract;

            virtual const CString &GetValue(const CString &Name) const abstract;
            virtual const CString &GetValue(reference Name) const abstract;

            void SetValue(const CString &Name, const CString &Value);
            void SetValue(reference Name, reference Value);

            TCHAR GetDelimiter();
            void SetDelimiter(TCHAR Value);
            LPCTSTR GetLineBreak() const;
            void SetLineBreak(LPCTSTR Value);
            TCHAR GetQuoteChar();
            void SetQuoteChar(TCHAR Value);
            TCHAR GetNameValueSeparator() const;
            void SetNameValueSeparator(TCHAR Value);
            bool GetStrictDelimiter();
            void SetStrictDelimiter(bool Value);
            CString GetValueFromIndex(int Index);

            void SetValueFromIndex(int Index, const CString &Value);
            void SetValueFromIndex(int Index, reference Value);

        protected:

            void Error(const CString &Msg, int Data);
            virtual CString &Get(int Index) abstract;
            virtual const CString &Get(int Index) const abstract;
            virtual int GetCapacity() const noexcept;
            virtual int GetCount() const noexcept abstract;
            virtual CObject* GetObject(int Index);
            virtual CObject* GetObject(int Index) const;
            virtual void Put(int Index, const CString &S);
            virtual void Put(int Index, reference Str);
            virtual void PutObject(int Index, CObject* AObject);
            virtual void SetCapacity(int NewCapacity);
            virtual void SetText(const CString &Value);
            virtual void SetUpdateState(bool Updating);
            int UpdateCount() { return m_UpdateCount; };
            virtual int CompareStrings(const CString &S1, const CString &S2);

        public:

            CStrings();

            ~CStrings() override = default;

            virtual int Add(const CString &S);
            virtual int Add(reference Str);
            virtual int Add(TCHAR C);
            virtual int AddObject(const CString &S, CObject* AObject);
            virtual int AddObject(reference Str, CObject* AObject);
            virtual int AddObject(TCHAR C, CObject* AObject);
            virtual int AddPair(const CString &Name, const CString &Value);
            virtual int AddPair(reference Name, reference Value);
            virtual int AddPair(reference Name, const CString &Value);
            virtual int AddPair(const CString &Name, reference Value);
            void Append(const CString &S);
            void Append(reference Str);
            virtual void AddStrings(const CStrings& Strings);
            virtual void Assign(const CStrings& Source);
            virtual void SetStrings(const CStrings& Source);
            void BeginUpdate();
            virtual void Clear() abstract;
            virtual void Delete(int Index) abstract;
            void EndUpdate();
            bool Equals(CStrings* Strings);
            virtual void Exchange(int Index1, int Index2);
            virtual bool GetTextStr(LPTSTR Buffer, size_t &SizeBuf);
            virtual const CString &GetText(CString& Value) const;
            virtual int IndexOf(const CString &S);
            virtual int IndexOf(const CString &S) const;
            virtual int IndexOf(reference Str);
            virtual int IndexOf(reference Str) const;
            virtual int IndexOfName(const CString &Name);
            virtual int IndexOfName(const CString &Name) const;
            virtual int IndexOfName(reference Name);
            virtual int IndexOfName(reference Name) const;
            virtual int IndexOfObject(CObject* AObject);
            virtual int IndexOfObject(CObject* AObject) const;
            virtual void Insert(int Index, const CString &S) abstract;
            virtual void Insert(int Index, reference Str) abstract;
            virtual void Insert(int Index, TCHAR C) abstract;
            virtual void InsertObject(int Index, const CString &S, CObject* AObject);
            virtual void InsertObject(int Index, reference Str, CObject* AObject);
            virtual void InsertObject(int Index, TCHAR C, CObject* AObject);
            virtual void LoadFromFile(LPCTSTR lpszFileName);
            virtual void LoadFromStream(CStream* Stream);
            virtual void Move(int CurIndex, int NewIndex);
            virtual void SaveToFile(LPCTSTR lpszFileName);
            virtual void SaveToStream(CStream* Stream);
            virtual void SetTextStr(LPCTSTR Text, size_t Size);

            int Capacity() { return GetCapacity(); };
            void Capacity(int NewCapacity) { SetCapacity(NewCapacity); };

            int Count() const noexcept { return GetCount(); };
            TCHAR Delimiter() { return m_Delimiter; };
            void Delimiter(TCHAR Value) { m_Delimiter = Value; };

            LPCTSTR LineBreak() const { return GetLineBreak(); };
            void LineBreak(LPCTSTR Value) { SetLineBreak(Value); };

            const CString &Names(int Index) const { return GetName(Index); };

            CObject *Objects(int Index) { return GetObject(Index); };
            CObject *Objects(int Index) const { return GetObject(Index); };
            void Objects(int Index, CObject *Value) { return PutObject(Index, Value); };
            TCHAR QuoteChar() { return GetQuoteChar(); }
            void QuoteChar(TCHAR Value) { SetQuoteChar(Value); };

            const CString &Values(const CString &Name) const { return GetValue(Name); };
            const CString &Values(reference Name) const { return GetValue(Name); };

            void Values(const CString &Name, const CString &Value) { SetValue(Name, Value); };
            void Values(reference Name, reference Value) { SetValue(Name, Value); };

            CString ValueFromIndex(int Index) { return GetValueFromIndex(Index); };
            void ValueFromIndex(int Index, const CString &Value) { SetValueFromIndex(Index, Value); };
            void ValueFromIndex(int Index, reference Value) { SetValueFromIndex(Index, Value); };

            TCHAR NameValueSeparator() const { return GetNameValueSeparator(); };
            void NameValueSeparator(TCHAR Value) { SetNameValueSeparator(Value); };

            bool StrictDelimiter() { return GetStrictDelimiter(); };
            void StrictDelimiter(bool Value) { SetStrictDelimiter(Value); };

            CString &Strings(int Index) { return Get(Index); };
            const CString &Strings(int Index) const { return Get(Index); };

            void Strings(int Index, const CString &Value) { return Put(Index, Value); };

            const CString& ToText(CString& Value) const { return GetText(Value); };

            const CString& Text() { return GetText(m_Text); };
            void Text(const CString &Value) { SetText(Value); };

            CString &First() { return Get(0); };
            const CString &First() const { return Get(0); };

            CString &front() { return Get(0); };
            const CString &front() const { return Get(0); };

            CString &Last() { return Get(GetCount() - 1); };
            const CString &Last() const { return Get(GetCount() - 1); };

            CString &back() { return Get(GetCount() - 1); };
            const CString &back() const { return Get(GetCount() - 1); };

            CStrings &operator=(const CStrings &Strings) {
                if (&Strings != this) {
                    Assign(Strings);
                }
                return *this;
            }

            virtual CStrings &operator=(const CString &String) {
                if (!String.IsEmpty()) {
                    SetText(String);
                }
                return *this;
            }

            virtual CStrings &operator=(reference Str) {
                if (Str != nullptr) {
                    size_t Size = strlen(Str);
                    SetTextStr(Str, Size);
                }
                return *this;
            }

            virtual CString &operator[] (int Index) { return Strings(Index); }
            virtual const CString &operator[] (int Index) const { return Strings(Index); }

            virtual const CString &operator[] (reference Name) const { return Values(Name); }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CStringsEnumerator ----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CStringsEnumerator: public CObject
        {
            typedef CObject inherited;

        private:

            int         m_Index;
            CStrings   *m_Strings;

        public:

            explicit CStringsEnumerator(CStrings* AStrings): CObject() {
                m_Index = -1;
                m_Strings = AStrings;
            }

            inline ~CStringsEnumerator() override = default;

            CString &GetCurrent() {
                return m_Strings->Strings(m_Index);
            }

            const CString &GetCurrent() const {
                return m_Strings->Strings(m_Index);
            }

            bool MoveNext() {
                if (m_Index < m_Strings->Count() - 1) {
                    m_Index++;
                    return true;
                }
                return false;
            }

            CString& Current() { return GetCurrent(); };

            const CString& Current() const { return GetCurrent(); };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CStringList -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CStringList;
        //--------------------------------------------------------------------------------------------------------------

        typedef struct StringItem {
            CString     String;
            CString     Name;
            CString     Value;
            CObject    *Object;
        } CStringItem, *PStringItem;
        //--------------------------------------------------------------------------------------------------------------

        //typedef CStringItem CStringItemList[];
        typedef PStringItem *PStringItemList;
        //--------------------------------------------------------------------------------------------------------------

        typedef int (*PStringListSortCompare)(CStringList *List, int Index1, int Index2);
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CStringList: public CHeapComponent, public CStrings
        {
            typedef CStrings inherited;
            typedef LPCTSTR reference;

        private:

            PStringItemList m_pList;

            CString m_NullValue;

            int m_nCount;
            int m_nCapacity;

            bool m_fOwnsObjects;

            virtual void Grow();
            //void QuickSort(int L, int R, PStringListSortCompare SCompare);

            const CString &GetName(int Index) const override;

            const CString &GetValue(const CString &Name) const override;
            const CString &GetValue(reference Name) const override;

        protected:

            CString &Get(int Index) override;
            const CString &Get(int Index) const override;

            void Put(int Index, const CString &S) override;
            void Put(int Index, reference Str) override;

            int GetCapacity() const noexcept override { return m_nCapacity; }
            void SetCapacity(int NewCapacity) override;

            int GetCount() const noexcept override { return m_nCount; }

            CObject* GetObject(int Index) override;
            CObject* GetObject(int Index) const override;
            void PutObject(int Index, CObject* AObject) override;

            virtual void InsertItem(int Index, const CString &S, CObject *AObject);
            virtual void InsertItem(int Index, reference Str, CObject *AObject);
            virtual void InsertItem(int Index, TCHAR C, CObject *AObject);

        public:

            CStringList();

            explicit CStringList(bool AOwnsObjects);

            inline static class CStringList* Create(bool AOwnsObjects = false) { return new CStringList(AOwnsObjects); };

            ~CStringList() override;

            void Assign(const CStrings& Source) override;

            void Clear() override;

            virtual void Update(int Index);

            void Delete(int Index) override;

            int Add(const CString &S) override;
            int AddObject(const CString &S, CObject* AObject) override;

            int Add(reference Str) override;
            int AddObject(reference Str, CObject* AObject) override;

            int Add(TCHAR C) override;
            int AddObject(TCHAR C, CObject* AObject) override;

            void Insert(int Index, const CString &S) override;
            void InsertObject(int Index, const CString &S, CObject* AObject) override;

            void Insert(int Index, reference Str) override;
            void InsertObject(int Index, reference Str, CObject* AObject) override;

            void Insert(int Index, TCHAR C) override;
            void InsertObject(int Index, TCHAR C, CObject* AObject) override;

            bool OwnsObjects() { return m_fOwnsObjects; };

            CStringList &operator=(const CStringList &Strings) {
                if (&Strings != this) {
                    Assign(Strings);
                }
                return *this;
            }

            CStringList &operator=(const CString &String) override {
                if (!String.IsEmpty()) {
                    SetText(String);
                }
                return *this;
            }

            CStringList &operator=(reference Str) override {
                if (Str != nullptr) {
                    size_t Size = strlen(Str);
                    SetTextStr(Str, Size);
                }
                return *this;
            }

        }; // CStringList

        //--------------------------------------------------------------------------------------------------------------

        //-- CQueueItem ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CQueueItem: public CCollectionItem
        {
        private:

            Pointer m_pQueue;
            CList *m_pItems;

        protected:

            Pointer Get(int Index);
            void Put(int Index, Pointer Value);

        public:

            CQueueItem(CCollection *ACollection, Pointer AQueue);

            ~CQueueItem() override;

            int Add(Pointer Item);
            void Insert(int Index, Pointer Item);
            void Delete(int Index);

            int IndexOf(Pointer Item);
            int Remove(Pointer Item);

            Pointer First();
            Pointer Last();

            Pointer Queue() { return m_pQueue; }

            int Count() { return m_pItems->Count(); }

            Pointer Item(int Index) { return Get(Index); }
            void Item(int Index, Pointer Value) { Put(Index, Value); }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CQueue ----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CQueue: public CCollection
        {
            typedef CCollection inherited;

        protected:

            CQueueItem *GetItem(int Index) override;
            void SetItem(int Index, const CQueueItem *Item);

        public:

            CQueue();

            int IndexOf(Pointer Queue);
            int Remove(Pointer Queue);

            CQueueItem *Add(Pointer Queue);
            CQueueItem *Insert(int Index, Pointer Queue);
            CQueueItem *First();
            CQueueItem *Last();

            int AddToQueue(Pointer Queue, Pointer P);
            void InsertToQueue(Pointer Queue, int Index, Pointer P);
            void RemoveFromQueue(Pointer Queue, Pointer P);

            Pointer FirstItem(Pointer Queue);
            Pointer LastItem(Pointer Queue);

            CQueueItem *Items(int Index) override { return GetItem(Index); }
            void Items(int Index, CQueueItem *Value) { SetItem(Index, Value); }

            CQueueItem *operator [] (int Index) override { return GetItem(Index); };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CSysError -------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSysError: public CStringList {
        private:

            CString     m_UnknownError;

        protected:

            void Create();

        public:

            CSysError(): CStringList(), m_UnknownError(_T("Unknown Error")) { Create(); };

            ~CSysError() override = default;

            class CSysError *CreateSysError() { return GSysError = new CSysError(); };

            void DestroySysError() { delete GSysError; };

            const CString& StrError(int AError);
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CSysErrorComponent ----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSysErrorComponent {
        public:
            CSysErrorComponent();
            ~CSysErrorComponent();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CThread ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CThread;
        //--------------------------------------------------------------------------------------------------------------

        typedef void (CThreadMethod)();
        //--------------------------------------------------------------------------------------------------------------

#pragma pack (push, 4)

        typedef struct tagSynchronizeRecord {
            PVOID Thread;
            CThreadMethod *Method;
            PVOID SynchronizeException;
        } CSynchronizeRecord, *PSynchronizeRecord;
        //--------------------------------------------------------------------------------------------------------------

#pragma pack (pop)

        typedef struct tagSyncProc {
            PSynchronizeRecord SyncRec;
            HANDLE Signal;
        } CSyncProc, *PSyncProc;
        //--------------------------------------------------------------------------------------------------------------

        LPVOID ThreadProc(LPVOID lpParameter);
        //--------------------------------------------------------------------------------------------------------------

        enum CThreadPriority { tpIdle, tpLowest, tpLower, tpNormal, tpHigher, tpHighest, tpTimeCritical };
        //--------------------------------------------------------------------------------------------------------------

        void AddThread();
        void RemoveThread();
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CThread
        {
            friend LPVOID ThreadProc(LPVOID lpParameter);
            //friend bool CheckSynchronize(int Timeout = 0);

        private:

            pthread_t           m_hHandle;
            pthread_attr_t      m_hAttr;

            pthread_mutex_t     m_Lock;
            pthread_cond_t      m_Suspend;

            pid_t               m_nThreadId;

            CSynchronizeRecord *m_Synchronize;
            CNotifyEvent        m_OnTerminate;

            int                 m_nReturnValue;

            bool                m_bCreateSuspended;
            bool                m_bTerminated;
            bool                m_bSuspended;
            bool                m_bFreeOnTerminate;
            bool                m_bFinished;

            void CallOnTerminate();

            void CreateSyncList();
            void Synchronize(PSynchronizeRecord ASyncRec);

            CThreadPriority GetPriority();
            void SetPriority(CThreadPriority Value);

            void SetSuspended(bool Value);

        protected:

            void AfterConstruction();
            void CheckThreadError(int ErrCode);
            void CheckThreadError(bool Success);

            virtual void DoTerminate();
            virtual void Execute() abstract;
            void Synchronize(CThreadMethod *AMethod);

            int ReturnValue() { return m_nReturnValue; };
            void ReturnValue(int Value) { m_nReturnValue = Value; };

            bool Terminated() { return m_bTerminated; };

        public:

            explicit CThread(bool CreateSuspended);
            virtual ~CThread();

            void Resume();
            void Suspend();

            virtual void Terminate();
            int WaitFor();

            void Synchronize(CThread *AThread, CThreadMethod *AMethod);

            pthread_t Handle() { return m_hHandle; };

            pid_t ThreadId() { return m_nThreadId; };

            bool Suspended() { return m_bSuspended; };
            void Suspended(bool Value) { SetSuspended(Value); }

            CThreadPriority Priority() { return GetPriority(); }
            void Priority(CThreadPriority Value) { SetPriority(Value); }

            bool FreeOnTerminate() { return m_bFreeOnTerminate; };
            void FreeOnTerminate(bool Value) { m_bFreeOnTerminate = Value; };

            const CNotifyEvent& OnTerminate() { return m_OnTerminate; };
            void OnTerminate(CNotifyEvent && Value) { m_OnTerminate = Value; };

        }; // CThread

        //--------------------------------------------------------------------------------------------------------------

        //-- CThreadList -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CThreadList
        {

        private:

            CList              *m_List;
            pthread_mutex_t     m_Lock;
            CDuplicates         m_Duplicates;

            void FreeList();

        public:

            CThreadList();
            virtual ~CThreadList();

            void Add(CThread *Item);
            void Clear();
            CList *LockList();
            void Remove(CThread *Item);
            void UnlockList();

            CDuplicates Duplicates() { return m_Duplicates; };
            void Duplicates(CDuplicates Value) { m_Duplicates = Value; };

        }; // CThreadList

    }
}

using namespace Delphi::Classes;
}
#endif //DELPHI_CLASSES_HPP