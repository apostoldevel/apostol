/*++

Library name:

  libdelphi

Module Name:

  SQLLite.cpp

Notices:

  Delphi classes for C++

  SQLLite3

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "delphi.hpp"
#include "SQLite.hpp"

extern "C++" {

namespace Delphi {

    namespace SQLLite3 {

        CSQlLiteConnection::CSQlLiteConnection(const CString &ADataBase): CObject() {
            m_Handle = nullptr;
            m_DataBase = ADataBase;
            m_ResultCode = SQLITE_ERROR;
            m_OnConnected = nullptr;
            m_OnDisconnected = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CSQlLiteConnection::~CSQlLiteConnection() {
            Disconnect();
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CSQlLiteConnection::Connect() {
            if ((m_ResultCode = sqlite3_open_v2(m_DataBase.c_str(), &m_Handle, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr)) == SQLITE_OK)
                DoConnected(this);
            return m_ResultCode == SQLITE_OK;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteConnection::Disconnect() {
            if (m_Handle != nullptr) {
                DoDisconnected(this);
                sqlite3_close_v2(m_Handle);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        int CSQlLiteConnection::GetResultCode() {
            m_ResultCode = sqlite3_errcode(m_Handle);
            return m_ResultCode;
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CSQlLiteConnection::GetErrorMessage() {
            sqlite3_mutex *mutex = sqlite3_db_mutex(m_Handle);
            if (mutex != nullptr)
                sqlite3_mutex_enter(mutex);
            const char *Result = sqlite3_errmsg(m_Handle);
            if (mutex != nullptr)
                sqlite3_mutex_leave(mutex);
            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CSQlLiteConnection::ResultCodeString() {
            switch (m_ResultCode) {
                case SQLITE_OK:
                    return "Connection success.";
                case SQLITE_ERROR:
                    return "Connection failed.";
                case SQLITE_INTERNAL:
                    return "Internal malfunction.";
                case SQLITE_PERM:
                    return "Created database could not be provided.";
                case SQLITE_BUSY:
                    return "Database file could not be written.";
                case SQLITE_LOCKED:
                    return "Write operation could not continue.";
                case SQLITE_NOMEM:
                    return "Unable to allocate all the memory.";
                case SQLITE_READONLY:
                    return "Current database connection does not have write permission.";
                case SQLITE_INTERRUPT:
                    return "Operation was interrupted by the sqlite3_interrupt() interface.";
                case SQLITE_IOERR:
                    return "Operating system reported an I/O error.";
                case SQLITE_CORRUPT:
                    return "Database file has been corrupted.";
            }

            return GetErrorMessage();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteConnection::DoConnected(CSQlLiteConnection *AConnection) {
            if (m_OnConnected != nullptr) {
                m_OnConnected(AConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteConnection::DoDisconnected(CSQlLiteConnection *AConnection) {
            if (m_OnDisconnected != nullptr) {
                m_OnDisconnected(AConnection);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSQlLiteResult --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSQlLiteResult::CSQlLiteResult(CSQlLiteQuery *AQQuery, sqlite3_stmt *AHandle): CCollectionItem(AQQuery)  {
            m_Query = AQQuery;
            m_Handle = AHandle;
            m_ResultCode = SQLITE_ERROR;
            m_OnResult = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CSQlLiteResult::~CSQlLiteResult() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteResult::Clear() {
            if (m_Handle != nullptr)
                sqlite3_finalize(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteResult::DoResult() {
            if (m_OnResult != nullptr) {
                m_OnResult(this);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSQlLiteQuery ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSQlLiteQuery::CSQlLiteQuery(): CCollection(this) {
            m_Connection = nullptr;
            m_OnExecuted = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CSQlLiteQuery::CSQlLiteQuery(CSQlLiteConnection *AConnection): CSQlLiteQuery() {
            m_Connection = AConnection;
        }
        //--------------------------------------------------------------------------------------------------------------

        CSQlLiteQuery::~CSQlLiteQuery() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        CSQlLiteResult *CSQlLiteQuery::GetResult(int Index) {
            return (CSQlLiteResult *) inherited::GetItem(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteQuery::SetConnection(CSQlLiteConnection *Value) {
            if (m_Connection != Value) {
                m_Connection = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteQuery::Clear() {
            CCollection::Clear();
            m_SQL.Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteQuery::Execute() {

            if (m_Connection == nullptr)
                throw Delphi::Exception::EDBError(_T("Connection has not be empty"));

            if (!m_Connection->Connected()) {
                if (!m_Connection->Connect())
                    throw Delphi::Exception::EDBError(m_Connection->GetErrorMessage());
            }

            sqlite3_stmt *stmt = nullptr;
            int ResultCode = sqlite3_prepare_v2(m_Connection->Handle(), m_SQL.c_str(), m_SQL.Length() + 1, &stmt, nullptr);

            if (ResultCode != SQLITE_OK) {
                throw Delphi::Exception::EDBError(m_Connection->GetErrorMessage());
            }

            for (int i = 0; i < m_Params.Count(); i++) {

                const CVariant &Value = m_Params[i];

                switch (Value.VType) {
                    case vtEmpty:
                        ResultCode = sqlite3_bind_null(stmt, i + 1);
                        break;
                    case vtInteger:
                        ResultCode = sqlite3_bind_int(stmt, i + 1, Value.varInteger);
                        break;
                    case vtBoolean:
                        ResultCode = sqlite3_bind_int(stmt, i + 1, Value.varBoolean ? 1 : 0);
                        break;
                    case vtChar:
                        ResultCode = sqlite3_bind_text(stmt, i + 1, &Value.varChar, 1, SQLITE_TRANSIENT);
                        break;
                    case vtDouble:
                        ResultCode = sqlite3_bind_double(stmt, i + 1, Value.varDouble);
                        break;
                    case vtString:
                        ResultCode = sqlite3_bind_text(stmt, i + 1, Value.varString->c_str(), Value.varString->Size(), SQLITE_TRANSIENT);
                        break;
                    case vtPointer:
                        ResultCode = sqlite3_bind_null(stmt, i + 1);
                        break;
                    case vtPChar:
                        ResultCode = sqlite3_bind_text(stmt, i + 1, Value.varPChar, -1, SQLITE_TRANSIENT);
                        break;
                    case vtObject:
                        ResultCode = sqlite3_bind_null(stmt, i + 1);
                        break;
                    case vtUnsigned:
                        ResultCode = sqlite3_bind_int(stmt, i + 1, Value.varUnsigned);
                        break;
                    case vtWideChar:
                        ResultCode = sqlite3_bind_null(stmt, i + 1);
                        break;
                    case vtPWideChar:
                        ResultCode = sqlite3_bind_null(stmt, i + 1);
                        break;
                    case vtAnsiString:
                        ResultCode = sqlite3_bind_text(stmt, i + 1, Value.varAnsiString, -1, SQLITE_TRANSIENT);
                        break;
                    case vtFloat:
                        ResultCode = sqlite3_bind_double(stmt, i + 1, Value.varFloat);
                        break;
                    case vtVariant:
                        ResultCode = sqlite3_bind_null(stmt, i + 1);
                        break;
                    case vtUInt64:
                        ResultCode = sqlite3_bind_int64(stmt, i + 1, Value.varUInt64);
                        break;
                    case vtWideString:
                        ResultCode = sqlite3_bind_null(stmt, i + 1);
                        break;
                    case vtInt64:
                        ResultCode = sqlite3_bind_int64(stmt, i + 1, Value.varInt64);
                        break;
                    case vtUnicodeString:
                        ResultCode = sqlite3_bind_text(stmt, i + 1, Value.varUnicodeString->c_str(), Value.varUnicodeString->Size(), SQLITE_TRANSIENT);
                        break;
                }

                if (ResultCode != SQLITE_OK) {
                    throw Delphi::Exception::EDBError(m_Connection->GetErrorMessage());
                }
            }

            Added(new CSQlLiteResult(this, stmt));

            DoExecuted();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSQlLiteQuery::DoExecuted() {
            if (m_OnExecuted != nullptr) {
                try {
                    m_OnExecuted(this);
                } catch (...) {
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}
