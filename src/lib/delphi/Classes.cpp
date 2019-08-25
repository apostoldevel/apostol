/*++

Library name:

  libdelphi

Module Name:

  Classes.cpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "delphi.hpp"
#include "Classes.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Delphi {
    
    namespace Classes {

        static unsigned long GHeapCount = 0;
        static unsigned long GThreadCount = 0;
        static unsigned long GSysErrorCount = 0;
        //--------------------------------------------------------------------------------------------------------------

        CList *GSyncList = nullptr;
        bool GProcPosted = false;
        HANDLE GSyncEvent = nullptr;

        CNotifyEvent WakeMainThread = nullptr;

        static pthread_mutex_t GThreadLock;
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CHeap *GHeap = nullptr;
        LIB_DELPHI CSysError *GSysError = nullptr;
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI pid_t MainThreadID = getpid();
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI CDefaultLocale DefaultLocale;
        //--------------------------------------------------------------------------------------------------------------

        inline void InitThreadSynchronization() {
            GThreadLock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
//            pthread_mutex_init(&GThreadLock, nullptr);
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void DoneThreadSynchronization() {
            pthread_mutex_destroy(&GThreadLock);
        }
        //--------------------------------------------------------------------------------------------------------------

        inline void ResetSyncEvent() {

        };
        //--------------------------------------------------------------------------------------------------------------

        inline void WaitForSyncEvent(int Timeout) {

        };
        //--------------------------------------------------------------------------------------------------------------

        inline void SignalSyncEvent() {

        };
        //--------------------------------------------------------------------------------------------------------------

        inline void AddThread() {
            if (GThreadCount == 0)
                InitThreadSynchronization();

            pthread_mutex_lock(&GThreadLock);

            GThreadCount++;

            pthread_mutex_unlock(&GThreadLock);
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void RemoveThread() {
            pthread_mutex_lock(&GThreadLock);

            try {
                GThreadCount--;
                if ((GThreadCount == 0) && (GSyncList != nullptr)) {
                    for (int i = GSyncList->Count() - 1; i >= 0; i--)
                        delete (CSyncProc *) GSyncList->Items(i);

                    delete GSyncList;
                }
            } catch (...) {
            }

            pthread_mutex_unlock(&GThreadLock);

            if (GThreadCount == 0)
                DoneThreadSynchronization();
        };
        //--------------------------------------------------------------------------------------------------------------

        bool CheckSynchronize(int Timeout) {
            bool Result = false;

            CSyncProc *SyncProc;
            CList *LocalSyncList = nullptr;

            if (::getpid() != MainThreadID)
                throw Exception::Exception(_T("CheckSynchronize called from thread, which is NOT the main thread."));

            if (Timeout > 0)
                WaitForSyncEvent(Timeout);
            else
                ResetSyncEvent();

            pthread_mutex_lock(&GThreadLock);
            try {
                LocalSyncList = new CList();
                LocalSyncList->Assign(GSyncList, laCopy);

                Result = (LocalSyncList != nullptr) && (LocalSyncList->Count() > 0);

                if (Result) {
                    while (LocalSyncList->Count() > 0) {
                        SyncProc = (CSyncProc *) LocalSyncList->Items(0);

                        LocalSyncList->Delete(0);

                        pthread_mutex_unlock(&GThreadLock);

                        try {
                            SyncProc->SyncRec->Method();
                        } catch (...) {
                        }

                        pthread_mutex_lock(&GThreadLock);
                        //TODO: SetEvent(SyncProc->Signal);
                    }
                }
            } catch (...) {
            }

            pthread_mutex_unlock(&GThreadLock);

            if (LocalSyncList != nullptr)
                delete LocalSyncList;

            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void AddHeap() {
            if (GHeapCount == 0) {
                GHeap = GHeap->CreateHeap();
                //GHeap->Initialize();
            }

            GHeapCount++;
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void RemoveHeap() {
            GHeapCount--;

            if (GHeapCount == 0) {
                // Проверка утечки памяти
                chVERIFY(GHeap->GetInitialSize() == 0);
                GHeap->DestroyHeap();
                GHeap = nullptr;
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CHeap -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CHeap::CHeap() {
            m_hHandle = nullptr;

            m_Options = PROT_READ | PROT_WRITE;
            m_MaximumSize = MaxListSize * sizeof(Pointer);
            m_InitialSize = 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        CHeap::~CHeap() {
            if (m_hHandle != nullptr)
                Destroy();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CHeap::Create() {
            m_hHandle = ::mmap(nullptr, m_MaximumSize, m_Options, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

            if (m_hHandle == MAP_FAILED)
                throw ExceptionFrm(_T("Failed to create a new heap with LastError %d.\n"), errno);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CHeap::Destroy() {
            if (m_hHandle != nullptr) {
                ::munmap(m_hHandle, m_MaximumSize);
                m_hHandle = nullptr;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CHeap::Initialize() {
            if (m_hHandle == nullptr)
                Create();
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CHeap::Alloc(unsigned long ulFlags, size_t ulSize) {
            //Initialize();
            auto P = (Pointer) ::malloc(ulSize);
            if (P == nullptr)
                throw Delphi::Exception::Exception(_T("Out of memory while expanding memory stream"));
            if (ulFlags == HEAP_ZERO_MEMORY)
                ::memset(P, 0, ulSize);
            m_InitialSize += ulSize;
            return P;
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CHeap::ReAlloc(unsigned long ulFlags, Pointer lpMem, size_t ulNewSize, size_t ulOldSize) {
            m_InitialSize += (ulNewSize - ulOldSize);
            return (Pointer) ::realloc(lpMem, ulNewSize);
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CHeap::Free(unsigned long ulFlags, Pointer lpMem, size_t ulSize) {
            m_InitialSize -= ulSize;
            ::free(lpMem);
            return nullptr;
        };
        //--------------------------------------------------------------------------------------------------------------

        unsigned long CHeap::Size(unsigned long ulFlags, Pointer lpMem) {
            return ::malloc_usable_size(lpMem);
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CDefaultLocale --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CDefaultLocale::CDefaultLocale(locale_t ALocale) noexcept {
            m_Locale = ALocale;
            m_LocaleName = nullptr;
            m_Category = LC_ALL;
            m_CategoryMask = LC_ALL_MASK;
        };
        //--------------------------------------------------------------------------------------------------------------

        CDefaultLocale::CDefaultLocale(LPCSTR Locale): CDefaultLocale() {
            SetLocale(Locale);
        };
        //--------------------------------------------------------------------------------------------------------------

        CDefaultLocale::~CDefaultLocale() {
            SetLocale(nullptr);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CDefaultLocale::SetLocale(LPCSTR Value) {
            if (m_LocaleName != Value) {
                if ((m_Locale != LC_GLOBAL_LOCALE) && (m_Locale != nullptr)) {
                    freelocale(m_Locale); // !!! WARNING: Valgrind error
                    m_Locale = nullptr;
                }

                m_LocaleName = Value;

                if (m_LocaleName != nullptr) {

                    if (m_Locale != LC_GLOBAL_LOCALE) {

                        if (m_LocaleName[0] == '\0') {
                            m_LocaleName = setlocale(m_Category, "");
                        }

                        locale_t Locale = newlocale(m_CategoryMask, m_LocaleName, m_Locale);

                        if (Locale == nullptr) {
                            throw EOSError(errno, "Could not set locale argument \"%s\" failed: ", m_LocaleName);
                        }

                        m_Locale = Locale;

                        uselocale(m_Locale);
                    } else {
                        setlocale(m_Category, m_LocaleName);
                    }
                }
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CHeapComponent --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CHeapComponent::CHeapComponent() {
            AddHeap();
        };
        //--------------------------------------------------------------------------------------------------------------

        CHeapComponent::~CHeapComponent() {
            RemoveHeap();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CList -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------
        
        void QuickSort(PPointerList SortList, int L, int R, ListSortCompare SCompare) {
            int I, J;
            Pointer P, T;

            do {
                I = L;
                J = R;
                P = SortList[(L + R) >> 1];

                do {
                    while (SCompare(SortList[I], P) < 0)
                        I++;
                    while (SCompare(SortList[J], P) > 0)
                        J--;
                    if (I <= J) {
                        T = SortList[I];
                        SortList[I] = SortList[J];
                        SortList[J] = T;
                        I++;
                        J--;
                    }
                } while (I <= J);

                if (L < J)
                    QuickSort(SortList, L, J, SCompare);

                L = I;
            } while (I < R);
        };
        //--------------------------------------------------------------------------------------------------------------

        CList::CList() : CHeapComponent() {
            m_nCount = 0;
            m_nCapacity = 0;
            m_pList = nullptr;
        };
        //--------------------------------------------------------------------------------------------------------------

        CList::~CList() {
            Clear();
            if (m_pList != nullptr)
                GHeap->Free(0, m_pList, m_nCapacity * sizeof(Pointer));
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CList::Get(int Index) {
            if ((Index < 0) || (m_pList == nullptr) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index];
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Grow() {
            int Delta;

            if (m_nCapacity > 64)
                Delta = m_nCapacity / 4;
            else if (m_nCapacity > 8)
                Delta = 16;
            else
                Delta = 4;

            SetCapacity(m_nCapacity + Delta);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Put(int Index, Pointer Item) {
            Pointer Temp;

            if ((Index < 0) || (m_pList == nullptr) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            if (Item != m_pList[Index]) {
                Temp = m_pList[Index];
                m_pList[Index] = Item;

                if (Temp != nullptr)
                    Notify(Temp, lnDeleted);

                if (Item != nullptr)
                    Notify(Temp, lnAdded);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Notify(Pointer Ptr, ListNotification Action) {

        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::SetCapacity(int NewCapacity) {
            if ((NewCapacity < m_nCount) || (NewCapacity > MaxListSize))
                throw ExceptionFrm(SListCapacityError, NewCapacity);

            if (NewCapacity != m_nCapacity) {
                if (NewCapacity == 0) {
                    m_pList = (PPointerList) GHeap->Free(0, (PPointerList) m_pList, m_nCapacity * sizeof(Pointer));
                } else {
                    if (m_nCapacity == 0) {
                        m_pList = (PPointerList) GHeap->Alloc(0, NewCapacity * sizeof(Pointer));
                    } else {
                        m_pList = (PPointerList) GHeap->ReAlloc(0, (PPointerList) m_pList,
                                                                NewCapacity * sizeof(Pointer),
                                                                m_nCapacity * sizeof(Pointer));
                    }

                    if (m_pList == nullptr)
                        throw Delphi::Exception::Exception(_T("Out of memory while expanding memory heap"));
                }

                m_nCapacity = NewCapacity;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::SetCount(int NewCount) {
            if ((NewCount < 0) || (NewCount > MaxListSize))
                throw ExceptionFrm(SListCountError, NewCount);

            if (NewCount > m_nCapacity)
                SetCapacity(NewCount);

            if (NewCount > m_nCount)
                ZeroMemory(&m_pList[m_nCount], (NewCount - m_nCount) * sizeof(Pointer));
            else
                for (int i = m_nCount - 1; i >= NewCount; i--)
                    Delete(i);

            m_nCount = NewCount;
        };
        //--------------------------------------------------------------------------------------------------------------

        int CList::Add(Pointer Item) {
            int Result = m_nCount;

            if (Result == m_nCapacity)
                Grow();

            m_pList[Result] = Item;
            m_nCount++;

            if (Item != nullptr)
                Notify(Item, lnAdded);

            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Clear() {
            SetCount(0);
            SetCapacity(0);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Delete(int Index) {
            Pointer Temp;

            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            Temp = Get(Index);
            m_nCount--;

            if (Index < m_nCount)
                MoveMemory(&m_pList[Index], &m_pList[Index + 1], (m_nCount - Index) * sizeof(Pointer));

            if (Temp != nullptr)
                Notify(Temp, lnDeleted);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Exchange(int Index1, int Index2) {
            Pointer Item;

            if ((Index1 < 0) || (Index1 >= m_nCount))
                throw ExceptionFrm(SListIndexError, (int) Index1);
            if ((Index2 < 0) || (Index2 >= m_nCount))
                throw ExceptionFrm(SListIndexError, (int) Index2);
            Item = m_pList[Index1];
            m_pList[Index1] = m_pList[Index2];
            m_pList[Index2] = Item;
        };
        //--------------------------------------------------------------------------------------------------------------

        CList *CList::Expand() {
            if (m_nCount == m_nCapacity)
                Grow();

            return this;
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CList::Extract(Pointer Item) {
            int I;
            Pointer Result = nullptr;

            I = IndexOf(Item);

            if (I >= 0) {
                Result = Item;
                m_pList[I] = nullptr;
                Delete(I);
                Notify(Result, lnExtracted);
            }
            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CList::First() {
            return Get(0);
        };
        //--------------------------------------------------------------------------------------------------------------

        int CList::IndexOf(Pointer Item) {
            int Result = 0;

            while ((Result < m_nCount) && (m_pList[Result] != Item))
                Result++;

            if (Result == m_nCount)
                Result = -1;

            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Insert(int Index, Pointer Item) {
            if ((Index < 0) || (Index > m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            if (m_nCount == m_nCapacity)
                Grow();

            if (Index < m_nCount)
                ::MoveMemory(&m_pList[Index + 1], &m_pList[Index], (m_nCount - Index) * sizeof(Pointer));

            m_pList[Index] = Item;
            m_nCount++;

            if (Item != nullptr)
                Notify(Item, lnAdded);
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CList::Last() {
            return Get(m_nCount - 1);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Move(int CurIndex, int NewIndex) {
            Pointer Item;

            if (CurIndex != NewIndex) {
                if ((NewIndex < 0) || (NewIndex >= m_nCount))
                    throw ExceptionFrm(SListIndexError, (int) NewIndex);

                Item = Get(CurIndex);
                m_pList[CurIndex] = nullptr;
                Delete(CurIndex);
                Insert(NewIndex, nullptr);
                m_pList[NewIndex] = Item;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        int CList::Remove(Pointer Item) {
            int Result = IndexOf(Item);
            if (Result >= 0)
                Delete(Result);
            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Pack() {
            for (int i = m_nCount - 1; i >= 0; i--)
                if (Get(i) == nullptr)
                    Delete(i);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Sort(ListSortCompare Compare) {
            if ((m_pList != nullptr) && (m_nCount > 0))
                QuickSort(m_pList, 0, GetCount() - 1, Compare);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CList::Assign(CList *ListA, ListAssignOp AOperator, CList *ListB) {
            int I;
            CList *LSource;

            // ListB given?
            if (ListB != nullptr) {
                LSource = ListB;
                Assign(ListA);
            } else
                LSource = ListA;

            // on with the show
            switch (AOperator) {
                // 12345, 346 = 346 : only those in the new list
                case laCopy: {
                    Clear();
                    SetCapacity(LSource->GetCapacity());
                    for (I = 0; I < LSource->GetCount(); I++)
                        Add(LSource->Items(I));
                    break;
                }

                    // 12345, 346 = 34 : intersection of the two lists
                case laAnd: {
                    for (I = GetCount() - 1; I >= 0; I--)
                        if (LSource->IndexOf(Get(I)) == -1)
                            Delete(I);
                    break;
                }

                    // 12345, 346 = 123456 : union of the two lists
                case laOr: {
                    for (I = 0; I < LSource->GetCount(); I++)
                        if (IndexOf(LSource->Items(I)) == -1)
                            Add(LSource->Items(I));
                    break;
                }

                    // 12345, 346 = 1256 : only those not in both lists
                case laXor: {
                    CList LTemp; // Temp holder of 4 byte values
                    LTemp.SetCapacity(LSource->Count());
                    for (I = 0; I < LSource->Count(); I++)
                        if (IndexOf(LSource->Items(I)) == -1)
                            LTemp.Add(LSource->Items(I));

                    for (I = Count() - 1; I >= 0; I--)
                        if (LSource->IndexOf(Get(I)) != -1)
                            Delete(I);
                    I = Count() + LTemp.Count();
                    if (GetCapacity() < I)
                        SetCapacity(I);
                    for (I = 0; I < LTemp.Count(); I++)
                        Add(LTemp[I]);
                    break;
                }

                    // 12345, 346 = 125 : only those unique to source
                case laSrcUnique: {
                    for (I = Count() - 1; I >= 0; I--)
                        if (LSource->IndexOf(Get(I)) != -1)
                            Delete(I);
                    break;
                }

                    // 12345, 346 = 6 : only those unique to dest
                case laDestUnique: {
                    CList LTemp;
                    LTemp.SetCapacity(LSource->Count());
                    for (I = LSource->Count() - 1; I >= 0; I--)
                        if (IndexOf(LSource->Items(I)) == -1)
                            LTemp.Add(LSource->Items(I));
                    Assign(&LTemp);
                    break;
                }
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCollection -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCollection::CCollection(CPersistent *AOwner): CPersistent(AOwner) {
            m_NextId = 0;
            m_UpdateCount = 0;
            m_Items = new CList();
        };
        //--------------------------------------------------------------------------------------------------------------

        CCollection::~CCollection() {
            m_UpdateCount = 1;
            if (m_Items != nullptr)
                Clear();
            delete m_Items;
        };
        //--------------------------------------------------------------------------------------------------------------

        int CCollection::GetCount() {
            return m_Items->Count();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::InsertItem(CCollectionItem *Item) {
            m_Items->Add(Item);
            Item->m_Collection = this;
            Item->m_Id = m_NextId++;
            SetItemName(Item);
            Notify(Item, cnAdded);
            Changed();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::RemoveItem(CCollectionItem *Item) {
            Notify(Item, cnExtracting);
            if (Item == m_Items->Last())
                m_Items->Delete(m_Items->Count() - 1);
            else
                m_Items->Remove(Item);
            Item->m_Collection = nullptr;
            //NotifyDesigner(this, Item, opRemove);
            Changed();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::SetItemName(CCollectionItem *Item) {
            //
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::Added(CCollectionItem *Item) {
            //
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::Update(CCollectionItem *Item) {
            //
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::Deleting(CCollectionItem *Item) {
            //
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::Notify(CCollectionItem *Item, CCollectionNotification Action) {
            switch (Action) {
                case cnAdded:
                    Added(Item);
                    break;
                case cnDeleting:
                    Deleting(Item);
                    break;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        int CCollection::GetAttrCount() {
            return 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::Changed() {
            if (m_UpdateCount == 0)
                Update(nullptr);
        };
        //--------------------------------------------------------------------------------------------------------------

        CCollectionItem *CCollection::GetItem(int Index) {
            return (CCollectionItem *) m_Items->Items(Index);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::SetItem(int Index, CCollectionItem *Value) {
            m_Items->Items(Index, Value);
        };
        //--------------------------------------------------------------------------------------------------------------

        CPersistent *CCollection::Owner() {
            return GetOwner();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::BeginUpdate() {
            m_UpdateCount++;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::Clear() {
            if (m_Items->Count() > 0) {
                BeginUpdate();
                try {
                    while (m_Items->Count() > 0) {
                        delete (CCollectionItem *) m_Items->Last();
                    }
                } catch (...) {
                    EndUpdate();
                    throw;
                }
                EndUpdate();
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::Delete(int Index) {
            Notify((CCollectionItem *) m_Items->Items(Index), cnDeleting);
            delete (CCollectionItem *) m_Items->Items(Index);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollection::EndUpdate() {
            m_UpdateCount--;
            Changed();
        };
        //--------------------------------------------------------------------------------------------------------------

        CCollectionItem *CCollection::FindItemId(int Id) {
            CCollectionItem *Item = nullptr;

            for (int I = 0; I < m_Items->Count(); I++) {
                Item = static_cast<CCollectionItem *> (m_Items->Items(I));
                if (Item->Id() == Id)
                    return Item;
            }

            return nullptr;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCollectionItem -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCollectionItem::CCollectionItem(CCollection *Collection): CPersistent(Collection) {
            m_Id = 0;
            m_Collection = nullptr;

            SetCollection(Collection);
        };
        //--------------------------------------------------------------------------------------------------------------

        CCollectionItem::~CCollectionItem() {
            SetCollection(nullptr);
        };
        //--------------------------------------------------------------------------------------------------------------

        int CCollectionItem::GetIndex() {
            if (m_Collection != nullptr)
                return m_Collection->m_Items->IndexOf(this);
            else
                return -1;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollectionItem::Changed(bool AllItems) {
            CCollectionItem *Item;

            if ((m_Collection != nullptr) && (m_Collection->m_UpdateCount == 0)) {
                if (AllItems)
                    Item = nullptr;
                else
                    Item = this;
                m_Collection->Update(Item);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        CPersistent *CCollectionItem::GetOwner() {
            return m_Collection;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollectionItem::SetCollection(CCollection *Value) {
            if (m_Collection != Value) {
                if (m_Collection != nullptr)
                    m_Collection->RemoveItem(this);

                if (Value != nullptr)
                    Value->InsertItem(this);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCollectionItem::SetIndex(int Value) {
            int CurIndex;
            CurIndex = GetIndex();
            if ((CurIndex >= 0) && (CurIndex != Value)) {
                m_Collection->m_Items->Move(CurIndex, Value);
                Changed(true);
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CStream ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        size_t CStream::GetPosition() {
            return (size_t) Seek(0, soCurrent);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStream::SetPosition(size_t Pos) {
            Seek(Pos, soBeginning);
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CStream::GetSize() {
            size_t Pos, Result;

            Pos = Seek(0, soCurrent);
            Result = Seek(0, soEnd);
            Seek(Pos, soBeginning);

            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStream::SetSize(size_t NewSize) {
            if (NewSize < 0)
                throw ERangeError(_T("Range check error"));
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CStream::Seek(off_t Offset, CSeekOrigin Origin) {
            switch (Origin) {
                case soBeginning:
                case soCurrent:
                    if (Offset < 0)
                        throw ERangeError(_T("Offset check error"));
                    break;

                case soEnd:
                    if (Offset > 0)
                        throw ERangeError(_T("Offset check error"));
                    break;
            }

            return Seek(Offset, (unsigned short) Origin);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStream::ReadBuffer(void *Buffer, size_t Count) {
            if ((Count != 0) && (Read(Buffer, Count) != Count))
                throw ERangeError(_T("Range check error"));
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStream::WriteBuffer(const void *Buffer, size_t Count) {
            if ((Count != 0) && (Write(Buffer, Count) != Count))
                throw EWriteError(_T("Range check error"));
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CStream::CopyFrom(CStream *Source, size_t Count) {
            size_t Result, BufSize, N;
            Pointer Buffer;

            if (Count == 0) {
                Source->Position(0);
                Count = Source->Size();
            }

            Result = Count;
            if (Count > MaxBufSize)
                BufSize = MaxBufSize;
            else
                BufSize = Count;

            Buffer = (Pointer) GHeap->Alloc(HEAP_ZERO_MEMORY, BufSize);
            try {
                while (Count != 0) {
                    if (Count > BufSize) N = BufSize; else N = Count;

                    Source->ReadBuffer(Buffer, N);
                    WriteBuffer(Buffer, N);
                    Count -= N;
                }
            } catch (...) {
                GHeap->Free(0, Buffer, BufSize);
                throw;
            }
            GHeap->Free(0, Buffer, BufSize);

            return Result;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CHandleStream ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CHandleStream::CreateHandle(int AHandle) {
            m_Handle = AHandle;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CHandleStream::SetSize(const size_t NewSize) {
            Seek(NewSize, soBeginning);
            OSCheck((BOOL) Seek(0L, soFromEnd));
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CHandleStream::Read(void *Buffer, size_t Count) {
            ssize_t Result = ::read(m_Handle, Buffer, Count);
            if (!Result)
                Result = 0;
            return (size_t) Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CHandleStream::Write(const void *Buffer, size_t Count) {
            ssize_t Result = ::write(m_Handle, Buffer, Count);
            if (!Result)
                Result = 0;
            return (size_t) Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CHandleStream::Seek(const off_t Offset, unsigned short Origin) {
            return (size_t) ::lseek(m_Handle, Offset, Origin);
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CFileStream -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CFileStream::CFileStream(LPCTSTR lpszFileName, unsigned short Mode) : CHandleStream(INVALID_HANDLE_VALUE) {
            if (Mode == OF_CREATE) {
                CreateHandle(::creat(lpszFileName, FILE_ATTRIBUTE_NORMAL));
                if (m_Handle == INVALID_HANDLE_VALUE)
                    throw EFilerError(errno, _T("Could not create file: \"%s\" error: "), lpszFileName);

            } else {
                CreateHandle(::open(lpszFileName, O_RDWR, FILE_ATTRIBUTE_NORMAL));
                if (m_Handle == INVALID_HANDLE_VALUE)
                    throw EFilerError(errno, _T("Could not open file: \"%s\" error: "), lpszFileName);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        CFileStream::~CFileStream() {
            if (m_Handle != INVALID_HANDLE_VALUE) {
                ::close(m_Handle);
                m_Handle = INVALID_HANDLE_VALUE;
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomMemoryStream ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCustomMemoryStream::CCustomMemoryStream() : CStream() {
            m_Size = 0;
            m_Position = 0;
            m_Memory = nullptr;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCustomMemoryStream::SetPointer(void *Ptr, size_t Size) {
            m_Memory = Ptr;
            m_Size = Size;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CCustomMemoryStream::Read(void *Buffer, size_t Count) {
            size_t Result = 0;
            if ((m_Position >= 0) && (Count >= 0)) {
                Result = m_Size - m_Position;
                if (Result > 0) {
                    if (Result > Count)
                        Result = Count;
                    ::CopyMemory(Buffer, Pointer((size_t) m_Memory + m_Position), Result);
                    m_Position += Result;
                    return Result;
                }
            }

            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CCustomMemoryStream::Seek(off_t Offset, unsigned short Origin) {
            switch (Origin) {
                case soFromBeginning:
                    m_Position = (size_t) Offset;
                    break;

                case soFromCurrent:
                    m_Position += Offset;
                    break;

                case soFromEnd:
                    if (m_Size > labs(Offset))
                        m_Position = m_Size + Offset;
                    else
                        m_Position = m_Size;

                    break;

                default:
                    break;
            }

            return m_Position;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCustomMemoryStream::SaveToStream(CStream *Stream) {
            if (m_Size != 0)
                Stream->WriteBuffer(m_Memory, m_Size);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCustomMemoryStream::SaveToFile(LPCTSTR lpszFileName) {
            CStream *Stream = new CFileStream(lpszFileName, OF_CREATE);
            try {
                SaveToStream(Stream);
            } catch (...) {
            }
            delete Stream;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CMemoryStream ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define MemoryDelta 0x2000

        CMemoryStream::CMemoryStream() : CCustomMemoryStream() {
            m_Capacity = 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        CMemoryStream::~CMemoryStream() {
            Clear();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CMemoryStream::Clear() {
            SetCapacity(0);
            m_Size = 0;
            m_Position = 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CMemoryStream::LoadFromStream(CStream *Stream) {
            size_t Count;
            Stream->Position(0);
            Count = Stream->Size();
            SetSize(Count);

            if (Count != 0)
                Stream->ReadBuffer(Memory(), Count);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CMemoryStream::LoadFromFile(LPCTSTR lpszFileName) {
            CStream *Stream = new CFileStream(lpszFileName, O_RDONLY);
            try {
                LoadFromStream(Stream);
            } catch (...) {
                delete Stream;
                throw;
            }
            delete Stream;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CMemoryStream::SetCapacity(size_t NewCapacity) {
            SetPointer(Realloc(NewCapacity), m_Size);
            m_Capacity = NewCapacity;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CMemoryStream::SetSize(size_t NewSize) {
            inherited::SetSize(NewSize);
            size_t OldPosition = m_Position;
            SetCapacity(NewSize);
            m_Size = NewSize;
            if (OldPosition > NewSize)
                Seek(0, soEnd);
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CMemoryStream::Realloc(size_t &NewCapacity) {
            if ((NewCapacity > 0) && (NewCapacity != m_Size))
                NewCapacity = (NewCapacity + (MemoryDelta - 1)) & ~(MemoryDelta - 1);

            Pointer P = Memory();

            if (NewCapacity != m_Capacity) {
                if (NewCapacity == 0) {
                    P = GHeap->Free(0, Memory(), m_Capacity);
                } else {
                    if (m_Capacity == 0)
                        P = GHeap->Alloc(0, NewCapacity);
                    else
                        P = GHeap->ReAlloc(0, Memory(), NewCapacity, m_Capacity);

                    if (P == nullptr)
                        throw EStreamError(_T("Out of memory while expanding memory stream"));
                }
            }

            return P;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CMemoryStream::Write(const void *Buffer, size_t Count) {
            size_t Pos;
            if ((m_Position >= 0) && (Count >= 0)) {
                Pos = m_Position + Count;
                if (Pos > 0) {
                    if (Pos > m_Size) {
                        if (Pos > m_Capacity)
                            SetCapacity(Pos);
                        m_Size = Pos;
                    }
                    ::CopyMemory(Pointer((size_t) m_Memory + m_Position), Buffer, Count);
                    m_Position = Pos;
                    return Count;
                }
            }
            return 0;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomStringStream ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCustomStringStream::CCustomStringStream() : CStream() {
            m_Size = 0;
            m_Position = 0;
            m_Data = nullptr;
            m_wData = nullptr;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCustomStringStream::SetData(LPTSTR Data, size_t Size) {
            m_Data = Data;
            m_Size = Size;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CCustomStringStream::Read(void *Buffer, size_t Count) {
            size_t Result = 0;
            if ((m_Position >= 0) && (Count >= 0)) {
                Result = m_Size - m_Position;
                if (Result > 0) {
                    if (Result > Count)
                        Result = Count;
                    ::CopyMemory(Buffer, (LPTSTR) m_Data + m_Position, Result);
                    m_Position += Result;
                    return Result;
                }
            }

            return Result;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CCustomStringStream::Seek(off_t Offset, unsigned short Origin) {
            switch (Origin) {
                case soFromBeginning:
                    m_Position = (size_t) Offset;
                    break;

                case soFromCurrent:
                    m_Position += Offset;
                    break;

                case soFromEnd:
                    if (m_Size > labs(Offset))
                        m_Position = m_Size + Offset;
                    else
                        m_Position = m_Size;

                    break;

                default:
                    break;
            }

            return m_Position;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCustomStringStream::SaveToStream(CStream *Stream) {
            if (m_Size != 0)
                Stream->WriteBuffer(m_Data, m_Size);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCustomStringStream::SaveToFile(LPCTSTR lpszFileName) {
            CStream *Stream = new CFileStream(lpszFileName, OF_CREATE);
            try {
                SaveToStream(Stream);
            } catch (...) {
            }
            delete Stream;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CStringStream ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define StringDelta 16

        CStringStream::CStringStream() : CCustomStringStream() {
            m_Capacity = 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        CStringStream::~CStringStream() {
            Clear();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringStream::Clear() {
            SetCapacity(0);
            m_Size = 0;
            m_Position = 0;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringStream::LoadFromStream(CStream *Stream) {
            size_t Count;
            Stream->Position(0);
            Count = Stream->Size();
            SetLength(Count / sizeof(TCHAR));

            if (Count != 0)
                Stream->ReadBuffer(Data(), Count);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringStream::LoadFromFile(LPCTSTR lpszFileName) {
            CStream *Stream = new CFileStream(lpszFileName, O_RDONLY);
            try {
                LoadFromStream(Stream);
            } catch (...) {
                delete Stream;
                throw;
            }
            delete Stream;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringStream::LoadFromFile(const CString& FileName) {
            LoadFromFile(FileName.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringStream::SetCapacity(size_t NewCapacity) {
            SetData(Realloc(NewCapacity), m_Size);
            m_Capacity = NewCapacity;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringStream::SetSize(size_t NewSize) {
            inherited::SetSize(NewSize);
            size_t OldPosition = m_Position;
            SetCapacity(NewSize);
            m_Size = NewSize;
            if (OldPosition > NewSize)
                Seek(0, soEnd);
        };
        //--------------------------------------------------------------------------------------------------------------

        LPTSTR CStringStream::Realloc(size_t &NewCapacity) {
            if ((NewCapacity > 0) && (NewCapacity != m_Size))
                NewCapacity = (NewCapacity + StringDelta) & ~(StringDelta - 1);

            Pointer P = m_Data;

            if (NewCapacity != m_Capacity) {
                if (NewCapacity == 0) {
                    P = GHeap->Free(0, m_Data, m_Capacity);
                } else {
                    if (m_Capacity == 0)
                        P = GHeap->Alloc(HEAP_ZERO_MEMORY, NewCapacity);
                    else
                        P = GHeap->ReAlloc(0, m_Data, NewCapacity, m_Capacity);

                    if (P == nullptr)
                        throw EStreamError(_T("Out of memory while expanding memory stream"));
                }
            }

            return (LPTSTR) P;
        };
        //--------------------------------------------------------------------------------------------------------------

        size_t CStringStream::Write(const void *Buffer, size_t Count) {
            size_t Pos;
            if ((m_Position >= 0) && (Count >= 0)) {
                Pos = m_Position + Count;
                if (Pos > 0) {
                    if (Pos > m_Size) {
                        if (Pos > m_Capacity)
                            SetCapacity(Pos);
                        m_Size = Pos;
                    }
                    ::CopyMemory((LPTSTR) m_Data + m_Position, Buffer, Count);
                    m_Position = Pos;
                    return Count;
                }
            }
            return 0;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomString ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCustomString::CCustomString(): CStringStream() {
            m_Length = 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::Clear() {
            inherited::Clear();
            m_Length = 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::SetLength(size_t NewLength) {
            if ((NewLength > 0) && (NewLength != m_Length)) {
                if (NewLength > 0)
                    SetSize(NewLength * sizeof(TCHAR));
                else
                    SetSize(0);
                m_Length = NewLength;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        TCHAR CCustomString::GetChar(size_t Index) {
            if (Index >= 0 && Index < m_Length)
                return m_Data[Index];
            return '\0';
        }
        //--------------------------------------------------------------------------------------------------------------

        const TCHAR CCustomString::GetChar(size_t Index) const {
            if (Index >= 0 && Index < m_Length)
                return m_Data[Index];
            return '\0';
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::WriteStr(LPCTSTR Str, size_t Length) {

            if (Length == 0) {
                chVERIFY(SUCCEEDED(StringCchLength(Str, _INT_T_LEN, &Length)));
            }

            SetLength(m_Length + Length);
            Write(Str, Length * sizeof(TCHAR));
        }

        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::AddStr(LPCTSTR Str, size_t Length) {
            Seek(0, soEnd);
            WriteStr(Str, Length);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::SetStr(LPCTSTR Str, size_t Length) {
            m_Length = 0;
            Seek(0, soBeginning);
            WriteStr(Str, Length);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::AddChar(TCHAR C, size_t Length) {
            Seek(0, soEnd);
            SetLength(m_Length + Length);
            for (int I = 0; I < Length; ++I)
                Write(&C, sizeof(TCHAR));
        };
        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::SetChar(TCHAR C, size_t Length) {
            m_Length = Length;
            Seek(0, soBeginning);
            for (int I = 0; I < Length; ++I)
                Write(&C, sizeof(TCHAR));
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CCustomString::Truncate() {
            size_t Length;
            chVERIFY(SUCCEEDED(StringCchLength(m_Data, m_Length, &Length)));
            SetLength(Length);
            return Length;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomString::SetSize(size_t NewSize) {
            inherited::SetSize(NewSize);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CString ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CString::CString(): CCustomString(), m_MaxFormatSize(MaxFormatStringLength) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CString::CString(const CString &S) : CString()  {
            Create(S);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString::CString(LPCTSTR Str, size_t Length) : CString() {
            Create(Str, Length);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString::CString(TCHAR C) : CString() {
            Create(C);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString::CString(size_t Length, TCHAR C) : CString() {
            Create(Length, C);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Create(const CString& S) {
            if (!S.IsEmpty())
                SetStr(S.Data(), S.Length());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Create(LPCTSTR Str, size_t Length) {
            if (Assigned(Str))
                SetStr(Str, Length);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Create(TCHAR C) {
            if (C != '\0')
                SetChar(C, 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Create(size_t Length, TCHAR C) {
            if (C != '\0')
              SetChar(C, Length);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Append(const CString& S) {
            if (!S.IsEmpty())
                AddStr(S.Data(), S.Length());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Append(LPCTSTR Str, size_t Length) {
            if (Assigned(Str))
                AddStr(Str, Length);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Append(TCHAR C) {
            if (C != '\0')
                AddChar(C, 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Append(size_t Length, TCHAR C) {
            if (C != '\0')
              AddChar(C, Length);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CString::Compare(const CString& S) const {
            if (IsEmpty())
                return -1;
            if (S.IsEmpty())
                return 1;
            return CompareString(c_str(), S.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        int CString::Compare(const std::string& S) const {
            if (IsEmpty())
                return -1;
            if (S.empty())
                return 1;
            return CompareString(c_str(), S.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        int CString::Compare(LPCTSTR Str) const {
            if (IsEmpty())
                return -1;
            if (Str == nullptr)
                return 1;
            return CompareString(c_str(), Str);
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CString::Find(const CString& S, size_t Pos) const {
            if (m_Size == 0) return npos;
            LPCTSTR P = strstr(Str() + Pos, S.Str());
            return (P == nullptr) ? npos : (P - Str());
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CString::Find(LPCTSTR Str, size_t Pos) const {
            if (m_Size == 0) return npos;
            LPCTSTR P = strstr(this->Str() + Pos, Str);
            return (P == nullptr) ? npos : (P - this->Str());
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CString::Find(LPCTSTR Str, size_t Pos, size_t Length) const {
            if (m_Size == 0) return npos;
            CString L(Str, Length);
            return Find(L, Pos);
        }
        //--------------------------------------------------------------------------------------------------------------

        size_t CString::Find(TCHAR C, size_t Pos) const {
            if (m_Size == 0) return npos;
            LPCTSTR P = strchr(Str() + Pos, C);
            return (P == nullptr) ? npos : (P - Str());
        }
        //--------------------------------------------------------------------------------------------------------------

        //This may be equal to len or to length()-pos (if the string value is shorter than pos+len).
        size_t CString::Copy(LPTSTR Str, size_t Len, size_t Pos) const {
            //chVERIFY(SUCCEEDED(StringCchCopy(Str, Len, Data() + Pos)));
            //return ((Pos + Len) < Length()) ? Length() - Pos : Len;
            size_t Result = 0;
            size_t Length = Size();
            if ((Pos >= 0) && (Len >= 0) && (Length > Pos)) {
                Result = Length - Pos;
                if (Result > 0) {
                    if (Result > Len)
                        Result = Len;
                    ::CopyMemory((LPTSTR) Str, Data() + Pos, Result);
                }
            }
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Format(LPCTSTR pszFormat, ...) {
            va_list args;
            va_start(args, pszFormat);
            Format(pszFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CString::Format(LPCTSTR pszFormat, va_list argList) {
            CString LStr;
            size_t cchDest;
            LStr.SetLength(m_MaxFormatSize);
            cchDest = LStr.GetSize();
            chVERIFY(SUCCEEDED(StringPCchVPrintf(LStr.Data(), &cchDest, pszFormat, argList)));
            AddStr(LStr.Data(), cchDest / sizeof(TCHAR));
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CString::SubString(size_t StartIndex, size_t Length) const {
            CString L;
            if (StartIndex < GetLength()) {
                Length = (Length == npos) ? GetLength() - StartIndex : Length;
                L.SetLength(Length);
                Copy(L.Data(), Length, StartIndex);
            }
            return L;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CString::Upper() const {
            CString L;
            L.SetLength(Length());
            UpperCase(L.Data(), L.Length(), Data());
            return L;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CString::Lower() const {
            CString L;
            L.SetLength(Length());
            LowerCase(L.Data(), L.Length(), Data());
            return L;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CString::Trim(TCHAR TrimChar) const {
            size_t I, L;
            L = Length() - 1;
            I = 0;
            if (IsEmpty() || ((GetChar(I) > TrimChar) && (GetChar(L) > TrimChar)))
                return *this;
            while ((I <= L) && (GetChar(I) <= TrimChar)) I++;
                if (I > L) {
                    CString S;
                    return S;
                }
            while (GetChar(L) <= TrimChar) L--;
            return SubString(I, L - I + 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CString::TrimLeft(TCHAR TrimChar) const {
            size_t I;
            I = 0;
            while (!IsEmpty() && ((GetChar(I) <= TrimChar))) I++;
            if (I > 0)
                return SubString(I);
            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CString::TrimRight(TCHAR TrimChar) const {
            size_t I;
            I = Length() - 1;
            if (IsEmpty() || (GetChar(I) > TrimChar)) {
                return *this;
            } else {
                while ((I >= 0) && (GetChar(I) <= TrimChar)) I--;
                return SubString(0, I + 1);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CStrings --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CStrings::CStrings(): CPersistent(this) {
            m_LineBreak = sLineBreak;
            m_Delimiter = sDelimiter;
            m_QuoteChar = sQuoteChar;
            m_NameValueSeparator = sNameValueSeparator;
            m_StrictDelimiter = false;
            m_UpdateCount = 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetValue(const CString &Name, const CString &Value) {
            int I = IndexOfName(Name);

            if (!Value.IsEmpty()) {
                if (I < 0) I = Add(CString());
                Put(I, Name + NameValueSeparator() + Value);
            } else {
                if (I >= 0)
                    Delete(I);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetValue(CStrings::reference Name, CStrings::reference Value) {
            CString LName(Name);

            int I = IndexOfName(Name);

            if (Assigned(Value)) {
                if (I < 0) I = Add(CString());
                Put(I, LName + NameValueSeparator() + Value);
            } else {
                if (I >= 0)
                    Delete(I);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        TCHAR CStrings::GetDelimiter() {
            return m_Delimiter;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetDelimiter(TCHAR Value) {
            if (m_Delimiter != Value)
                m_Delimiter = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCTSTR CStrings::GetLineBreak() const {
            return m_LineBreak;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetLineBreak(LPCTSTR Value) {
            if (*m_LineBreak != *Value)
                m_LineBreak = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        TCHAR CStrings::GetQuoteChar() {
            return m_QuoteChar;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetQuoteChar(const TCHAR Value) {
            if (m_QuoteChar != Value)
                m_QuoteChar = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        TCHAR CStrings::GetNameValueSeparator() const {
            return m_NameValueSeparator;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetNameValueSeparator(const TCHAR Value) {
            if (m_NameValueSeparator != Value)
                m_NameValueSeparator = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CStrings::GetStrictDelimiter() {
            return m_StrictDelimiter;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetStrictDelimiter(bool Value) {
            if (m_StrictDelimiter != Value)
                m_StrictDelimiter = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString CStrings::GetValueFromIndex(int Index) {
            size_t      SepPos;
            CString     R;

            if (Index >= 0) {
                const CString &S = Get(Index);
                SepPos = S.Find(NameValueSeparator());
                if (SepPos != CString::npos) {
                    R.SetLength(S.Length() - SepPos - 1);
                    S.Copy(R.Data(), R.Length(), SepPos + 1);
                }
            }

            return R;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetValueFromIndex(int Index, const CString &Value) {
            if (!Value.IsEmpty()) {
                if (Index < 0) Index = Add(CString());
                Put(Index, Names(Index) + NameValueSeparator() + Value);
            } else {
                if (Index >= 0)
                    Delete(Index);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetValueFromIndex(int Index, CStrings::reference Value) {
            if (Assigned(Value)) {
                if (Index < 0) Index = Add(CString());
                Put(Index, Names(Index) + NameValueSeparator() + Value);
            } else {
                if (Index >= 0)
                    Delete(Index);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Error(const CString &Msg, int Data) {
            throw ExceptionFrm(Msg.c_str(), Data);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::GetCapacity() const noexcept {
            return Count();
        }
        //--------------------------------------------------------------------------------------------------------------

        CObject *CStrings::GetObject(int Index) {
            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CObject *CStrings::GetObject(int Index) const {
            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString& CStrings::GetText(CString& Value) const {
            int I;
            size_t L, LineBreakLen;
            LPCTSTR LB = LineBreak();

            Value.Clear();

            L = 0;
            LineBreakLen = strlen(LB);

            for (I = 0; I < GetCount(); ++I) {
                const CString &S = Get(I);
                if (!S.IsEmpty())
                    L += S.Size();
                L += LineBreakLen;
            }

            Value.SetLength(L);
            Value.Position(0);

            for (I = 0; I < GetCount(); ++I) {
                const CString &S = Get(I);
                if (!S.IsEmpty())
                    Value.WriteBuffer(S.Data(), S.Size());
                Value.WriteBuffer(LB, LineBreakLen);
            }

            return Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Put(int Index, const CString &S) {
            CObject *TempObject;
            TempObject = GetObject(Index);
            Delete(Index);
            InsertObject(Index, S, TempObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Put(int Index, reference Str) {
            CObject *TempObject;
            TempObject = GetObject(Index);
            Delete(Index);
            InsertObject(Index, Str, TempObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::PutObject(int Index, CObject *AObject) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetCapacity(int NewCapacity) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetText(const CString &Value) {
            SetTextStr(Value.c_str(), Value.size());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetUpdateState(bool Updating) {

        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::CompareStrings(const CString &S1, const CString &S2) {
            return S1.Compare(S2);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::Add(const CString &S) {
            int Result = GetCount();
            Insert(Result, S);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::Add(reference Str) {
            int Result = GetCount();
            Insert(Result, Str);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::Add(TCHAR C) {
            int Result = GetCount();
            Insert(Result, C);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::AddObject(const CString &S, CObject *AObject) {
            int Result = Add(S);
            PutObject(Result, AObject);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::AddObject(reference Str, CObject *AObject) {
            int Result = Add(Str);
            PutObject(Result, AObject);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::AddObject(TCHAR C, CObject *AObject) {
            int Result = Add(C);
            PutObject(Result, AObject);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::AddPair(const CString &Name, const CString &Value) {
            int Result = GetCount();
            Insert(Result, Name + NameValueSeparator() + Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::AddPair(CStrings::reference Name, CStrings::reference Value) {
            CString LName(Name);
            int Result = GetCount();
            Insert(Result, LName + NameValueSeparator() + Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::AddPair(CStrings::reference Name, const CString &Value) {
            CString LName(Name);
            int Result = GetCount();
            Insert(Result, LName + NameValueSeparator() + Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::AddPair(const CString &Name, CStrings::reference Value) {
            CString LValue(Value);
            int Result = GetCount();
            Insert(Result, Name + NameValueSeparator() + LValue);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Append(const CString &S) {
            Add(S);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Append(reference Str) {
            Add(Str);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::AddStrings(const CStrings& AStrings) {
            BeginUpdate();
            try {
                for (int I = 0; I < AStrings.Count(); ++I)
                    AddObject(AStrings.Strings(I), AStrings.Objects(I));
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Assign(const CStrings& Source) {

            BeginUpdate();
            try {
                Clear();

                m_NameValueSeparator = Source.m_NameValueSeparator;
                m_QuoteChar = Source.m_QuoteChar;
                m_Delimiter = Source.m_Delimiter;
                m_LineBreak = Source.m_LineBreak;
                m_StrictDelimiter = Source.m_StrictDelimiter;

                AddStrings(Source);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();

            //inherited::Assign(Source);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetStrings(const CStrings& Source) {
            BeginUpdate();
            try {
                Clear();
                AddStrings(Source);
            } catch (...) {
            }
            EndUpdate();
        }

        //--------------------------------------------------------------------------------------------------------------
        void CStrings::BeginUpdate() {
            if (m_UpdateCount == 0)
                SetUpdateState(true);
            m_UpdateCount++;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::EndUpdate() {
            m_UpdateCount--;
            if (m_UpdateCount == 0)
                SetUpdateState(false);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CStrings::Equals(CStrings *Strings) {
            int I, Count;
            Count = GetCount();
            if (Count != Strings->GetCount())
                return false;
            for (I = 0; I < Count; ++I)
                if (Get(I) != Strings->Get(I))
                    return false;
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Exchange(int Index1, int Index2) {
            CObject *TempObject;

            BeginUpdate();
            try {
                CString &TempString = Strings(Index1);
                TempObject = Objects(Index1);
                Strings(Index1, Strings(Index2));
                Objects(Index1, Objects(Index2));
                Strings(Index2, TempString);
                Objects(Index2, TempObject);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOf(const CString &S) {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I) == S)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOf(const CString &S) const {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I) == S)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOf(reference Str) {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I) == Str)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOf(reference Str) const {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I) == Str)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOfName(const CString &Name) {
            for (int I = 0; I < GetCount(); ++I) {
                if (GetName(I) == Name)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOfName(const CString &Name) const {
            for (int I = 0; I < GetCount(); ++I) {
                if (GetName(I) == Name)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOfName(reference Name) {
            for (int I = 0; I < GetCount(); ++I) {
                if (GetName(I) == Name)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOfName(reference Name) const {

            for (int I = 0; I < GetCount(); ++I) {
                if (GetName(I) == Name)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOfObject(CObject *AObject) {
            for (int I = 0; I < GetCount(); ++I)
                if (GetObject(I) == AObject)
                    return I;
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStrings::IndexOfObject(CObject *AObject) const {
            for (int I = 0; I < GetCount(); ++I)
                if (GetObject(I) == AObject)
                    return I;
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::InsertObject(int Index, const CString &S, CObject *AObject) {
            Insert(Index, S);
            PutObject(Index, AObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::InsertObject(int Index, reference Str, CObject *AObject) {
            Insert(Index, Str);
            PutObject(Index, AObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::InsertObject(int Index, TCHAR C, CObject *AObject) {
            Insert(Index, C);
            PutObject(Index, AObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::LoadFromFile(LPCTSTR lpszFileName) {
            CStream *Stream = new CFileStream(lpszFileName, O_RDONLY);
            try {
                LoadFromStream(Stream);
            } catch (...) {
                delete Stream;
                throw;
            }
            delete Stream;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::LoadFromStream(CStream *Stream) {

            size_t BufSize, Count;
            LPTSTR Buffer;

            Count = Stream->Size() - Stream->Position();

            if (Count > MaxBufSize)
                BufSize = MaxBufSize;
            else
                BufSize = Count;

            Buffer = (LPTSTR) GHeap->Alloc(HEAP_ZERO_MEMORY, BufSize);
            BeginUpdate();
            try {
                Stream->Read(Buffer, BufSize);
                SetTextStr(Buffer, BufSize);
            } catch (...) {
                GHeap->Free(0, Buffer, BufSize);
                EndUpdate();
                throw;
            }
            GHeap->Free(0, Buffer, BufSize);
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::Move(int CurIndex, int NewIndex) {
            CObject *TempObject;

            if (CurIndex != NewIndex) {
                BeginUpdate();
                try {
                    CString &TempString = Get(CurIndex);
                    TempObject = GetObject(CurIndex);
                    PutObject(CurIndex, nullptr);
                    Delete(CurIndex);
                    InsertObject(NewIndex, TempString, TempObject);
                } catch (...) {
                    EndUpdate();
                    throw;
                }
                EndUpdate();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SaveToFile(LPCTSTR lpszFileName) {
            CStream *Stream = new CFileStream(lpszFileName, OF_CREATE);
            try {
                SaveToStream(Stream);
            } catch (...) {
            }
            delete Stream;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SaveToStream(CStream *Stream) {
            CString S;
            GetText(S);
            Stream->WriteBuffer(S.Data(), S.Size());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStrings::SetTextStr(LPCTSTR Text, size_t Size) {

            size_t LineBreakLen;
            LPCTSTR P, E, LB;

            BeginUpdate();
            try {
                Clear();
                P = Text;
                E = P + Size;
                if (Assigned(P)) {
                    Add(CString());
                    if (LineBreak() == sLineBreak) {
                        while (P < E) {
                            if ((*P == '\n') || (*P == m_Delimiter)) {
                                Add(CString());
                            } else {
                                if (!IsCtl(*P) && (*P != m_QuoteChar)) {
                                    Last().Append(*P);
                                }
                            }
                            P++;
                        }
                    } else {
                        LineBreakLen = strlen(LineBreak());
                        LB = strstr(P, LineBreak());
                        while (P < E) {
                            if ((P == LB) || (*P == m_Delimiter)) {
                                Add(CString());
                                if (P == LB) {
                                    P += LineBreakLen;
                                    LB = strstr(P, LineBreak());
                                } else {
                                    if (P == (Text + Size))
                                        break;
                                    P++;
                                }
                            } else {
                                if (!IsCtl(*P) && (*P != m_QuoteChar)) {
                                    Last().Append(*P);
                                }
                                if (P == (Text + Size))
                                    break;
                                P++;
                            }
                        }
                    }
                }
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CStrings::GetTextStr(LPTSTR Buffer, size_t &SizeBuf) {
            CString S;
            GetText(S);
            size_t Size = SizeBuf;
            SizeBuf = S.Size();
            if (Size >= SizeBuf) {
                S.ReadBuffer(Buffer, SizeBuf);
                Size = SizeBuf;
            }
            return (SizeBuf == Size);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CStringList -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CStringList::CStringList() : CHeapComponent(), CStrings() {
            m_nCount = 0;
            m_nCapacity = 0;
            m_fOwnsObjects = false;
            m_pList = nullptr;
        };
        //--------------------------------------------------------------------------------------------------------------

        CStringList::CStringList(bool AOwnsObjects) : CHeapComponent(), CStrings(), m_fOwnsObjects(AOwnsObjects) {
            m_nCount = 0;
            m_nCapacity = 0;
            m_pList = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CStringList::~CStringList() {
            Clear();
            if (m_pList != nullptr)
                GHeap->Free(0, m_pList, m_nCapacity * sizeof(PStringItem));
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Grow() {
            int Delta;

            if (m_nCapacity > 64)
                Delta = m_nCapacity / 4;
            else if (m_nCapacity > 8)
                Delta = 16;
            else
                Delta = 4;

            SetCapacity(m_nCapacity + Delta);
        };
        //--------------------------------------------------------------------------------------------------------------

        CString &CStringList::Get(int Index) {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index]->String;
        };
        //--------------------------------------------------------------------------------------------------------------

        const CString &CStringList::Get(int Index) const {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index]->String;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Put(int Index, const CString &S) {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            m_pList[Index]->String = S;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Put(int Index, reference Str) {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            m_pList[Index]->String = Str;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::PutObject(int Index, CObject *AObject) {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            m_pList[Index]->Object = AObject;
        }
        //--------------------------------------------------------------------------------------------------------------

        CObject *CStringList::GetObject(int Index) {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index]->Object;
        }
        //--------------------------------------------------------------------------------------------------------------

        CObject *CStringList::GetObject(int Index) const {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index]->Object;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CStringList::GetName(int Index) const {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            size_t L = CString::npos;
            CStringItem *R = m_pList[Index];

            R->Name.Clear();
            if (!R->String.IsEmpty())
                L = R->String.Find(NameValueSeparator());

            if (L != CString::npos) {
                R->Name.SetLength(L);
                R->String.Copy(R->Name.Data(), L);
            }

            return R->Name;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CStringList::GetValue(const CString &Name) const {
            CStringItem *P;

            int Index = IndexOfName(Name);
            if (Index != -1) {
                P = m_pList[Index];
                if (P->String.Length() - Name.Length() > 0) {
                    P->Value.SetLength(P->String.Length() - Name.Length() - 1);
                    if (P->Value.Length() > 0)
                        P->String.Copy(P->Value.Data(), P->Value.Length(), Name.Length() + 1);
                }

                return P->Value;
            }

            return m_NullValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CStringList::GetValue(reference Name) const {
            CStringItem *P;
            size_t Length;

            chVERIFY(SUCCEEDED(StringCchLength(Name, _INT_T_LEN, &Length)));

            int Index = IndexOfName(Name);
            if (Index != -1) {
                P = m_pList[Index];
                if (P->String.Length() - Length > 0) {
                    P->Value.SetLength(P->String.Length() - Length - 1);
                    if (P->Value.Length() > 0)
                        P->String.Copy(P->Value.Data(), P->Value.Length(), Length + 1);
                }

                return P->Value;
            }

            return m_NullValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Assign(const CStrings& Source) {
            m_fOwnsObjects = false;
            CStrings::Assign(Source);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Clear() {
            int I;
            CObject** Temp = nullptr;
            size_t TempCount = 0;

            if (m_nCount != 0) {

                // If the list owns the Objects gather them and free after the list is disposed
                if (OwnsObjects()) {
                    Temp = new CObject* [m_nCount];
                    for (I = 0; I < m_nCount; ++I) {
                        Temp[I] = m_pList[I]->Object;
                        TempCount++;
                    }
                }

                for (I = 0; I < m_nCount; ++I)
                    delete m_pList[I];

                m_nCount = 0;
                SetCapacity(0);

                // Free the objects that were owned by the list
                if (Temp != nullptr) {
                    for (I = 0; I < TempCount; ++I)
                        FreeAndNil(Temp[I]);
                    delete[] Temp;
                }
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::SetCapacity(int NewCapacity) {
            if ((NewCapacity < m_nCount) || (NewCapacity > MaxListSize))
                throw ExceptionFrm(SListCapacityError, NewCapacity);

            if (NewCapacity != m_nCapacity) {
                if (NewCapacity == 0) {
                    m_pList = (PStringItemList) GHeap->Free(0, (PStringItemList) m_pList,
                            m_nCapacity * sizeof(PStringItem));
                } else {
                    if (m_nCapacity == 0) {
                        m_pList = (PStringItemList) GHeap->Alloc(0, NewCapacity * sizeof(PStringItem));
                    } else {
                        m_pList = (PStringItemList) GHeap->ReAlloc(0, (PStringItemList) m_pList,
                                NewCapacity * sizeof(PStringItem), m_nCapacity * sizeof(PStringItem));
                    }

                    if (m_pList == nullptr)
                        throw Delphi::Exception::Exception(_T("Out of memory while expanding memory heap"));
                }

                m_nCapacity = NewCapacity;
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Update(int Index) {
            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Delete(int Index) {

            CObject *Obj;

            if ((Index < 0) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            // If this list owns its objects then free the associated TObject with this index
            if (OwnsObjects())
                Obj = m_pList[Index]->Object;
            else
                Obj = nullptr;

            delete m_pList[Index];
            m_nCount--;

            if (Index < m_nCount)
                MoveMemory(&m_pList[Index], &m_pList[Index + 1], (m_nCount - Index) * sizeof(PStringItem));

            if (Obj != nullptr)
                FreeAndNil(Obj);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStringList::Add(const CString &S) {
            return AddObject(S, nullptr);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStringList::Add(reference Str) {
            return AddObject(Str, nullptr);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStringList::Add(TCHAR C) {
            return AddObject(C, nullptr);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CStringList::AddObject(const CString &S, CObject *AObject) {
            int Index = GetCount();
            InsertItem(Index, S, AObject);
            return Index;
        };
        //--------------------------------------------------------------------------------------------------------------

        int CStringList::AddObject(reference Str, CObject *AObject) {
            int Index = GetCount();
            InsertItem(Index, Str, AObject);
            return Index;
        };
        //--------------------------------------------------------------------------------------------------------------

        int CStringList::AddObject(TCHAR C, CObject *AObject) {
            int Index = GetCount();
            InsertItem(Index, C, AObject);
            return Index;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Insert(int Index, const CString &S) {
            InsertObject(Index, S, nullptr);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Insert(int Index, reference Str) {
            InsertObject(Index, Str, nullptr);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::Insert(int Index, TCHAR C) {
            InsertObject(Index, C, nullptr);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::InsertObject(int Index, const CString &S, CObject *AObject) {
            if ((Index < 0) || (Index > m_nCount))
                throw ExceptionFrm(SListIndexError, Index);
            InsertItem(Index, S, AObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::InsertObject(int Index, reference Str, CObject *AObject) {
            if ((Index < 0) || (Index > m_nCount))
                throw ExceptionFrm(SListIndexError, Index);
            InsertItem(Index, Str, AObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::InsertObject(int Index, TCHAR C, CObject *AObject) {
            if ((Index < 0) || (Index > m_nCount))
                throw ExceptionFrm(SListIndexError, Index);
            InsertItem(Index, C, AObject);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::InsertItem(int Index, const CString &S, CObject *AObject) {

            if (m_nCount == m_nCapacity)
                Grow();

            if (Index < m_nCount)
                MoveMemory(&m_pList[Index], &m_pList[Index + 1], (m_nCount - Index) * sizeof(PStringItem));

            m_pList[Index] = new CStringItem;

            m_pList[Index]->String = S;
            m_pList[Index]->Object = AObject;

            m_nCount++;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::InsertItem(int Index, reference Str, CObject *AObject) {

            if (m_nCount == m_nCapacity)
                Grow();

            if (Index < m_nCount)
                MoveMemory(&m_pList[Index], &m_pList[Index + 1], (m_nCount - Index) * sizeof(PStringItem));

            m_pList[Index] = new CStringItem;

            m_pList[Index]->String = Str;
            m_pList[Index]->Object = AObject;

            m_nCount++;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CStringList::InsertItem(int Index, TCHAR C, CObject *AObject) {

            if (m_nCount == m_nCapacity)
                Grow();

            if (Index < m_nCount)
                MoveMemory(&m_pList[Index], &m_pList[Index + 1], (m_nCount - Index) * sizeof(PStringItem));

            m_pList[Index] = new CStringItem;

            m_pList[Index]->String = C;
            m_pList[Index]->Object = AObject;

            m_nCount++;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CQueueItem ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CQueueItem::CQueueItem(CCollection *ACollection, Pointer AQueue) : CCollectionItem(ACollection) {
            m_pQueue = AQueue;
            m_pItems = new CList();
        };
        //--------------------------------------------------------------------------------------------------------------

        CQueueItem::~CQueueItem() {
            delete m_pItems;
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CQueueItem::Get(int Index) {
            return m_pItems->Items(Index);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CQueueItem::Put(int Index, Pointer Value) {
            m_pItems->Insert(Index, Value);
        };
        //--------------------------------------------------------------------------------------------------------------

        int CQueueItem::Add(Pointer Item) {
            return m_pItems->Add(Item);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CQueueItem::Insert(int Index, Pointer Item) {
            m_pItems->Insert(Index, Item);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CQueueItem::Delete(int Index) {
            m_pItems->Delete(Index);
        };
        //--------------------------------------------------------------------------------------------------------------

        int CQueueItem::Remove(Pointer Item) {
            int Index = IndexOf(Item);
            if (Index >= 0)
                Delete(Index);
            return Index;
        };
        //--------------------------------------------------------------------------------------------------------------

        int CQueueItem::IndexOf(Pointer Item) {
            return m_pItems->IndexOf(Item);
        }
        //--------------------------------------------------------------------------------------------------------------

        Pointer CQueueItem::First() {
            Pointer P = nullptr;

            if (m_pItems->Count() > 0)
                P = m_pItems->Items(0);

            return P;
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CQueueItem::Last() {
            Pointer P = nullptr;

            if (m_pItems->Count() > 0)
                P = m_pItems->Items(m_pItems->Count() - 1);

            return P;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CQueue ----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CQueue::CQueue() : CCollection(this) {

        };
        //--------------------------------------------------------------------------------------------------------------

        CQueueItem *CQueue::GetItem(int Index) {
            return (CQueueItem *) inherited::GetItem(Index);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CQueue::SetItem(int Index, const CQueueItem *Item) {
            inherited::SetItem(Index, (CQueueItem *) Item);
        };
        //--------------------------------------------------------------------------------------------------------------

        int CQueue::IndexOf(Pointer Queue) {
            int Index = 0;

            while ((Index < Count()) && (Items(Index)->Queue() != Queue))
                Index++;

            if (Index == Count())
                Index = -1;

            return Index;
        };
        //--------------------------------------------------------------------------------------------------------------

        CQueueItem *CQueue::Add(Pointer Queue) {
            auto Item = new CQueueItem(this, Queue);
            inherited::Added((CCollectionItem *) Item);
            return Item;
        };
        //--------------------------------------------------------------------------------------------------------------

        CQueueItem *CQueue::Insert(int Index, Pointer Queue) {
            CQueueItem *Item = Add(Queue);
            Item->Index(Index);
            return Item;
        };
        //--------------------------------------------------------------------------------------------------------------

        int CQueue::Remove(Pointer Queue) {
            int Index = IndexOf(Queue);
            if (Index >= 0)
                Delete(Index);
            return Index;
        };
        //--------------------------------------------------------------------------------------------------------------

        CQueueItem *CQueue::First() {
            return GetItem(0);
        };
        //--------------------------------------------------------------------------------------------------------------

        CQueueItem *CQueue::Last() {
            return GetItem(Count() - 1);
        };
        //--------------------------------------------------------------------------------------------------------------

        int CQueue::AddToQueue(Pointer Queue, Pointer P) {
            CQueueItem *Item = nullptr;
            int I = IndexOf(Queue);

            if (I == -1)
                Item = Add(Queue);
            else
                Item = GetItem(I);

            return Item->Add(P);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CQueue::InsertToQueue(Pointer Queue, int Index, Pointer P) {
            CQueueItem *Item = nullptr;
            int I = IndexOf(Queue);

            if (I == -1)
                Item = Add(Queue);
            else
                Item = GetItem(I);

            Item->Insert(Index, P);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CQueue::RemoveFromQueue(Pointer Queue, Pointer P) {
            CQueueItem *Item = nullptr;
            int I = IndexOf(Queue);

            if (I >= 0) {
                Item = GetItem(I);
                Item->Remove(P);
                if (Item->Count() == 0)
                    Delete(I);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CQueue::FirstItem(Pointer Queue) {
            Pointer P = nullptr;
            int Index = IndexOf(Queue);

            if (Index >= 0)
                P = Items(Index)->First();

            return P;
        };
        //--------------------------------------------------------------------------------------------------------------

        Pointer CQueue::LastItem(Pointer Queue) {
            Pointer P = nullptr;
            int Index = IndexOf(Queue);

            if (Index >= 0)
                P = Items(Index)->Last();

            return P;
        };

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        inline void AddSysError() {
            if (GSysErrorCount == 0) {
                GSysError = GSysError->CreateSysError();
            }

            GSysErrorCount++;
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void RemoveSysError() {
            GSysErrorCount--;

            if (GSysErrorCount == 0) {
                GSysError->DestroySysError();
                GSysError = nullptr;
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CSysError -------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CSysError::Create() {

            Clear();

            char *M;
            char S[MAX_ERROR_STR + 1];

            for (int I = 0; I < SYS_ERRNO_COUNT; I++) {
                if (DefaultLocale.Locale() == LC_GLOBAL_LOCALE) {
                    M = ::strerror_r(I, S, MAX_ERROR_STR);
                } else {
                    M = ::strerror_l(I, DefaultLocale.Locale());
                }

                Add(M);
            };
        }

        const CString& CSysError::StrError(int AError) {
            if (AError >= 0 && AError < Count())
                return Strings(AError);
            return m_UnknownError;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSysErrorComponent ----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSysErrorComponent::CSysErrorComponent() {
            AddSysError();
        };
        //--------------------------------------------------------------------------------------------------------------

        CSysErrorComponent::~CSysErrorComponent() {
            RemoveSysError();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CThread ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        LPVOID ThreadProc(LPVOID lpParameter) {

            auto Thread = static_cast<CThread *> (lpParameter);

            bool FreeThread;

            Thread->m_nThreadId = (pid_t) syscall(SYS_gettid);

            pthread_mutex_lock(&Thread->m_Lock);
            while (Thread->m_bSuspended) {
                pthread_cond_wait(&Thread->m_Suspend, &Thread->m_Lock);
            }
            pthread_mutex_unlock(&Thread->m_Lock);

            try {
                if (!Thread->Terminated())
                    Thread->Execute();
            }
            catch (...) {
            }

            FreeThread = Thread->m_bFreeOnTerminate;
            //Result = Thread->m_nReturnValue;

            Thread->m_bFinished = true;
            Thread->DoTerminate();

            if (FreeThread) {
                delete Thread;
            }

            pthread_exit((LPVOID) "Success");
        }
        //--------------------------------------------------------------------------------------------------------------

        CThread::CThread(bool CreateSuspended)
        {
            AddThread();

            m_hHandle = {0};
            m_nThreadId = 0;
            m_bTerminated = false;
            m_bFreeOnTerminate = false;
            m_bFinished = false;

            m_nReturnValue = 0;

            m_OnTerminate = nullptr;

            m_bSuspended = CreateSuspended;
            m_bCreateSuspended = CreateSuspended;

            m_Synchronize = new CSynchronizeRecord;

            pthread_cond_init(&m_Suspend, nullptr);

            pthread_mutex_init(&m_Lock, nullptr);
            //m_Lock = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

            pthread_attr_init(&m_hAttr);

//            pthread_attr_setdetachstate(&m_hAttr, PTHREAD_CREATE_DETACHED);

            m_nThreadId = pthread_create(&m_hHandle, &m_hAttr, &ThreadProc, (PVOID) this);

            if ( m_nThreadId != 0 )
                throw Exception::Exception(_T("Thread creation error."));
        };
        //--------------------------------------------------------------------------------------------------------------

        CThread::~CThread()
        {
            if ( (m_nThreadId != 0) && (!m_bFinished) )
            {
                Terminate();
                if ( m_bCreateSuspended )
                    Resume();
                WaitFor();
            }

            pthread_attr_destroy(&m_hAttr);
            pthread_mutex_destroy(&m_Lock);
            pthread_cond_destroy(&m_Suspend);

            delete m_Synchronize;

            RemoveThread();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::AfterConstruction()
        {
            if ( !m_bCreateSuspended )
                Resume();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::CheckThreadError(int ErrCode)
        {
            throw EOSError(ErrCode, _T("Thread error: "));
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::CheckThreadError(bool Success)
        {
            if ( !Success )
                CheckThreadError(errno);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::CallOnTerminate()
        {
            if ( m_OnTerminate != nullptr )
                m_OnTerminate((CObject *) this);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::DoTerminate()
        {
            CallOnTerminate();
        };
        //--------------------------------------------------------------------------------------------------------------

        CThreadPriority CThread::GetPriority()
        {
            return tpNormal;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::SetPriority(CThreadPriority Value)
        {
            //
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::SetSuspended(bool Value)
        {
            if ( Value != m_bSuspended )
            {
                if ( Value )
                    Suspend();
                else
                    Resume();
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::Suspend()
        {
            m_bSuspended = true;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::Resume()
        {
            if (m_bSuspended) {
                m_bSuspended = false;
                pthread_cond_signal(&m_Suspend);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::Terminate()
        {
            m_bTerminated = true;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::CreateSyncList()
        {
            GSyncList = new CList();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::Synchronize(PSynchronizeRecord ASyncRec)
        {
            auto SyncProc = new CSyncProc;

            if ( ::getppid() == MainThreadID )
            {
                ASyncRec->Method();
            }
            else
            {
                if ( GSyncList != nullptr && SyncProc != nullptr )
                {
                    //SyncProc->Signal = ::CreateEvent(nullptr, TRUE, FALSE, nullptr);

                    if ( SyncProc->Signal != nullptr )
                    {
                        try
                        {
                            pthread_mutex_lock(&GThreadLock);
                            try
                            {
                                if ( GSyncList == nullptr )
                                    CreateSyncList();

                                SyncProc->SyncRec = ASyncRec;
                                GSyncList->Add(SyncProc);

                                SignalSyncEvent();

                                if ( WakeMainThread  )
                                    WakeMainThread((CObject *) SyncProc->SyncRec->Thread);

                                pthread_mutex_unlock(&GThreadLock);
                                try
                                {
                                    //WaitForSingleObject(SyncProc->Signal, INFINITE);
                                } catch (...) {
                                }
                                pthread_mutex_lock(&GThreadLock);
                            } catch (...) {
                            }
                            pthread_mutex_unlock(&GThreadLock);

                        } catch (...) {
                        }
                        //CloseHandle(SyncProc->Signal);
                    }

                    if ( ASyncRec->SynchronizeException != nullptr )
                        throw ASyncRec->SynchronizeException;
                }
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::Synchronize(CThreadMethod *AMethod)
        {
            m_Synchronize->Thread = this;
            m_Synchronize->SynchronizeException = nullptr;
            m_Synchronize->Method = AMethod;

            Synchronize(m_Synchronize);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThread::Synchronize(CThread *AThread, CThreadMethod *AMethod)
        {
            CSynchronizeRecord SyncRec;

            if ( AThread != nullptr )
            {
                AThread->Synchronize(AMethod);
            }
            else
            {
                SyncRec.Thread = nullptr;
                SyncRec.SynchronizeException = nullptr;
                SyncRec.Method = AMethod;
                Synchronize(&SyncRec);
            };
        };
        //--------------------------------------------------------------------------------------------------------------

        int CThread::WaitFor() {
            void *Result;
            pthread_join(m_hHandle, &Result);
            DebugMessage("thread %d exited with '%s'\n", m_nThreadId, (char *) Result);
            return ReturnValue();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CThreadList -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CThreadList::CThreadList()
        {
            pthread_mutex_init(&m_Lock, nullptr);
            m_List = new CList;
            m_Duplicates = dupIgnore;
        };
        //--------------------------------------------------------------------------------------------------------------

        CThreadList::~CThreadList()
        {
            FreeList();
            pthread_mutex_destroy(&m_Lock);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThreadList::FreeList()
        {
            LockList();
            try
            {
                FreeAndNil(m_List);
            } catch (...) {
                UnlockList();
                throw;
            }
            UnlockList();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThreadList::Add(CThread *Item)
        {
            LockList();
            try
            {
                if ( Assigned(m_List) && ((Duplicates() == dupAccept) || (m_List->IndexOf(Item) == -1)) )
                    m_List->Add(Item);
                else if ( Duplicates() == dupError )
                    throw Exception::Exception(_T("List does not allow duplicates"));
            } catch (...) {
                UnlockList();
                throw;
            }
            UnlockList();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThreadList::Clear()
        {
            LockList();
            try
            {
                m_List->Clear();
            } catch (...) {
                UnlockList();
                throw;
            }
            UnlockList();
        };
        //--------------------------------------------------------------------------------------------------------------

        CList *CThreadList::LockList()
        {
            pthread_mutex_lock(&m_Lock);
            return m_List;
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThreadList::Remove(CThread *Item)
        {
            LockList();
            try
            {
                m_List->Remove(Item);
            } catch (...) {
                UnlockList();
                throw;
            }
            UnlockList();
        };
        //--------------------------------------------------------------------------------------------------------------

        void CThreadList::UnlockList()
        {
            pthread_mutex_unlock(&m_Lock);
        };
        //--------------------------------------------------------------------------------------------------------------
    }
}
}
