/*++

Library name:

  libdelphi

Module Name:

  JSON.cpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "delphi.hpp"
#include "JSON.hpp"

extern "C++" {

namespace Delphi {

    namespace Json {

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSON -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJSON::CJSON(): CJSON(this, jvtNull) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON::CJSON(CJSONValueType ValueType): CJSON(this, ValueType) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON::CJSON(CPersistent *AOwner): CJSON(AOwner, jvtNull) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON::CJSON(const CString &AString): CJSON() {
            StringToJson(AString);
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON::CJSON(CPersistent *AOwner, CJSONValueType ValueType): CPersistent(AOwner), m_ValueType(ValueType) {
            m_Value = nullptr;
            m_UpdateCount = 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON::~CJSON() {
            delete m_Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::SetUpdateState(bool Updating) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::BeginUpdate() {
            if (m_UpdateCount == 0)
                SetUpdateState(true);
            m_UpdateCount++;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::EndUpdate() {
            m_UpdateCount--;
            if (m_UpdateCount == 0)
                SetUpdateState(false);
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONObject *CJSON::CreateObject() {
            m_ValueType = jvtObject;
            m_Value = new CJSONObject(this);
            return (CJSONObject *) m_Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONArray *CJSON::CreateArray() {
            m_ValueType = jvtArray;
            m_Value = new CJSONArray(this);
            return (CJSONArray *) m_Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::Assign(const CJSON& Source) {

            Clear();

            m_ValueType = Source.ValueType();

            if (Assigned(Source.Value())) {
                if (Source.Value()->ValueType() == jvtObject) {
                    CreateObject()->Assign(Source.Object());
                }

                if (Source.Value()->ValueType() == jvtArray) {
                    CreateArray()->Assign(Source.Array());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::Concat(const CJSON& Source) {
            if (Assigned(Source.Value()) && (ValueType() == Source.Value()->ValueType())) {
                if (Source.Value()->ValueType() == jvtObject) {
                    CreateObject()->Concat(Source.Object());
                }

                if (Source.Value()->ValueType() == jvtArray) {
                    CreateArray()->Concat(Source.Array());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSON::GetCount() const noexcept {
            if (Assigned(m_Value)) {
                if (m_Value->ValueType() == jvtObject)
                    return Object().Count();

                if (m_Value->ValueType() == jvtArray)
                    return Array().Count();
            }

            return 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::Clear() {
            if (Assigned(m_Value)) {
                if (m_Value->ValueType() == jvtObject)
                    return Object().Clear();

                if (m_Value->ValueType() == jvtArray)
                    return Array().Clear();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString& CJSON::JsonToString(CString& String) const {

            if (ValueType() == jvtObject) {
                String = "{";

                for (int i = 0; i < Count(); i++) {

                    if (i > 0) {
                        String += ", ";
                    }

                    String += "\"";
                    String += Members(i).String();
                    String += "\": ";

                    Object()[i].ToString(String);
                }

                String += "}";
            }

            if (ValueType() == jvtArray) {

                String = "[";

                for (int i = 0; i < Count(); i++) {

                    if (i > 0) {
                        String += ", ";
                    }

                    Array()[i].ToString(String);
                }

                String += "]";
            }

            return String;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::StringToJson(const CString &Value) {
            if (!Value.IsEmpty())
              StrToJson(Value.c_str(), Value.Size());
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSON::JsonToStr(LPTSTR ABuffer, size_t &ASize) {
            CString S;
            JsonToString(S);
            size_t Size = ASize;
            ASize = S.Size();
            if (Size >= ASize) {
                S.ReadBuffer(ABuffer, ASize);
                Size = ASize;
            }
            return (ASize == Size);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::StrToJson(LPCTSTR ABuffer, size_t ASize) {
            CJSONParser LParser(this);
            CJSONParserResult R;

            BeginUpdate();
            try {
                if (Assigned(ABuffer)) {
                    Clear();
                    R = LParser.Parse((LPTSTR) ABuffer, ABuffer + ASize);
                    if (!R.result) {
                        throw Exception::EJSONParseSyntaxError(_T("JSON Parser syntax error in position %d, char \"%c\""), R.pos, ABuffer[R.pos]);
                    }
                }
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }

        //--------------------------------------------------------------------------------------------------------------

        void CJSON::LoadFromFile(LPCTSTR lpszFileName) {
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

        void CJSON::LoadFromStream(CStream *Stream) {

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
                StrToJson(Buffer, BufSize);
            } catch (...) {
                GHeap->Free(0, Buffer, BufSize);
                EndUpdate();
                throw;
            }
            GHeap->Free(0, Buffer, BufSize);
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::SaveToFile(LPCTSTR lpszFileName) {
            CStream *Stream = new CFileStream(lpszFileName, OF_CREATE);
            try {
                SaveToStream(Stream);
            } catch (...) {
            }
            delete Stream;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::SaveToStream(CStream *Stream) {
            CString S;
            JsonToString(S);
            Stream->WriteBuffer(S.Data(), S.Size());
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONMember &CJSON::Members(int Index) {
            return Object().Members(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONMember &CJSON::Members(int Index) const {
            return Object().Members(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSON::Members(int Index, const CJSONMember &Value) {
            return Object().Members(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSON::operator[](int Index) {
            return Array()[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSON::operator[](int Index) const {
            return Array()[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSON::operator[](CJSON::reference String) {
            return Object().Values(String);
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSON::operator[](const CString &String) const {
            return Object().Values(String);
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSON::operator[](const CString &String) {
            return Object().Values(String);
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSON::operator[](CJSON::reference String) const {
            return Object().Values(String);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONElements ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJSONElements::CJSONElements(CPersistent *AOwner, CJSONValueType ValueType): CJSON(AOwner, ValueType) {
            m_LineBreak = sLineBreak;
            m_Delimiter = ',';
            m_QuoteChar = '"';
            m_StrictDelimiter = false;
            m_CurrentIndex = -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONElements::GetValueFromIndex(int Index) {
            return Get(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONElements::GetValueFromIndex(int Index) const {
            return Get(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::SetValueFromIndex(int Index, const CJSONValue& Value) {
            if (!Value.IsEmpty()) {
                if (Index < 0) Index = Add(Value);
                Put(Index, Value);
            } else {
                if (Index >= 0)
                    Delete(Index);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::SetCurrentIndex(int Index) {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            m_CurrentIndex = Index;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::Error(const CString &Msg, int Data) {
            throw ExceptionFrm(Msg.c_str(), Data);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONElements::GetCapacity() const noexcept {
            return Count();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::Put(int Index, const CJSONValue &Value) {
            Delete(Index);
            Insert(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::SetCapacity(int NewCapacity) {

        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONElements::Add(const CJSONValue &Value) {
            int Result = GetCount();
            Insert(Result, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONElements::Add(const CString &Value) {
            int Result = GetCount();
            Insert(Result, CJSONValue(Value));
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONElements::Add(reference Value) {
            int Result = GetCount();
            Insert(Result, CJSONValue(Value));
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::AddElements(const CJSONElements &Value) {
            BeginUpdate();
            try {
                for (int I = 0; I < Value.Count(); ++I)
                    Add(Value[I]);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::Assign(const CJSONElements &Source) {
            BeginUpdate();
            try {
                Clear();

                m_QuoteChar = Source.m_QuoteChar;
                m_Delimiter = Source.m_Delimiter;
                m_LineBreak = Source.m_LineBreak;
                m_StrictDelimiter = Source.m_StrictDelimiter;

                AddElements(Source);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::Concat(const CJSONElements &Source) {
            BeginUpdate();
            try {
                AddElements(Source);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::SetElements(const CJSONElements &Source) {
            BeginUpdate();
            try {
                Clear();
                AddElements(Source);
            } catch (...) {
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSONElements::Equals(const CJSONElements &Value) {
            int I, Count;
            Count = GetCount();
            if (Count != Value.GetCount())
                return false;
            for (I = 0; I < Count; ++I)
                if (Get(I) != Value.Get(I))
                    return false;
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::Exchange(int Index1, int Index2) {

            BeginUpdate();
            try {
                CJSONValue &Temp = Values(Index1);
                Values(Index1, Values(Index2));
                Values(Index2, Temp);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONElements::IndexOf(const CJSONValue &Value) const {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I) == Value)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONElements::Move(int CurIndex, int NewIndex) {
            if (CurIndex != NewIndex) {
                BeginUpdate();
                try {
                    auto Temp = Get(CurIndex);
                    Delete(CurIndex);
                    Insert(NewIndex, Temp);
                } catch (...) {
                    EndUpdate();
                    throw;
                }
                EndUpdate();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString& CJSONElements::JsonToString(CString &String) const {
            String += "[";

            for (int i = 0; i < Count(); i++) {
                if (i > 0) {
                    String += ", ";
                }
                Values(i).ToString(String);
            }

            String += "]";

            return String;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONMembers ----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJSONMembers::CJSONMembers(CPersistent *AOwner, CJSONValueType ValueType): CJSON(AOwner, ValueType) {
            m_LineBreak = sLineBreak;
            m_Delimiter = sDelimiter;
            m_QuoteChar = sQuoteChar;
            m_NameValueSeparator = sNameValueSeparator;
            m_StrictDelimiter = false;
            m_CurrentIndex = -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::SetValue(const CString &String, const CJSONValue &Value) {
            int I = IndexOfString(String);

            if (!Value.IsEmpty()) {
                if (I < 0) I = Add(CJSONMember(Value));
                PutPair(I, String, Value);
            } else {
                if (I >= 0)
                    Delete(I);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::SetValue(reference String, const CJSONValue &Value) {
            int I = IndexOfString(String);

            if (!Value.IsEmpty()) {
                if (I < 0) I = Add(CJSONMember(Value));
                PutPair(I, String, Value);
            } else {
                if (I >= 0)
                    Delete(I);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONMembers::GetValueFromIndex(int Index) {
            return Get(Index).Value();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONMembers::GetValueFromIndex(int Index) const {
            return Get(Index).Value();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::SetValueFromIndex(int Index, const CJSONValue &Value) {
            if (!Value.IsEmpty()) {
                if (Index < 0)
                    Index = Add(CJSONMember(Value));
                const CString &String = Strings(Index);
                PutPair(Index, String, Value);
            } else {
                if (Index >= 0)
                    Delete(Index);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::SetCurrentIndex(int Index) {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            m_CurrentIndex = Index;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::Error(const CString &Msg, int Data) {
            throw ExceptionFrm(Msg.c_str(), Data);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::GetCapacity() const noexcept {
            return Count();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString& CJSONMembers::JsonToString(CString &String) const {
            String += "{";

            for (int i = 0; i < Count(); i++) {

                if (i > 0) {
                    String += ", ";
                }

                String += "\"";
                String += Members(i).String();
                String += "\": ";

                Members(i).Value().ToString(String);
            }

            String += "}";

            return String;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::Put(int Index, const CJSONMember &Value) {
            Delete(Index);
            Insert(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::PutPair(int Index, const CString &String, const CJSONValue& Value) {
            Delete(Index);
            InsertPair(Index, String, Value);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::PutPair(int Index, reference String, const CJSONValue& Value) {
            Delete(Index);
            InsertPair(Index, String, Value);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::SetCapacity(int NewCapacity) {

        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::Add(const CJSONMember &Value) {
            int Result = GetCount();
            Insert(Result, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::AddPair(const CString &String, const CJSONValue &Value) {
            int Result = GetCount();
            InsertPair(Result, String, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::AddPair(reference String, const CJSONValue &Value) {
            int Result = GetCount();
            InsertPair(Result, String, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::AddPair(const CString &String, const CString &Value) {
            int Result = GetCount();
            InsertPair(Result, String, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::AddPair(reference String, const CString &Value) {
            int Result = GetCount();
            InsertPair(Result, String, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::AddPair(reference String, int Value) {
            int Result = GetCount();
            InsertPair(Result, String, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::AddPair(reference String, float Value) {
            int Result = GetCount();
            InsertPair(Result, String, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::AddPair(reference String, double Value) {
            int Result = GetCount();
            InsertPair(Result, String, Value);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::AddMembers(const CJSONMembers& AMembers) {
            BeginUpdate();
            try {
                for (int I = 0; I < AMembers.Count(); ++I)
                    Add(AMembers.Members(I));
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::Assign(const CJSONMembers& Source) {

            BeginUpdate();
            try {
                Clear();

                m_NameValueSeparator = Source.m_NameValueSeparator;
                m_QuoteChar = Source.m_QuoteChar;
                m_Delimiter = Source.m_Delimiter;
                m_LineBreak = Source.m_LineBreak;
                m_StrictDelimiter = Source.m_StrictDelimiter;

                AddMembers(Source);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::Concat(const CJSONMembers& Source) {

            BeginUpdate();
            try {
                AddMembers(Source);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::SetMembers(const CJSONMembers& Source) {
            BeginUpdate();
            try {
                Clear();
                AddMembers(Source);
            } catch (...) {
            }
            EndUpdate();
        }

        //--------------------------------------------------------------------------------------------------------------

        bool CJSONMembers::Equals(const CJSONMembers &Members) {
            int I, Count;
            Count = GetCount();
            if (Count != Members.GetCount())
                return false;
            for (I = 0; I < Count; ++I)
                if (Get(I) != Members.Get(I))
                    return false;
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::Exchange(int Index1, int Index2) {
            BeginUpdate();
            try {
                const CJSONMember &Temp = Members(Index1);
                Members(Index1, Members(Index2));
                Members(Index2, Temp);
            } catch (...) {
                EndUpdate();
                throw;
            }
            EndUpdate();
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::IndexOf(const CJSONMember &Value) const {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I) == Value)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::IndexOfString(const CString &String) const {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I).String() == String)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONMembers::IndexOfString(reference String) const {
            for (int I = 0; I < GetCount(); ++I) {
                if (Get(I).String() == String)
                    return I;
            }

            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONMembers::Move(int CurIndex, int NewIndex) {
            if (CurIndex != NewIndex) {
                BeginUpdate();
                try {
                    const CJSONMember &Temp = Get(CurIndex);
                    Delete(CurIndex);
                    Insert(NewIndex, Temp);
                } catch (...) {
                    EndUpdate();
                    throw;
                }
                EndUpdate();
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONValue ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONValue::Get(int Index) {
            if (ValueType() == jvtObject)
                return AsObject()[Index];

            if (ValueType() == jvtArray)
                return AsArray()[Index];

            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONValue::Get(int Index) const {
            if (ValueType() == jvtObject)
                return AsObject()[Index];

            if (ValueType() == jvtArray)
                return AsArray()[Index];

            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONValue::Put(int Index, const CJSONValue &Value) {
            if (ValueType() == jvtObject)
                AsObject()[Index] = Value;

            if (ValueType() == jvtArray)
                AsArray()[Index] = Value;

            *this = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONValue::GetValue(const CString &String) {
            if (ValueType() == jvtObject)
                return AsObject()[String];

            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONValue::GetValue(const CString &String) const {
            if (ValueType() == jvtObject)
                return AsObject()[String];

            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONValue::GetValue(reference String) {
            if (ValueType() == jvtObject)
                return AsObject()[String];

            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONValue::GetValue(reference String) const {
            if (ValueType() == jvtObject)
                return AsObject()[String];

            return *this;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONValue::SetValue(const CString &String, const CJSONValue &Value) {
            if (ValueType() == jvtObject)
                AsObject()[String] = Value;

            *this = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONValue::SetValue(CJSONValue::reference String, const CJSONValue &Value) {
            if (ValueType() == jvtObject)
                AsObject()[String] = Value;

            *this = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONValue::Assign(const CJSONValue &Value) {
            inherited::Assign(Value);
            m_Data = Value.m_Data;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONValue::Compare(const CJSONValue &Value) const {

            if (IsEmpty())
                return -1;

            if (Value.IsEmpty())
                return 1;

            if (ValueType() == Value.ValueType()) {

                switch (ValueType()) {
                    case jvtObject:
                    case jvtArray:
                        if (Count() == Value.Count())
                            return 0;
                        if (Count() > Value.Count())
                            return 1;
                        return -1;

                    case jvtString:
                        return AsSiring().Compare(Value.AsSiring());

                    case jvtNumber:
                        if (AsDouble() == Value.AsDouble())
                            return 0;
                        if (AsDouble() > Value.AsDouble())
                            return 1;
                        return -1;

                    case jvtBoolean:
                        if (AsBoolean() == Value.AsBoolean())
                            return 0;
                        if (AsBoolean())
                            return 1;
                        return -1;

                    case jvtNull:
                        return 0;
                }
            }

            return incorrect_type;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString& CJSONValue::JsonToString(CString &String) const {
            switch (ValueType()) {
                case jvtObject:
                    AsObject().ToString(String);
                    break;

                case jvtArray:
                    AsArray().ToString(String);
                    break;

                case jvtString:
                    String += "\"";
                    String += Data();
                    String += "\"";
                    break;

                case jvtNumber:
                    String += Data();
                    break;

                case jvtBoolean:
                    if (AsBoolean()) {
                        String += "true";
                    } else {
                        String += "false";
                    }
                    break;

                case jvtNull:
                    String += "null";
                    break;
            }

            return String;
        }
        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONArray ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void QuickSort(CJSONValueList& SortList, int L, int R, CJSONListSortCompare SCompare) {
            int I, J;
            CJSONValue T;

            do {
                I = L;
                J = R;
                const CJSONValue& P = SortList[(L + R) >> 1];

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
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONArray::CJSONArray() : CJSONArray(this) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONArray::CJSONArray(CPersistent *AOwner) : CJSONElements(AOwner, jvtArray) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONArray::~CJSONArray() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONArray::Get(int Index) {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONArray::Get(int Index) const {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::Put(int Index, const CJSONValue &Value) {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            m_pList[Index] = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONArray::Add(const CJSONValue &Value) {
            return m_pList.Add(Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONArray::Add(const CString &Value) {
            return m_pList.Add(CJSONValue(Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONArray::Add(reference Value) {
            return m_pList.Add(CJSONValue(Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::Insert(int Index, const CJSONValue &Value) {
            m_pList.Insert(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::SetCapacity(int NewCapacity) {
            m_pList.SetCapacity(NewCapacity);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONArray::GetCapacity() const noexcept {
            return m_pList.Capacity();
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONArray::GetCount() const noexcept {
            return m_pList.Count();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::Assign(const CJSONElements &Source) {
            Clear();
            for (int I = 0; I < Source.Count(); ++I) {
                Add(Source[I]);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::Concat(const CJSONElements &Source) {
            for (int I = 0; I < Source.Count(); ++I) {
                Add(Source[I]);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::Clear() {
            m_pList.Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::Delete(int Index) {
            m_pList.Delete(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONArray::Sort(CJSONListSortCompare Compare) {
            if (m_pList.Count() > 0)
                QuickSort(m_pList, 0, GetCount() - 1, Compare);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONObject -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJSONObject::CJSONObject(): CJSONObject(this) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONObject::CJSONObject(CPersistent *AOwner): CJSONMembers(AOwner, jvtObject) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONObject::~CJSONObject() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CJSONObject::GetString(int Index) const {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index].String();
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONObject::GetValue(const CString &String) {
            int Index = IndexOfString(String);
            if (Index != -1) {
                return m_pList[Index].Value();
            }

            return m_NullValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONObject::GetValue(const CString &String) const {
            int Index = IndexOfString(String);
            if (Index != -1) {
                return m_pList[Index].Value();
            }

            return m_NullValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONObject::GetValue(reference String) {
            int Index = IndexOfString(String);
            if (Index != -1) {
                return m_pList[Index].Value();
            }

            return m_NullValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONValue &CJSONObject::GetValue(reference String) const {
            int Index = IndexOfString(String);
            if (Index != -1) {
                return m_pList[Index].Value();
            }

            return m_NullValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONMember &CJSONObject::Get(int Index) {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        const CJSONMember &CJSONObject::Get(int Index) const {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            return m_pList[Index];
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::Put(int Index, const CJSONMember &Value) {
            if ((Index < 0) || (Index >= GetCount()))
                throw ExceptionFrm(SListIndexError, Index);

            m_pList[Index] = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::Add(const CJSONMember &Value) {
            return m_pList.Add(Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::AddPair(const CString& String, const CJSONValue &Value) {
            return m_pList.Add(CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::AddPair(reference String, const CJSONValue &Value) {
            return m_pList.Add(CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::AddPair(const CString& String, const CString &Value) {
            return m_pList.Add(CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::AddPair(reference String, const CString &Value) {
            return m_pList.Add(CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::AddPair(reference String, int Value) {
            return m_pList.Add(CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::AddPair(reference String, float Value) {
            return m_pList.Add(CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::AddPair(reference String, double Value) {
            return m_pList.Add(CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::Insert(int Index, const CJSONMember &Value) {
            return m_pList.Insert(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::InsertPair(int Index, const CString &String, const CJSONValue &Value) {
            m_pList.Insert(Index, CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::InsertPair(int Index, reference String, const CJSONValue &Value) {
            m_pList.Insert(Index, CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::InsertPair(int Index, const CString &String, const CString &Value) {
            m_pList.Insert(Index, CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::InsertPair(int Index, reference String, const CString &Value) {
            m_pList.Insert(Index, CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::InsertPair(int Index, reference String, int Value) {
            m_pList.Insert(Index, CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::InsertPair(int Index, reference String, float Value) {
            m_pList.Insert(Index, CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::InsertPair(int Index, reference String, double Value) {
            m_pList.Insert(Index, CJSONMember(String, Value));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONObject::SetCapacity(int NewCapacity) {
            m_pList.SetCapacity(NewCapacity);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::GetCapacity() const noexcept {
            return m_pList.Capacity();
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONObject::GetCount() const noexcept {
            return m_pList.Count();
        }

        void CJSONObject::Assign(const CJSONMembers &Source) {
            CJSONMembers::Assign(Source);
        }

        void CJSONObject::Clear() {

        }

        void CJSONObject::Update(int Index) {

        }

        void CJSONObject::Delete(int Index) {

        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONParser -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        const TCHAR _value_true[]  = _T("true");
        const TCHAR _value_false[] = _T("false");
        const TCHAR _value_null[]  = _T("null");
        //--------------------------------------------------------------------------------------------------------------

        CJSONParser::CJSONParser(CJSON *Json) : CObject() {
            m_Json = Json;

            m_State = json_start;
            m_Result = -1;
            m_CharIndex = 0;

            m_pJsonList = new CList();
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONParser::~CJSONParser() {
            delete m_pJsonList;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONParser::Reset() {
            m_State = json_start;
            m_Result = -1;
            m_CharIndex = 0;

            m_pJsonList->Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONParser::DeleteLastJson() {
            m_pJsonList->Extract(m_pJsonList->Last());
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON &CJSONParser::CurrentJson() {
            return *(CJSON *) m_pJsonList->Last();
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONMember &CJSONParser::CurrentMember() {
            return CurrentObject().Last();
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONObject &CJSONParser::CurrentObject() {
            if (CurrentJson().ValueType() != jvtObject)
                throw Exception::Exception(_T("Invalid JSON type!"));
            return *(CJSONObject *) &CurrentJson();
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONArray &CJSONParser::CurrentArray() {
            if (CurrentJson().ValueType() != jvtArray)
                throw Exception::Exception(_T("Invalid JSON type!"));
            return *(CJSONArray *) &CurrentJson();
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONValue &CJSONParser::CurrentValue() {
            if (CurrentJson().ValueType() == jvtObject)
                return CurrentObject().Last().Value();
            return CurrentArray().Last();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJSONParser::CreateValue(CJSONValueType ValueType) {
            if (CurrentJson().ValueType() == jvtObject) {
                switch (ValueType) {
                    case jvtObject:
                        m_pJsonList->Add(CurrentValue().CreateObject());
                        break;
                    case jvtArray:
                        m_pJsonList->Add(CurrentValue().CreateArray());
                        break;
                    default:
                        CurrentValue().ValueType(ValueType);
                        break;
                }
            } else if (CurrentJson().ValueType() == jvtArray) {
                switch (ValueType) {
                    case jvtObject:
                    case jvtArray:
                        CurrentArray().Add(CJSONValue(ValueType));
                        m_pJsonList->Add(CurrentArray().Last().Value());
                        break;
                    default:
                        CurrentArray().Add(CJSONValue(ValueType));
                        break;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSONParserResult CJSONParser::Parse(LPTSTR ABegin, LPCTSTR AEnd) {
            LPCTSTR Start = ABegin;

            if (m_Json->Count() > 0 ) {
                m_State = value_start;
            }

            while ((m_Result == -1) && (ABegin != AEnd)) {
                m_Result = Consume(*ABegin++);
            }
            return {m_Result, size_t(ABegin - Start - 1)};
        }
        //--------------------------------------------------------------------------------------------------------------

        int CJSONParser::Consume(u_char AInput) {
            switch (m_State) {
                case json_start:
                    if (AInput == '{') {
                        m_pJsonList->Add(m_Json->CreateObject());
                        m_State = string_start;
                        return -1;
                    } else if (AInput == '[') {
                        m_pJsonList->Add(m_Json->CreateArray());
                        m_State = value_start;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case string_start:
                    if (AInput == '"') {
                        CurrentObject().Add(CJSONMember());
                        m_State = string;
                        return -1;
                    } else if (AInput == '}') {
                        DeleteLastJson();
                        m_State = value_end;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case string:
                    if (IsLetter(AInput) || (AInput == '0') || IsDigit(AInput)) {
                        CurrentMember().String().Append(AInput);
                        m_State = string;
                        return -1;
                    } else if (AInput == '"') {
                        m_State = string_end;
                        return -1;
                    }
                    return false;
                case string_end:
                    if (AInput == ':') {
                        m_State = value_start;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_start:
                    if (AInput == '{') {
                        CreateValue(jvtObject);
                        m_State = string_start;
                        return -1;
                    } else if (AInput == '[') {
                        CreateValue(jvtArray);
                        m_State = value_start;
                        return -1;
                    } else if (AInput == '"') {
                        CreateValue(jvtString);
                        m_State = value_string_start;
                        return -1;
                    } else if ((AInput == '-') || (AInput == '0') || IsDigit(AInput)) {
                        CreateValue(jvtNumber);
                        CurrentValue().Data().Append(AInput);
                        m_State = value_digit;
                        return -1;
                    } else if (AInput == 't') {
                        CreateValue(jvtBoolean);
                        CurrentValue().Data().Append(AInput);
                        m_CharIndex++;
                        m_State = value_true;
                        return -1;
                    } else if (AInput == 'f') {
                        CreateValue(jvtBoolean);
                        CurrentValue().Data().Append(AInput);
                        m_CharIndex++;
                        m_State = value_false;
                        return -1;
                    } else if (AInput == 'n') {
                        CreateValue(jvtNull);
                        CurrentValue().Data().Append(AInput);
                        m_CharIndex++;
                        m_State = value_null;
                        return -1;
                    } else if ((AInput == '}') || (AInput == ']')) {
                        DeleteLastJson();
                        m_State = value_end;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_end:
                    if ((AInput == '}') || (AInput == ']')) {
                        DeleteLastJson();
                        if (m_pJsonList->Count() == 0)
                            return true;
                        return -1;
                    } else if (AInput == ',') {
                        if (CurrentJson().ValueType() == jvtObject)
                            m_State = string_start;
                        else
                            m_State = value_start;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_string_start:
                    if (AInput == '"') {
                        m_State = value_string_end;
                        return -1;
                    } if (IsCharacter(AInput)) {
                        CurrentValue().Data().Append(AInput);
                        m_State = value_string;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_string:
                    if (AInput == '"') {
                        m_State = value_string_end;
                        return -1;
                    } if (IsCharacter(AInput)) {
                        CurrentValue().Data().Append(AInput);
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_string_end:
                    if ((AInput == '}') || (AInput == ']')) {
                        DeleteLastJson();
                        m_State = value_end;
                        return -1;
                    } else if (AInput == ',') {
                        if (CurrentJson().ValueType() == jvtObject)
                            m_State = string_start;
                        else
                            m_State = value_start;
                        return -1;
                    } else if (AInput == '"') {
                        m_State = value_string;
                        return -1;
                    } if (IsCharacter(AInput)) {
                        CurrentValue().Data().Append(AInput);
                        m_State = value_string_end;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_digit:
                    if ((AInput == '}') || (AInput == ']')) {
                        DeleteLastJson();
                        m_State = value_end;
                        return -1;
                    } else if (AInput == ',') {
                        if (CurrentJson().ValueType() == jvtObject)
                            m_State = string_start;
                        else
                            m_State = value_start;
                        return -1;
                    } else if ((AInput == '.') || (AInput == '0') || IsDigit(AInput) || (AInput == 'e') || (AInput == 'E') || (AInput == '+') || (AInput == '-')) {
                        CurrentValue().Data().Append(AInput);
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_true:
                    if ((AInput == '}') || (AInput == ']')) {
                        DeleteLastJson();
                        m_State = value_end;
                        return -1;
                    } else if (AInput == _value_true[m_CharIndex]) {
                        m_CharIndex++;
                        if (m_CharIndex == 4) {
                            m_CharIndex = 0;
                        }
                        CurrentValue().Data().Append(AInput);
                        return -1;
                    } else if (AInput == ',') {
                        if (CurrentJson().ValueType() == jvtObject)
                            m_State = string_start;
                        else
                            m_State = value_start;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_false:
                    if ((AInput == '}') || (AInput == ']')) {
                        DeleteLastJson();
                        m_State = value_end;
                        return -1;
                    } else if (AInput == _value_false[m_CharIndex]) {
                        m_CharIndex++;
                        if (m_CharIndex == 5) {
                            m_CharIndex = 0;
                        }
                        CurrentValue().Data().Append(AInput);
                        return -1;
                    } else if (AInput == ',') {
                        if (CurrentJson().ValueType() == jvtObject)
                            m_State = string_start;
                        else
                            m_State = value_start;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                case value_null:
                    if ((AInput == '}') || (AInput == ']')) {
                        DeleteLastJson();
                        m_State = value_end;
                        return -1;
                    } else if (AInput == _value_null[m_CharIndex]) {
                        m_CharIndex++;
                        if (m_CharIndex == 3) {
                            m_CharIndex = 0;
                        }
                        CurrentValue().Data().Append(AInput);
                        return -1;
                    } else if (AInput == ',') {
                        if (CurrentJson().ValueType() == jvtObject)
                            m_State = string_start;
                        else
                            m_State = value_start;
                        return -1;
                    } else if (IsWS(AInput)) {
                        return -1;
                    }
                    return false;
                default:
                    return false;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSONParser::IsLetter(u_char c) {
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSONParser::IsCharacter(unsigned c) {
            return (c >= 0x20 && c <= 0x10FFFF) || (c == '"') || (c == '\\');
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSONParser::IsCtl(u_char c) {
            return (c >= 0 && c <= 31) || (c == 127);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSONParser::IsDigit(u_char c) {
            return c >= '1' && c <= '9';
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSONParser::IsWS(u_char c) {
            return (c == ' ') || (c == '\t') || (c == '\n')  || (c == '\r');
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CJSONParser::IsEscape(u_char c) {
            switch (c) {
                case '"':
                case '\\':
                case '/':
                case 'b':
                case 'n':
                case 'r':
                case 't':
                case 'u':
                    return true;
                default:
                    return false;
            }
        }
    }
}
}
