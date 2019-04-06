/*++

Library name:

  libdelphi

Module Name:

  Templates.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_TEMPLATES_HPP
#define DELPHI_TEMPLATES_HPP

extern "C++" {

namespace Delphi {

    namespace Classes {

        template<class ClassName>
        class TList : public CObject, public CHeapComponent {
            typedef ClassName *PClassName;
            typedef PClassName *PClassNameList;

        private:

            PClassNameList m_pList;

            int m_nCount;
            int m_nCapacity;

            void QuickSort(PClassNameList SortList, int L, int R, ListSortCompare SCompare);

        protected:

            PClassName Get(int Index);

            PClassName Get(int Index) const;

            void Put(int Index, PClassName Item);

            ClassName &GetItem(int Index);

            const ClassName &GetItem(int Index) const;

            void PutItem(int Index, const ClassName &Item);

            virtual void Grow();

            virtual void Notify(PClassName Item, ListNotification Action);

        public:

            TList();

            ~TList() override;

            virtual void Clear();

            void Exchange(int Index1, int Index2);

            TList<ClassName> *Expand();

            PClassName Extract(PClassName Item);

            int IndexOf(PClassName Item) const;

            int IndexOf(const ClassName &Item) const;

            void Insert(int Index, const ClassName &Item);

            ClassName &First();

            const ClassName &First() const;

            ClassName &Last();

            const ClassName &Last() const;

            int Add(const ClassName &Item);

            void Delete(int Index);

            void Move(int CurIndex, int NewIndex);

            int Remove(const ClassName &Item);

            void Pack();

            void Sort(ListSortCompare Compare);

            void Assign(TList<ClassName> *ListA, ListAssignOp AOperator = laCopy, TList<ClassName> *ListB = nullptr);

            int GetCapacity() const noexcept { return m_nCapacity; }

            void SetCapacity(int NewCapacity);

            int GetCount() const noexcept { return m_nCount; }

            void SetCount(int NewCount);

            int Capacity() const noexcept { return GetCapacity(); }

            int Count() const noexcept { return GetCount(); }

            PClassNameList GetList() const { return m_pList; }

            ClassName &Items(int Index) { return GetItem(Index); }

            const ClassName &Items(int Index) const { return GetItem(Index); }

            void Items(int Index, const ClassName &Value) { PutItem(Index, Value); }

            ClassName &operator[](int Index) { return GetItem(Index); }

            const ClassName &operator[](int Index) const { return GetItem(Index); }

        }; // TList<ClassName>

        //--------------------------------------------------------------------------------------------------------------

        //-- TList<ClassName> ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        TList<ClassName>::TList() {
            m_nCount = 0;
            m_nCapacity = 0;
            m_pList = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        TList<ClassName>::~TList() {
            Clear();
            if (m_pList != nullptr)
                GHeap->Free(0, m_pList, m_nCapacity * sizeof(PClassName));
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        ClassName *TList<ClassName>::Get(int Index) {
            if ((Index < 0) || (m_pList == nullptr) || (Index >= m_nCount))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        ClassName *TList<ClassName>::Get(int Index) const {
            if ((Index < 0) || (m_pList == nullptr) || (Index >= m_nCount))
                throw Delphi::Exception::ExceptionFrm(SListIndexError, Index);

            return m_pList[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Put(int Index, PClassName Item) {
            PClassName Temp;

            if ((Index < 0) || (m_pList == nullptr) || (Index >= m_nCount))
                throw Delphi::Exception::ExceptionFrm(SListIndexError, Index);

            if (Item != m_pList[Index]) {
                Temp = m_pList[Index];
                m_pList[Index] = Item;

                if (Temp != nullptr)
                    Notify(Temp, lnDeleted);

                if (Item != nullptr)
                    Notify(Temp, lnAdded);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        ClassName &TList<ClassName>::GetItem(int Index) {
            return *Get(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        const ClassName &TList<ClassName>::GetItem(int Index) const {
            return *Get(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::PutItem(int Index, const ClassName &Item) {
            PClassName Temp;

            if ((Index < 0) || (m_pList == nullptr) || (Index >= m_nCount))
                throw Delphi::Exception::ExceptionFrm(SListIndexError, Index);

            if (&Item != m_pList[Index]) {
                Temp = m_pList[Index];

                if (Temp != nullptr)
                    Notify(Temp, lnDeleted);

                *m_pList[Index] = Item;

                Notify(Temp, lnAdded);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Grow() {
            int Delta;

            if (m_nCapacity > 64)
                Delta = m_nCapacity / 4;
            else if (m_nCapacity > 8)
                Delta = 16;
            else
                Delta = 4;

            SetCapacity(m_nCapacity + Delta);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Notify(PClassName Item, ListNotification Action) {

        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::SetCapacity(int NewCapacity) {
            if ((NewCapacity < m_nCount) || (NewCapacity > MaxListSize))
                throw Delphi::Exception::ExceptionFrm(SListCapacityError, NewCapacity);

            if (NewCapacity != m_nCapacity) {
                if (NewCapacity == 0) {
                    m_pList = (PClassNameList) GHeap->Free(0, m_pList, m_nCapacity * sizeof(PClassName));
                } else {
                    if (m_nCapacity == 0) {
                        m_pList = (PClassNameList) GHeap->Alloc(0, NewCapacity * sizeof(PClassName));
                    } else {
                        m_pList = (PClassNameList) GHeap->ReAlloc(0, m_pList, NewCapacity * sizeof(PClassName),
                                                                  m_nCapacity * sizeof(PClassName));
                    }

                    if (m_pList == nullptr)
                        throw Delphi::Exception::Exception(_T("Out of memory while expanding memory heap"));
                }

                m_nCapacity = NewCapacity;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::SetCount(int NewCount) {
            if ((NewCount < 0) || (NewCount > MaxListSize))
                throw Delphi::Exception::ExceptionFrm(SListCountError, NewCount);

            if (NewCount > m_nCapacity)
                SetCapacity(NewCount);

            if (NewCount > m_nCount)
                ZeroMemory(&m_pList[m_nCount], (NewCount - m_nCount) * sizeof(PClassName));
            else
                for (int i = m_nCount - 1; i >= NewCount; i--)
                    Delete(i);

            m_nCount = NewCount;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Clear() {
            SetCount(0);
            SetCapacity(0);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Exchange(int Index1, int Index2) {
            PClassName Item;

            if ((Index1 < 0) || (Index1 >= m_nCount))
                throw Delphi::Exception::ExceptionFrm(SListIndexError, Index1);
            if ((Index2 < 0) || (Index2 >= m_nCount))
                throw Delphi::Exception::ExceptionFrm(SListIndexError, Index2);
            Item = m_pList[Index1];
            m_pList[Index1] = m_pList[Index2];
            m_pList[Index2] = Item;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        TList<ClassName> *TList<ClassName>::Expand() {
            if (m_nCount == m_nCapacity)
                Grow();

            return this;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        ClassName *TList<ClassName>::Extract(PClassName Item) {
            int Index;
            PClassName Result = nullptr;

            Index = IndexOf(Item);

            if (Index >= 0) {
                Result = m_pList[Index];
                m_pList[Index] = nullptr;

                m_nCount--;
                if (Index < m_nCount)
                    MoveMemory(&m_pList[Index], &m_pList[Index + 1], (m_nCount - Index) * sizeof(PClassName));

                if (Result != nullptr)
                    Notify(Result, lnExtracted);
            }

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        int TList<ClassName>::IndexOf(PClassName Item) const {
            int Result = 0;

            while ((Result < m_nCount) && (m_pList[Result] != Item))
                Result++;

            if (Result == m_nCount)
                Result = -1;

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        int TList<ClassName>::IndexOf(const ClassName &Item) const {
            int Result = 0;

            while ((Result < m_nCount) && (*m_pList[Result] != Item))
                Result++;

            if (Result == m_nCount)
                Result = -1;

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        ClassName &TList<ClassName>::First() {
            return GetItem(0);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        const ClassName &TList<ClassName>::First() const {
            return GetItem(0);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        ClassName &TList<ClassName>::Last() {
            return GetItem(m_nCount - 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        const ClassName &TList<ClassName>::Last() const {
            return GetItem(m_nCount - 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        int TList<ClassName>::Add(const ClassName &Item) {
            int Result = m_nCount;

            if (Result == m_nCapacity)
                Grow();

            Insert(Result, Item);

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Insert(int Index, const ClassName &Item) {
            if ((Index < 0) || (Index > m_nCount))
                throw Delphi::Exception::ExceptionFrm(SListIndexError, Index);

            if (m_nCount == m_nCapacity)
                Grow();

            if (Index < m_nCount)
                ::MoveMemory(&m_pList[Index + 1], &m_pList[Index], (m_nCount - Index) * sizeof(PClassName));

            m_pList[Index] = new ClassName();

            *m_pList[Index] = Item;

            m_nCount++;

            Notify(m_pList[Index], lnAdded);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Delete(int Index) {
            if ((Index < 0) || (Index >= m_nCount))
                throw Delphi::Exception::ExceptionFrm(SListIndexError, Index);

            auto Temp = m_pList[Index];
            m_nCount--;

            if (Index < m_nCount)
                MoveMemory(&m_pList[Index], &m_pList[Index + 1], (m_nCount - Index) * sizeof(PClassName));

            if (Temp != nullptr)
                Notify(Temp, lnDeleted);

            delete Temp;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Move(int CurIndex, int NewIndex) {
            PClassName Item;

            if (CurIndex != NewIndex) {
                if ((NewIndex < 0) || (NewIndex >= m_nCount))
                    throw Delphi::Exception::ExceptionFrm(SListIndexError, NewIndex);

                Item = m_pList[CurIndex];
                m_pList[CurIndex] = nullptr;
                Delete(CurIndex);
                m_pList[NewIndex] = nullptr;
                m_pList[NewIndex] = Item;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        int TList<ClassName>::Remove(const ClassName &Item) {
            int Result = IndexOf(Item);
            if (Result >= 0)
                Delete(Result);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Pack() {
            for (int i = m_nCount - 1; i >= 0; i--)
                if (m_pList[i] == nullptr)
                    Delete(i);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Sort(ListSortCompare Compare) {
            if ((m_pList != nullptr) && (m_nCount > 0))
                QuickSort(m_pList, 0, GetCount() - 1, Compare);
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::Assign(TList<ClassName> *ListA, ListAssignOp AOperator, TList<ClassName> *ListB) {
            int I;
            TList<ClassName> *LSource;

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
                        if (LSource->IndexOf(GetItem(I)) == -1)
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
                    TList<ClassName> LTemp; // Temp holder of 4 byte values
                    LTemp.SetCapacity(LSource->Count());
                    for (I = 0; I < LSource->Count(); I++)
                        if (IndexOf(LSource->Items(I)) == -1)
                            LTemp.Add(LSource->Items(I));

                    for (I = Count() - 1; I >= 0; I--)
                        if (LSource->IndexOf(GetItem(I)) != -1)
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
                        if (LSource->IndexOf(GetItem(I)) != -1)
                            Delete(I);
                    break;
                }

                    // 12345, 346 = 6 : only those unique to dest
                case laDestUnique: {
                    TList<ClassName> LTemp;
                    LTemp.SetCapacity(LSource->Count());
                    for (I = LSource->Count() - 1; I >= 0; I--)
                        if (IndexOf(LSource->Items(I)) == -1)
                            LTemp.Add(LSource->Items(I));
                    Assign(&LTemp);
                    break;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        template<class ClassName>
        void TList<ClassName>::QuickSort(PClassNameList SortList, int L, int R, ListSortCompare SCompare) {
            int I, J;
            PClassName P, T;

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
    }
}
}

#endif //DELPHI_TEMPLATES_HPP
