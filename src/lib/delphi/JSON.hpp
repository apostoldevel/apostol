/*++

Library name:

  libdelphi

Module Name:

  JSON.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_JSON_HPP
#define APOSTOL_JSON_HPP

#include "Classes.hpp"

extern "C++" {

namespace Delphi {

    namespace Json {

        class CJSON;

        class CJSONValue;

        class CJSONMember;

        class CJSONArray;

        class CJSONObject;

        class CJSONParser;
        //--------------------------------------------------------------------------------------------------------------

        typedef struct json_parser_result_s {
            int result;
            size_t pos;
        } CJSONParserResult;

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSON -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        enum CJSONValueType {
            jvtObject = 0, jvtArray, jvtString, jvtNumber, jvtBoolean, jvtNull
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CJSON : public CPersistent {
            friend CJSONValue;
            typedef LPCTSTR reference;

        private:

            CJSONValueType m_ValueType;

            CJSON *m_Value;

            CString m_JSON;

            int m_UpdateCount;

        protected:

            virtual int GetCount() const noexcept;

            void StringToJson(const CString &Value);

            virtual const CString& JsonToString(CString &String) const;

            void StrToJson(LPCTSTR ABuffer, size_t ASize);

            bool JsonToStr(LPTSTR ABuffer, size_t &ASize);

        public:

            CJSON();

            explicit CJSON(CJSONValueType ValueType);

            explicit CJSON(CPersistent *AOwner);

            explicit CJSON(CPersistent *AOwner, CJSONValueType ValueType);

            explicit CJSON(const CString& AString);

            ~CJSON() override;

            CJSONValueType ValueType() const { return m_ValueType; };

            void ValueType(CJSONValueType AValueType) { m_ValueType = AValueType; };

            int UpdateCount() { return m_UpdateCount; };

            virtual void SetUpdateState(bool Updating);

            void BeginUpdate();

            void EndUpdate();

            virtual bool IsNull() const { return m_ValueType == jvtNull; };

            bool IsObject() const { return m_ValueType == jvtObject; };

            bool IsArray() const { return m_ValueType == jvtArray; };

            bool IsString() const { return m_ValueType == jvtString; };

            bool IsNumber() const { return m_ValueType == jvtNumber; };

            bool IsBoolean() const { return m_ValueType == jvtBoolean; };

            virtual CJSONArray *CreateArray();

            virtual CJSONObject *CreateObject();

            virtual void Assign(const CJSON &Source);

            virtual void Concat(const CJSON &Source);

            virtual int Count() const noexcept { return GetCount(); };

            virtual void Clear();

            virtual CJSON *Value() { return m_Value; };

            virtual const CJSON *Value() const { return m_Value; };

            CJSONArray &Array() {
                if (m_Value == nullptr) {
                    CreateArray();
                }
                return *(CJSONArray *) m_Value;
            }

            const CJSONArray &Array() const {
                return *(CJSONArray *) m_Value;
            }

            CJSONObject &Object() {
                if (m_Value == nullptr) {
                    CreateObject();
                }
                return *(CJSONObject *) m_Value;
            }

            const CJSONObject &Object() const { return *(CJSONObject *) m_Value; }

            void LoadFromFile(LPCTSTR lpszFileName);

            void LoadFromStream(CStream *Stream);

            void SaveToFile(LPCTSTR lpszFileName);

            void SaveToStream(CStream *Stream);

            const CString& ToString(CString& Value) const { return JsonToString(Value); };

            const CString& ToString() { return JsonToString(m_JSON); };
            void ToJson(const CString &Value) { StringToJson(Value); };

            CJSON &operator=(const CJSON &Json) {
                if (this != &Json)
                    Assign(Json);
                return *this;
            };

            CJSON &operator<<(const CJSON &Json) {
                if (this != &Json)
                    Concat(Json);
                return *this;
            };

            virtual CJSON &operator=(const CString &String) {
                StringToJson(String);
                return *this;
            };

            virtual CJSON &operator<<(const CString &String) {
                StringToJson(String);
                return *this;
            };

            virtual CJSON &operator<<(reference Str) {
                CString J(Str);
                StringToJson(J);
                return *this;
            };

            friend CJSON &operator>>(const CString &LS, CJSON &RM) {
                RM.StringToJson(LS);
                return RM;
            };

            friend CJSON &operator>>(reference LS, CJSON &RM) {
                CString J(LS);
                RM.StringToJson(J);
                return RM;
            };

            friend tostream &operator<<(tostream &Out, CJSON &RM) {
                CString S;
                RM.JsonToString(S);
                Out << S.c_str();
                return Out;
            };

            friend tistream &operator>>(tistream &In, CJSON &RM) {
                TCHAR C;
                CString S;
                while (In.get(C) && C != '\n')
                    S.Append(C);
                RM.StringToJson(S);
                return In;
            };

            virtual CJSONMember &Members(int Index);

            virtual const CJSONMember &Members(int Index) const;

            virtual void Members(int Index, const CJSONMember &Value);

            virtual CJSONValue &operator[](int Index);

            virtual const CJSONValue &operator[](int Index) const;

            virtual CJSONValue &operator[](const CString &String);

            virtual const CJSONValue &operator[](const CString &String) const;

            virtual CJSONValue &operator[](reference String);

            virtual const CJSONValue &operator[](reference String) const;
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONElements ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CJSONElements : public CJSON {

            typedef CJSON inherited;
            typedef LPCTSTR reference;

        private:

            LPCTSTR m_LineBreak;

            TCHAR m_Delimiter;
            TCHAR m_QuoteChar;

            bool m_StrictDelimiter;

            int m_CurrentIndex;

            virtual CJSONValue &Get(int Index) abstract;

            virtual const CJSONValue &Get(int Index) const abstract;

            virtual void Put(int Index, const CJSONValue &Value);

            CJSONValue &GetValueFromIndex(int Index);

            const CJSONValue &GetValueFromIndex(int Index) const;

            void SetValueFromIndex(int Index, const CJSONValue &Value);

            void SetCurrentIndex(int Index);

        protected:

            const CString& JsonToString(CString &String) const override;

            void Error(const CString &Msg, int Data);

            virtual int GetCapacity() const noexcept;

            virtual void SetCapacity(int NewCapacity);

        public:

            CJSONElements(CPersistent *AOwner, CJSONValueType ValueType);

            ~CJSONElements() override = default;

            virtual int Add(const CJSONValue &Value);

            virtual int Add(const CString &Value);

            virtual int Add(const reference Value);

            virtual void AddElements(const CJSONElements &Source);

            virtual void SetElements(const CJSONElements &Source);

            virtual void Assign(const CJSONElements &Source);

            virtual void Concat(const CJSONElements &Source);

            virtual void Delete(int Index) abstract;

            bool Equals(const CJSONElements &Elements);

            virtual void Exchange(int Index1, int Index2);

            virtual int IndexOf(const CJSONValue &Value) const;

            virtual void Insert(int Index, const CJSONValue &Value) abstract;

            virtual void Move(int CurIndex, int NewIndex);

            int CurrentIndex() const { return m_CurrentIndex; };

            void CurrentIndex(int Value) { SetCurrentIndex(Value); };

            int Capacity() const noexcept { return GetCapacity(); };

            void Capacity(int NewCapacity) { SetCapacity(NewCapacity); };

            CJSONValue &ValueFromIndex(int Index) { return GetValueFromIndex(Index); };

            const CJSONValue &ValueFromIndex(int Index) const { return GetValueFromIndex(Index); };

            void ValueFromIndex(int Index, const CJSONValue &Value) { SetValueFromIndex(Index, Value); };

            CJSONValue &First() { return Get(0); };

            const CJSONValue &First() const { return Get(0); };

            CJSONValue &front() { return Get(0); };

            const CJSONValue &front() const { return Get(0); };

            CJSONValue &Last() { return Get(GetCount() - 1); };

            const CJSONValue &Last() const { return Get(GetCount() - 1); };

            CJSONValue &back() { return Get(GetCount() - 1); };

            const CJSONValue &back() const { return Get(GetCount() - 1); };

            virtual CJSONValue &Values(int Index) { return Get(Index); };

            virtual const CJSONValue &Values(int Index) const { return Get(Index); };

            virtual void Values(int Index, const CJSONValue &Value) { return Put(Index, Value); };

            CJSONElements &operator=(const CJSONElements &Value) {
                if (&Value != this) {
                    Assign(Value);
                }
                return *this;
            }

            virtual CJSONElements &operator<<(const CJSONElements &Value) {
                if (this != &Value)
                    Concat(Value);
                return *this;
            };

            virtual CJSONElements &operator<<(const CJSONValue &Value) {
                Add(Value);
                return *this;
            };

            CJSONElements &operator<<(const CString &String) override {
                StringToJson(String);
                return *this;
            };

            CJSONElements &operator<<(reference Str) override {
                StrToJson(Str, strlen(Str));
                return *this;
            };

            friend CJSONElements &operator>>(const CString &LS, CJSONElements &RM) {
                RM.StringToJson(LS);
                return RM;
            };

            friend CJSONElements &operator>>(reference LS, CJSONElements &RM) {
                RM.StrToJson(LS, strlen(LS));
                return RM;
            };

            friend tostream &operator<<(tostream &Out, CJSONElements &RM) {
                CString J;
                RM.JsonToString(J);
                return Out << J.c_str();
            };

            friend tistream &operator>>(tistream &In, CJSONElements &RM) {
                TCHAR C;
                CString S;
                while (In.get(C) && C != '\n')
                    S.Append(C);
                RM.StringToJson(S);
                return In;
            };

            CJSONValue &operator[](int Index) override { return Get(Index); }

            const CJSONValue &operator[](int Index) const override { return Get(Index); }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONMembers ----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CJSONMembers : public CJSON {

            typedef CJSON inherited;
            typedef LPCTSTR reference;

        private:

            LPCTSTR m_LineBreak;

            TCHAR m_Delimiter;
            TCHAR m_QuoteChar;
            TCHAR m_NameValueSeparator;

            bool m_StrictDelimiter;

            int m_CurrentIndex;

            virtual const CString &GetString(int Index) const abstract;

            virtual CJSONValue &GetValue(const CString &String) abstract;

            virtual const CJSONValue &GetValue(const CString &String) const abstract;

            virtual CJSONValue &GetValue(reference String) abstract;

            virtual const CJSONValue &GetValue(reference String) const abstract;

            void SetValue(const CString &String, const CJSONValue &Value);

            void SetValue(reference String, const CJSONValue &Value);

            CJSONValue &GetValueFromIndex(int Index);

            const CJSONValue &GetValueFromIndex(int Index) const;

            void SetValueFromIndex(int Index, const CJSONValue &Value);

            void SetCurrentIndex(int Index);

        protected:

            const CString& JsonToString(CString &String) const override;

            static void Error(const CString &Msg, int Data);

            virtual CJSONMember &Get(int Index) abstract;

            virtual const CJSONMember &Get(int Index) const abstract;

            virtual void Put(int Index, const CJSONMember &Value);

            virtual int GetCapacity() const noexcept;

            virtual void PutPair(int Index, const CString &String, const CJSONValue &Value);

            virtual void PutPair(int Index, reference String, const CJSONValue &Value);

            virtual void SetCapacity(int NewCapacity);

        public:

            CJSONMembers(CPersistent *AOwner, CJSONValueType ValueType);

            ~CJSONMembers() override = default;

            virtual int Add(const CJSONMember &Value);

            virtual int AddPair(const CString &String, const CJSONValue &Value);

            virtual int AddPair(reference String, const CJSONValue &Value);

            virtual int AddPair(const CString &String, const CString &Value);

            virtual int AddPair(reference String, const CString &Value);

            virtual int AddPair(reference String, int Value);

            virtual int AddPair(reference String, float Value);

            virtual int AddPair(reference String, double Value);

            virtual void AddMembers(const CJSONMembers &Value);

            virtual void Assign(const CJSONMembers &Source);

            virtual void Concat(const CJSONMembers &Source);

            virtual void SetMembers(const CJSONMembers &Source);

            virtual void Delete(int Index) abstract;

            bool Equals(const CJSONMembers &Members);

            virtual void Exchange(int Index1, int Index2);

            virtual int IndexOf(const CJSONMember &Value) const;

            virtual int IndexOfString(const CString &Value) const;

            virtual int IndexOfString(reference Value) const;

            virtual void Insert(int Index, const CJSONMember &Value) abstract;

            virtual void InsertPair(int Index, const CString &String, const CJSONValue &Value) abstract;

            virtual void InsertPair(int Index, reference String, const CJSONValue &Value) abstract;

            virtual void InsertPair(int Index, const CString &String, const CString &Value) abstract;

            virtual void InsertPair(int Index, reference String, const CString &Value) abstract;

            virtual void InsertPair(int Index, reference String, int Value) abstract;

            virtual void InsertPair(int Index, reference String, float Value) abstract;

            virtual void InsertPair(int Index, reference String, double Value) abstract;

            virtual void Move(int CurIndex, int NewIndex);

            int CurrentIndex() const { return m_CurrentIndex; };

            void CurrentIndex(int Value) { SetCurrentIndex(Value); };

            int Capacity() const noexcept { return GetCapacity(); };

            void Capacity(int NewCapacity) { SetCapacity(NewCapacity); };

            const CString &Strings(int Index) const { return GetString(Index); };

            CJSONValue &Values(const CString &String) { return GetValue(String); };

            const CJSONValue &Values(const CString &String) const { return GetValue(String); };

            void Values(const CString &String, const CJSONValue &Value) { SetValue(String, Value); };

            CJSONValue &Values(reference String) { return GetValue(String); };

            const CJSONValue &Values(reference String) const { return GetValue(String); };

            void Values(reference String, const CJSONValue &Value) { SetValue(String, Value); };

            CJSONValue &ValueFromIndex(int Index) { return GetValueFromIndex(Index); };

            const CJSONValue &ValueFromIndex(int Index) const { return GetValueFromIndex(Index); };

            void ValueFromIndex(int Index, const CJSONValue &Value) { SetValueFromIndex(Index, Value); };

            CJSONMember &First() { return Get(0); };

            const CJSONMember &First() const { return Get(0); };

            CJSONMember &front() { return Get(0); };

            const CJSONMember &front() const { return Get(0); };

            CJSONMember &Last() { return Get(GetCount() - 1); };

            const CJSONMember &Last() const { return Get(GetCount() - 1); };

            CJSONMember &back() { return Get(GetCount() - 1); };

            const CJSONMember &back() const { return Get(GetCount() - 1); };

            CJSONMember &Members(int Index) override { return Get(Index); };

            const CJSONMember &Members(int Index) const override { return Get(Index); };

            void Members(int Index, const CJSONMember &Value) override { return Put(Index, Value); };

            CJSONMembers &operator=(const CJSONMembers &Value) {
                if (&Value != this) {
                    Assign(Value);
                }
                return *this;
            }

            CJSONMembers &operator<<(const CJSONMembers &Value) {
                if (this != &Value)
                    Concat(Value);
                return *this;
            };

            virtual CJSONMembers &operator<<(const CJSONMember &Value) {
                Add(Value);
                return *this;
            };

            CJSONMembers &operator<<(const CString &String) override {
                StringToJson(String);
                return *this;
            };

            CJSONMembers &operator<<(reference Str) override {
                StrToJson(Str, strlen(Str));
                return *this;
            };

            friend CJSONMembers &operator>>(const CString &LS, CJSONMembers &RM) {
                RM.StringToJson(LS);
                return RM;
            };

            friend CJSONMembers &operator>>(reference LS, CJSONMembers &RM) {
                RM.StrToJson(LS, strlen(LS));
                return RM;
            };

            friend tostream &operator<<(tostream &Out, CJSONMembers &RM) {
                CString J;
                RM.JsonToString(J);
                return Out << J.c_str();
            };

            friend tistream &operator>>(tistream &In, CJSONMembers &RM) {
                TCHAR C;
                CString S;
                while (In.get(C) && C != '\n')
                    S.Append(C);
                RM.StringToJson(S);
                return In;
            };

            CJSONValue &operator[](const CString &Value) override { return GetValue(Value); }

            const CJSONValue &operator[](const CString &Value) const override { return GetValue(Value); }

            CJSONValue &operator[](reference Value) override { return GetValue(Value); }

            const CJSONValue &operator[](reference Value) const override { return GetValue(Value); }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CMembersEnumerator ----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CMembersEnumerator : public CObject {
            typedef CObject inherited;

        private:

            int m_Index;

            CJSONMembers *m_Members;

        public:

            explicit CMembersEnumerator(CJSONMembers *AMembers) : CObject() {
                m_Index = -1;
                m_Members = AMembers;
            }

            inline ~CMembersEnumerator() override = default;

            CJSONMember &GetCurrent() {
                return m_Members->Members(m_Index);
            }

            const CJSONMember &GetCurrent() const {
                return m_Members->Members(m_Index);
            }

            bool Next() {
                if (m_Index < m_Members->Count() - 1) {
                    m_Index++;
                    return true;
                }
                return false;
            }

            bool Prior() {
                if (m_Index > 0) {
                    m_Index--;
                    return true;
                }
                return false;
            }

            CJSONMember &Current() { return GetCurrent(); };

            const CJSONMember &Current() const { return GetCurrent(); };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONValue ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CJSONValue : public CJSON {
            typedef CJSON inherited;
            typedef LPCTSTR reference;

        private:

            CString m_Data;

        protected:

            const CString& JsonToString(CString &String) const override;

            CJSONValue &GetValue(const CString &String);

            const CJSONValue &GetValue(const CString &String) const;

            CJSONValue &GetValue(reference String);

            const CJSONValue &GetValue(reference String) const;

            void SetValue(const CString &String, const CJSONValue &Value);

            void SetValue(reference String, const CJSONValue &Value);

            CJSONValue &Get(int Index);

            const CJSONValue &Get(int Index) const;

            void Put(int Index, const CJSONValue &Data);;

        public:

            CJSONValue() : CJSON(this, jvtNull) {
                m_Value = nullptr;
            };

            explicit CJSONValue(CJSONValueType AType) : CJSON(this, AType) {
                m_Value = nullptr;

                if (AType == jvtObject)
                    CreateObject();

                if (AType == jvtArray)
                    CreateArray();
            };

            explicit CJSONValue(const CJSON& Value) : CJSON(this, jvtObject) {
                m_Value = nullptr;
                CJSON::Assign(Value);
            };

            explicit CJSONValue(const CString& Value) : CJSON(this, jvtString) {
                m_Value = nullptr;
                m_Data = Value;
            };

            explicit CJSONValue(reference Value) : CJSON(this, jvtString) {
                m_Value = nullptr;
                m_Data = Value;
            };

            ~CJSONValue() override = default;

            void Assign(const CJSONValue &Value);

            bool IsEmpty() const { return m_Data.IsEmpty(); };

            CString &Data() { return m_Data; }

            const CString &Data() const { return m_Data; }

            CString &AsSiring() { return m_Data; }

            const CString &AsSiring() const { return m_Data; }

            long AsInteger() const { return StrToInt(m_Data.c_str()); };

            float AsFloat() const { return StrToFloat(m_Data.c_str()); };

            double AsDouble() const { return StrToDouble(m_Data.c_str()); };

            long double AsDecimal() const { return StrToDecimal(m_Data.c_str()); };

            bool AsBoolean() const {
                LPCTSTR LBoolStr[] = ARRAY_BOOLEAN_STRINGS;

                for (unsigned int i = 0; i < chARRAY(LBoolStr); ++i) {
                    if (SameText(LBoolStr[i], m_Data.c_str()))
                        return Odd(i);
                }

                throw EConvertError(_T("Invalid conversion string \"%s\" to boolean value."), m_Data.c_str());
            };

            int Compare(const CJSONValue& Value) const;

            CJSONArray &AsArray() { return *(CJSONArray *) m_Value; }

            const CJSONArray &AsArray() const { return *(CJSONArray *) m_Value; }

            CJSONObject &AsObject() { return *(CJSONObject *) m_Value; }

            const CJSONObject &AsObject() const { return *(CJSONObject *) m_Value; }

            virtual bool operator!=(const CJSONValue &AValue) const {
                if (this != &AValue) {
                    return Data() != AValue.Data();
                }
                return false;
            }

            virtual bool operator==(const CJSONValue &AValue) const {
                if (this != &AValue) {
                    return Data() == AValue.Data();
                }
                return false;
            }

            CJSONValue &operator<<(const CString &Value) override {
                m_Data << Value;
                return *this;
            };

            CJSONValue &operator<<(reference Value) override {
                m_Data << Value;
                return *this;
            };

            friend tostream &operator<<(tostream &Out, const CJSONValue &RM) {
                return Out << RM.Data().c_str();
            };

            CJSONValue &operator=(const CJSONValue &AValue) {
                if (this != &AValue) {
                    Assign(AValue);
                }
                return *this;
            }

            CJSONValue &operator[](int Index) override { return Get(Index); }

            const CJSONValue &operator[](int Index) const override { return Get(Index); }

            CJSONValue &operator[](const CString &String) override { return GetValue(String); }

            const CJSONValue &operator[](const CString &String) const override { return GetValue(String); }

            CJSONValue &operator[](reference String) override { return GetValue(String); }

            const CJSONValue &operator[](reference String) const override { return GetValue(String); }

            static const int incorrect_type = static_cast<int>(-1);
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONMember -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CJSONMember : public CPersistent {
            typedef LPCTSTR reference;

        private:

            CString m_String;

            CJSONValue m_Value;

        public:

            CJSONMember() : CPersistent(this) {

            };

            CJSONMember(const CJSONMember &AValue) : CPersistent(this) {
                Assign(AValue);
            }

            explicit CJSONMember(CJSONValueType ValueType) : CPersistent(this) {
                m_Value.ValueType(ValueType);
            }

            explicit CJSONMember(const CJSONValue &AValue) : CPersistent(this) {
                m_Value = AValue;
            }

            explicit CJSONMember(const CString &AString, const CJSONValue &AValue) : CPersistent(this) {
                m_String = AString;
                m_Value = AValue;
            }

            explicit CJSONMember(LPCTSTR AString, const CJSONValue &AValue) : CPersistent(this) {
                m_String = AString;
                m_Value = AValue;
            }

            explicit CJSONMember(const CString &AString, const CString &AValue) : CPersistent(this) {
                m_String = AString;
                m_Value.ValueType(jvtString);
                m_Value.Data() = AValue;
            }

            explicit CJSONMember(LPCTSTR AString, const CString &AValue) : CPersistent(this) {
                m_String = AString;
                m_Value.ValueType(jvtString);
                m_Value.Data() = AValue;
            }

            explicit CJSONMember(LPCTSTR AString, int AValue) : CPersistent(this) {
                m_String = AString;
                m_Value.ValueType(jvtNumber);
                m_Value.Data() = AValue;
            }

            explicit CJSONMember(LPCTSTR AString, float AValue) : CPersistent(this) {
                m_String = AString;
                m_Value.ValueType(jvtNumber);
                m_Value.Data() = AValue;
            }

            explicit CJSONMember(LPCTSTR AString, double AValue) : CPersistent(this) {
                m_String = AString;
                m_Value.ValueType(jvtNumber);
                m_Value.Data() = AValue;
            }

            CString &String() { return m_String; };

            const CString &String() const { return m_String; };

            CJSONValue &Value() { return m_Value; };

            const CJSONValue &Value() const { return m_Value; };

            void Assign(const CJSONMember &AValue) {
                m_String = AValue.m_String;
                m_Value = AValue.m_Value;
            };

            virtual bool operator!=(const CJSONMember &AValue) const {
                if (this != &AValue) {
                    return m_Value != AValue.m_Value;
                }
                return false;
            }

            virtual bool operator==(const CJSONMember &AValue) const {
                if (this != &AValue) {
                    return m_Value == AValue.m_Value;
                }
                return false;
            }

            CJSONMember &operator=(const CJSONMember &AValue) {
                if (this != &AValue) {
                    Assign(AValue);
                }
                return *this;
            }

            virtual CJSONMember &operator<<(const CJSONMember &Element) {
                if (this != &Element)
                    Assign(Element);
                return *this;
            };

            CJSONMember &operator<<(const CString &Value) {
                m_Value << Value;
                return *this;
            };

            CJSONMember &operator<<(reference Str) {
                m_Value << Str;
                return *this;
            };

            friend CJSONMember &operator>>(const CJSONMember &LE, CJSONMember &RE) {
                RE = LE;
                return RE;
            };

            friend tostream &operator<<(tostream &Out, const CJSONMember &RE) {
                return Out << RE.Value().Data().c_str();
            };

            friend tistream &operator>>(tistream &In, CJSONMember &RE) {
                TCHAR C;
                CString S;
                while (In.get(C) && C != '\n')
                    S.Append(C);
                RE.Value().Data() = S;
                return In;
            };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONArray ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef TList<CJSONValue> CJSONValueList;

        typedef int (*CJSONListSortCompare)(const CJSONValue& Value1, const CJSONValue& Value2);
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CJSONArray : public CJSONElements {
            typedef CJSONElements inherited;
            typedef LPCTSTR reference;

        private:

            CJSONValueList m_pList;

            CJSONValue &Get(int Index) override;

            const CJSONValue &Get(int Index) const override;

            void Put(int Index, const CJSONValue &Value) override;

        protected:

            int GetCapacity() const noexcept override;

            void SetCapacity(int NewCapacity) override;

            int GetCount() const noexcept override;

        public:

            CJSONArray();

            explicit CJSONArray(CPersistent *AOwner);

            ~CJSONArray() override;

            inline static class CJSONArray *Create(CPersistent *AOwner) { return new CJSONArray(AOwner); };

            void Assign(const CJSONElements &Source) override;

            void Concat(const CJSONElements &Source) override;

            void Clear() override;

            void Delete(int Index) override;

            int Count() const noexcept override { return GetCount(); };

            int Add(const CJSONValue &Value) override;

            int Add(const CString &Value) override;

            int Add(reference Value) override;

            void Insert(int Index, const CJSONValue &Value) override;

            void Sort(CJSONListSortCompare Compare);

            CJSONArray &operator=(const CJSONArray &Value) {
                if (&Value != this) {
                    Assign(Value);
                }
                return *this;
            }

            CJSONArray &operator<<(const CJSONArray &Value) {
                if (this != &Value)
                    Concat(Value);
                return *this;
            };

            CJSONArray &operator<<(const CJSONValue &Value) override {
                Add(Value);
                return *this;
            };

            CJSONArray &operator<<(const CString &String) override {
                StringToJson(String);
                return *this;
            };

            CJSONArray &operator<<(reference Str) override {
                StrToJson(Str, strlen(Str));
                return *this;
            };

            friend CJSONArray &operator>>(const CString &LS, CJSONArray &RM) {
                RM.StringToJson(LS);
                return RM;
            };

            friend CJSONArray &operator>>(reference LS, CJSONArray &RM) {
                RM.StrToJson(LS, strlen(LS));
                return RM;
            };

            friend tostream &operator<<(tostream &Out, CJSONArray &RM) {
                CString J;
                RM.JsonToString(J);
                return Out << J.c_str();
            };

            friend tistream &operator>>(tistream &In, CJSONArray &RM) {
                TCHAR C;
                CString S;
                while (In.get(C) && C != '\n')
                    S.Append(C);
                RM.StringToJson(S);
                return In;
            };

            CJSONValue &operator[](int Index) override { return Get(Index); }

            const CJSONValue &operator[](int Index) const override { return Get(Index); }

        }; // CJSONArray

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONObject -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CJSONObject : public CJSONMembers {
            typedef CJSONMembers inherited;
            typedef LPCTSTR reference;

        private:

            TList<CJSONMember> m_pList;

            CJSONValue m_NullValue;

            const CString &GetString(int Index) const override;

            CJSONValue &GetValue(const CString &String) override;

            const CJSONValue &GetValue(const CString &String) const override;

            CJSONValue &GetValue(reference String) override;

            const CJSONValue &GetValue(reference String) const override;

        protected:

            CJSONMember &Get(int Index) override;

            const CJSONMember &Get(int Index) const override;

            void Put(int Index, const CJSONMember &Value) override;

            int GetCapacity() const noexcept override;

            void SetCapacity(int NewCapacity) override;

            int GetCount() const noexcept override;

            void InsertPair(int Index, const CString &String, const CJSONValue &Value) override;

            void InsertPair(int Index, reference String, const CJSONValue &Value) override;

            void InsertPair(int Index, const CString &String, const CString &Value) override;

            void InsertPair(int Index, reference String, const CString &Value) override;

            void InsertPair(int Index, reference String, int Value) override;

            void InsertPair(int Index, reference String, float Value) override;

            void InsertPair(int Index, reference String, double Value) override;

        public:

            CJSONObject();

            explicit CJSONObject(CPersistent *AOwner);

            inline static class CJSONObject *Create(CPersistent *AOwner) { return new CJSONObject(AOwner); };

            ~CJSONObject() override;

            void Assign(const CJSONMembers &Source) override;

            void Clear() override;

            virtual void Update(int Index);

            void Delete(int Index) override;

            int Count() const noexcept override { return GetCount(); };

            int Add(const CJSONMember &Value) override;

            int AddPair(const CString &String, const CJSONValue &Value) override;

            int AddPair(reference String, const CJSONValue &Value) override;

            int AddPair(const CString &String, const CString &Value) override;

            int AddPair(reference String, const CString &Value) override;

            int AddPair(reference String, int Value) override;

            int AddPair(reference String, float Value) override;

            int AddPair(reference String, double Value) override;

            void Insert(int Index, const CJSONMember &Value) override;

            CJSONMember &Members(int Index) override { return Get(Index); };

            const CJSONMember &Members(int Index) const override { return Get(Index); };

            void Members(int Index, const CJSONMember &Value) override { return Put(Index, Value); };

            CJSONObject &operator=(const CJSONObject &Value) {
                if (&Value != this) {
                    Assign(Value);
                }
                return *this;
            }

            CJSONObject &operator<<(const CJSONObject &Value) {
                if (this != &Value)
                    Assign(Value);
                return *this;
            };

            CJSONObject &operator<<(const CJSONMember &Value) override {
                Add(Value);
                return *this;
            };

            CJSONObject &operator<<(const CString &String) override {
                StringToJson(String);
                return *this;
            };

            CJSONObject &operator<<(reference Str) override {
                CString J(Str);
                StringToJson(J);
                return *this;
            };

            friend CJSONObject &operator>>(const CString &LS, CJSONObject &RM) {
                RM.StringToJson(LS);
                return RM;
            };

            friend CJSONObject &operator>>(reference LS, CJSONObject &RM) {
                CString J(LS);
                RM.StringToJson(J);
                return RM;
            };

            friend tostream &operator<<(tostream &Out, CJSONObject &RM) {
                CString J;
                RM.JsonToString(J);
                return Out << J.c_str();
            };

            friend tistream &operator>>(tistream &In, CJSONObject &RM) {
                TCHAR C;
                CString S;
                while (In.get(C) && C != '\n')
                    S.Append(C);
                RM.StringToJson(S);
                return In;
            };

            CJSONValue &operator[](int Index) override { return ValueFromIndex(Index); }

            const CJSONValue &operator[](int Index) const override { return ValueFromIndex(Index); }

            CJSONValue &operator[](const CString &Value) override { return GetValue(Value); }

            const CJSONValue &operator[](const CString &Value) const override { return GetValue(Value); }

            CJSONValue &operator[](reference Value) override { return GetValue(Value); }

            const CJSONValue &operator[](reference Value) const override { return GetValue(Value); }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONParser -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef enum parcer_state {
            json_start,
            string_start,
            string,
            string_end,
            value_start,
            value,
            value_end,
            value_string_start,
            value_string,
            value_string_end,
            value_digit,
            value_true,
            value_false,
            value_null
        } CParcerState;

        //--------------------------------------------------------------------------------------------------------------

        //-- CJSONParser -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CJSONParser : public CObject {
        private:

            CJSON *m_Json;

            CList *m_pJsonList;

            CParcerState m_State;

            int m_Result;

            int m_CharIndex;

            int Consume(u_char AInput);

            static bool IsWS(u_char c);

            static bool IsEscape(u_char c);

            static bool IsLetter(u_char c);

            static bool IsCharacter(unsigned c);

            static bool IsCtl(u_char c);

            static bool IsDigit(u_char c);

        protected:

            CJSON &CurrentJson();

            CJSONMember &CurrentMember();

            CJSONObject &CurrentObject();

            CJSONArray &CurrentArray();

            CJSONValue &CurrentValue();

            void CreateValue(CJSONValueType ValueType);

            void DeleteLastJson();

        public:

            explicit CJSONParser(CJSON *Json);

            ~CJSONParser() override;

            void Reset();

            CJSONParserResult Parse(LPTSTR ABegin, LPCTSTR AEnd);

            CParcerState State() { return m_State; };

            int Result() { return m_Result; };

        };
    }
}

using namespace Delphi::Json;
}

#endif //APOSTOL_JSON_HPP
