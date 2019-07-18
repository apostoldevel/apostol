/*++

Library name:

  libdelphi

Module Name:

  SQLLite.hpp

Notices:

  Delphi classes for C++

  SQLLite3

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_SQLLITE_HPP
#define DELPHI_SQLLITE_HPP
//----------------------------------------------------------------------------------------------------------------------

#include <sqlite3.h>

extern "C++" {

namespace Delphi {

    namespace SQLLite3 {

        //--------------------------------------------------------------------------------------------------------------

        //- CSQlLiteConnection -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CSQlLiteConnection;

        typedef std::function<void (CSQlLiteConnection *AConnection)> COnSQlLiteConnectionEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CSQlLiteConnection: public CObject {
        private:

            sqlite3 *m_Handle;

            CString m_DataBase;

            int m_ResultCode;

            COnSQlLiteConnectionEvent m_OnConnected;
            COnSQlLiteConnectionEvent m_OnDisconnected;

        protected:

            int GetResultCode();

            void DoConnected(CSQlLiteConnection *AConnection);
            void DoDisconnected(CSQlLiteConnection *AConnection);

        public:

            explicit CSQlLiteConnection(const CString &ADataBase);

            ~CSQlLiteConnection() override;

            const CString &DataBase() const { return m_DataBase; }

            bool Connect();

            void Disconnect();

            bool Connected() { return ResultCode() == SQLITE_OK; }

            int ResultCode() { return GetResultCode(); }

            LPCSTR GetErrorMessage();

            LPCSTR ResultCodeString();

            sqlite3 *Handle() { return m_Handle; }

            const COnSQlLiteConnectionEvent &OnConnected() { return m_OnConnected; }
            void OnConnected(COnSQlLiteConnectionEvent && Value) { m_OnConnected = Value; }

            const COnSQlLiteConnectionEvent &OnDisconnected() { return m_OnDisconnected; }
            void OnDisconnected(COnSQlLiteConnectionEvent && Value) { m_OnDisconnected = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //- CSQlLiteResult ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CSQlLiteQuery;
        class CSQlLiteResult;

        typedef std::function<void (CSQlLiteResult *AResult)> COnSQlLiteResultEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CSQlLiteResult: public CCollectionItem {
        private:

            sqlite3_stmt *m_Handle;

            CSQlLiteQuery *m_Query;

            int m_ResultCode;

            COnSQlLiteResultEvent m_OnResult;

        protected:

            virtual void DoResult();

        public:

            explicit CSQlLiteResult(CSQlLiteQuery *AQQuery, sqlite3_stmt *AHandle);

            ~CSQlLiteResult() override;

            void Clear();

            sqlite3_stmt *Handle() { return m_Handle; };

            CSQlLiteQuery *Query() { return m_Query; };

            int ResultCode() { return m_ResultCode; };

            const COnSQlLiteResultEvent &OnResult() { return m_OnResult; }
            void OnResult(COnSQlLiteResultEvent && Value) { m_OnResult = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //- CSQlLiteQuery ----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CSQlLiteQuery;

        typedef std::function<void (CSQlLiteQuery *AQuery)> COnSQlLiteQueryExecutedEvent;
        //--------------------------------------------------------------------------------------------------------------

        typedef TList<CVariant> CCSQlLiteParams;

        class CSQlLiteQuery: public CCollection {
            typedef CCollection inherited;

        private:

            CSQlLiteConnection *m_Connection;

            CString m_SQL;

            CCSQlLiteParams m_Params;

            COnSQlLiteQueryExecutedEvent m_OnExecuted;

        protected:

            CSQlLiteResult *GetResult(int Index);

            virtual void DoExecuted();

            void SetConnection(CSQlLiteConnection *Value);

        public:

            CSQlLiteQuery();

            explicit CSQlLiteQuery(CSQlLiteConnection *AConnection);

            ~CSQlLiteQuery() override;

            CSQlLiteConnection *Connection() { return m_Connection; };

            void Connection(CSQlLiteConnection *Value) { SetConnection(Value); };

            void Clear() override;

            int ResultCount() { return inherited::Count(); };

            void Execute();

            CString& SQL() { return m_SQL; }
            const CString& SQL() const { return m_SQL; }

            CCSQlLiteParams& Params() { return m_Params; }
            const CCSQlLiteParams& Params() const { return m_Params; }

            CSQlLiteResult *Results(int Index) { return GetResult(Index); };

            const COnSQlLiteQueryExecutedEvent &OnExecuted() { return m_OnExecuted; }
            void OnExecute(COnSQlLiteQueryExecutedEvent && Value) { m_OnExecuted = Value; }

        };

    }
}

using namespace Delphi::SQLLite3;
}
#endif //DELPHI_SQLLITE_HPP
