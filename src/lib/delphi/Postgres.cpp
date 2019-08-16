/*++

Library name:

  libdelphi

Module Name:

  Postgres.cpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "delphi.hpp"
#include "Postgres.hpp"
//----------------------------------------------------------------------------------------------------------------------

void OnNoticeReceiver(void *AData, const PGresult *AResult) {
    if (Assigned(AData)) {
        auto LConnection = (CPQConnection *) AData;
        LConnection->CallReceiver(AResult);
    }
};
//----------------------------------------------------------------------------------------------------------------------

void OnNoticeProcessor(void *AData, LPCSTR AMessage) {
    if (Assigned(AData)) {
        auto LConnection = (CPQConnection *) AData;
        LConnection->CallProcessor(AMessage);
    }
};
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Delphi {

    namespace Postgres {

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnInfo -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQConnInfo::CPQConnInfo(): CPersistent(this) {
            m_Ping = PQPING_NO_ATTEMPT;
            m_PingValid = false;
            m_ExpandDBName = false;
            m_ApplicationName = _T("'") DELPHI_LIB_DESCRIPTION _T("'");
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQConnInfo::CPQConnInfo(LPCSTR AConnInfo): CPQConnInfo() {
            m_ConnInfo = AConnInfo;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQConnInfo::CPQConnInfo(LPCSTR const *AKeywords, LPCSTR const *AValues, bool AExpandDBName): CPQConnInfo() {
            m_ExpandDBName = AExpandDBName;
            for (int I = 0; AKeywords[I]; ++I) {
                m_Parameters.AddPair(AKeywords[I], AValues[I]);
            }
            UpdateConnInfo();
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQConnInfo::CPQConnInfo(LPCSTR AHost, LPCSTR APort, LPCSTR ADataBase, LPCSTR AUserName, LPCSTR APassword,
                                     LPCSTR AOptions): CPQConnInfo() {

            m_Parameters.AddPair(_T("host"), AHost);
            m_Parameters.AddPair(_T("port"), APort);
            m_Parameters.AddPair(_T("dbname"), ADataBase);
            m_Parameters.AddPair(_T("user"), AUserName);
            m_Parameters.AddPair(_T("password"), APassword);
            m_Parameters.AddPair(_T("options"), AOptions);

            UpdateConnInfo();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnInfo::UpdateConnInfo() {
            m_ConnInfo.Clear();

            if (GetValue(_T("host")).IsEmpty())
                m_Parameters.AddPair(_T("host"), _T("localhost"));

            if (GetValue(_T("port")).IsEmpty())
                m_Parameters.AddPair(_T("port"), _T("5432"));

            if (GetValue(_T("dbname")).IsEmpty())
                m_Parameters.AddPair(_T("dbname"), _T("postgres"));

            if (GetValue(_T("user")).IsEmpty())
                m_Parameters.AddPair(_T("user"), _T("postgres"));

            m_ConnInfo = m_Parameters[0];
            for (int I = 1; I < m_Parameters.Count(); ++I)
                m_ConnInfo += _T(" ") + m_Parameters[I];

            const CString &ApplicationName = m_Parameters.Values(_T("application_name"));
            if (ApplicationName.IsEmpty()) {
                if (!m_ApplicationName.IsEmpty())
                    m_ConnInfo += _T(" application_name=") + m_ApplicationName;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CPQConnInfo::GetConnInfo() {
            UpdateConnInfo();
            return m_ConnInfo;
        }
        //--------------------------------------------------------------------------------------------------------------

        PGPing CPQConnInfo::GetPing() {
            if (!m_PingValid)
                m_Ping = PQping(ConnInfo().c_str());
            return m_Ping;
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString& CPQConnInfo::GetValue(LPCSTR AKeyword) const {
            CString Keyword(AKeyword);
            return m_Parameters.Values(Keyword);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnInfo::Clear() {
            m_ConnInfo.Clear();
            m_Parameters.Clear();
            m_ExpandDBName = false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnInfo::Add(LPCSTR Keyword, LPCSTR Value) {
            m_Parameters.AddPair(Keyword, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnInfo::Assign(const CPQConnInfo &Source) {
            m_ApplicationName = Source.m_ApplicationName;
            m_ConnInfo = Source.m_ConnInfo;
            m_Parameters = Source.m_Parameters;
            m_ExpandDBName = Source.m_ExpandDBName;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnInfo::AssignParameters(const CStringList &Strings) {
            Clear();
            if (Strings.Count() > 0) {
                m_Parameters = Strings;
                m_ExpandDBName = GetValue(_T("dbname")).SubString(0, 11) == _T("postgresql:");
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnectionEvent ----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQConnectionEvent::CPQConnectionEvent(CPollManager *AManager): CPollConnection(this, AManager) {
            m_OnReceiver = nullptr;
            m_OnProcessor = nullptr;

            m_OnChangeSocket = nullptr;

            m_OnStatus = nullptr;
            m_OnPollingStatus = nullptr;

            m_OnConnected = nullptr;
            m_OnDisconnected = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQConnectionEvent::DoChangeSocket(CPQConnection *APQConnection, CSocket AOldSocket, CSocket ANewSocket) {
            if (m_OnChangeSocket != nullptr) {
                return m_OnChangeSocket(APQConnection, AOldSocket, ANewSocket);
            }
            return true;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectionEvent::DoStatus(CPQConnection *APQConnection) {
            if (m_OnStatus != nullptr) {
                m_OnStatus(APQConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectionEvent::DoPollingStatus(CPQConnection *APQConnection) {
            if (m_OnPollingStatus != nullptr) {
                m_OnPollingStatus(APQConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectionEvent::DoConnected(CPQConnection *APQConnection) {
            if (m_OnConnected != nullptr) {
                m_OnConnected(APQConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectionEvent::DoDisconnected(CPQConnection *APQConnection) {
            if (m_OnDisconnected != nullptr) {
                m_OnDisconnected(APQConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnection ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQConnection::CPQConnection(CPollManager *AManager): CPQConnectionEvent(AManager) {
            m_Handle = nullptr;
            m_Socket = INVALID_SOCKET;
            m_TryConnect = false;
            m_Status = CONNECTION_BAD;
            m_PollingStatus = PGRES_POLLING_WRITING;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQConnection::CPQConnection(const CPQConnInfo &AConnInfo, CPollManager *AManager): CPQConnection(AManager) {
            m_ConnInfo = AConnInfo;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQConnection::~CPQConnection() {
            Disconnect();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::Clear() {
            m_Handle = nullptr;
            m_Socket = INVALID_SOCKET;
            m_TryConnect = false;
            m_Status = CONNECTION_BAD;
            m_PollingStatus = PGRES_POLLING_WRITING;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::Finish() {
            if (m_TryConnect) {
                PQfinish(m_Handle);
                Clear();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::SetReceiver() {
            if (m_OnReceiver != nullptr)
                PQsetNoticeReceiver(m_Handle, OnNoticeReceiver, this);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::SetProcessor() {
            if (m_OnProcessor != nullptr)
                PQsetNoticeProcessor(m_Handle, OnNoticeProcessor, this);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::SetSocket(CSocket ASocket, int AFlags) {
            if (m_Socket != ASocket) {
                CSocket LOldSocket = m_Socket;
                m_Socket = ASocket;

                if (ASocket != SOCKET_ERROR && AFlags != -1) {
                    if (PQsetnonblocking(m_Handle, AFlags) == -1) {
                        throw EDBConnectionError(_T("[%d] Set non blocking connection failed: %s"), m_Socket,
                                                 GetErrorMessage());
                    }
                }

                if (!DoChangeSocket(this, LOldSocket, ASocket))
                    throw EDBConnectionError(_T("Cannot change (%d) socket to (%d)"), LOldSocket, ASocket);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQConnection::GetErrorMessage() {
            return PQerrorMessage(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQConnection::CheckStatus(int const AIgnore[], int ACount) {
            m_Status = PQstatus(m_Handle);
            DoStatus(this);
            for (int i = 0; i < ACount; ++i)
                if (m_Status == AIgnore[i])
                    return true;
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        ConnStatusType CPQConnection::GetStatus() {
            int Ignore[] = {0};
            CheckStatus(Ignore, 0);
            return m_Status;
        }
        //--------------------------------------------------------------------------------------------------------------

        PostgresPollingStatusType CPQConnection::GetPollingStatus() {
            return m_PollingStatus;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQConnection::GetConnected() {
            return GetStatus() == CONNECTION_OK;
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQConnection::StatusString() {
            switch (m_Status) {
                case CONNECTION_OK:
                    return "Connection success.";
                case CONNECTION_BAD:
                    return "Connection failed.";
                case CONNECTION_STARTED:
                    return "Waiting for connection to be made.";
                case CONNECTION_MADE:
                    return "Connection OK; waiting to send.";
                case CONNECTION_AWAITING_RESPONSE:
                    return "Waiting for a response from the server.";
                case CONNECTION_AUTH_OK:
                    return "Received authentication; waiting for backend start-up to finish.";
                case CONNECTION_SETENV:
                    return "Negotiating environment-driven parameter settings.";
                case CONNECTION_SSL_STARTUP:
                    return "Negotiating SSL encryption.";
                case CONNECTION_NEEDED:
                    return "Internal state: connect() needed.";
                case CONNECTION_CHECK_WRITABLE:
                    return "Check if we could make a writable connection.";
                case CONNECTION_CONSUME:
                    return "Wait for any pending message and consume them.";
            }

            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQConnection::PollingStatusString() {
            switch (m_PollingStatus) {
                case PGRES_POLLING_FAILED:
                    return "Polling failed.";
                case PGRES_POLLING_READING:
                    return "Waiting for socket to become readable.";
                case PGRES_POLLING_WRITING:
                    return "Waiting for socket to become writable.";
                case PGRES_POLLING_OK:
                    return "Polling success.";
                case PGRES_POLLING_ACTIVE:
                    return "Polling active.";
            }

            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::CheckConnection() {
            if (m_Handle == nullptr)
                throw EDBConnectionError(_T("Not enough memory."));

            m_TryConnect = true;

            GetStatus();

            if (m_Status == CONNECTION_BAD)
                throw EDBConnectionError(_T("[%d] Connection failed: %s"), m_Socket, GetErrorMessage());

            if (m_Status == CONNECTION_OK) {
                SetReceiver();
                SetProcessor();
                DoConnected(this);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::Connect() {
            m_Handle = PQconnectdb(m_ConnInfo.ConnInfo().c_str());
            CheckConnection();
            SetSocket(PQsocket(m_Handle));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::ConnectStart() {
            m_Handle = PQconnectStart(m_ConnInfo.ConnInfo().c_str());
            CheckConnection();
            SetSocket(PQsocket(m_Handle), 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::ConnectPoll() {
            m_PollingStatus = PQconnectPoll(m_Handle);
            DoPollingStatus(this);
            CheckConnection();
            SetSocket(PQsocket(m_Handle), 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::ResetStart() {
            if (!PQresetStart(m_Handle))
                throw EDBConnectionError(_T("PQresetStart failed: %s"), GetErrorMessage());
            CheckConnection();
            SetSocket(PQsocket(m_Handle), 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::ResetPoll() {
            m_PollingStatus = PQresetPoll(m_Handle);
            DoPollingStatus(this);
            CheckConnection();
            SetSocket(PQsocket(m_Handle), 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQConnection::PID() {
            if (m_Handle != nullptr)
                return PQbackendPID(m_Handle);
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::ConsumeInput() {
            if (!PQconsumeInput(m_Handle))
                throw EDBConnectionError(_T("PQconsumeInput failed: %s"), GetErrorMessage());
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQConnection::IsBusy() {
            return PQisBusy(m_Handle) == 1;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQConnection::Flush() {
            int Result = PQflush(m_Handle);

            if (Result == -1)
                throw EDBConnectionError(_T("PQflush failed: %s"), GetErrorMessage());

            return Result == 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::Disconnect() {
            if (m_TryConnect) {
                DoDisconnected(this);
                ClosePoll();
                Finish();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::CallReceiver(const PGresult *AResult) {
            if (m_OnReceiver != nullptr) {
                m_OnReceiver(this, AResult);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnection::CallProcessor(LPCSTR AMessage) {
            if (m_OnProcessor != nullptr) {
                m_OnProcessor(this, AMessage);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQPollConnection -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQPollConnection::CPQPollConnection(const CPQConnInfo &AConnInfo, CPollManager *AManager):
                CPQConnection(AConnInfo, AManager) {
            m_ConnectionStatus = qsConnect;
            m_WorkQuery = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQPollConnection::CheckResult() {
            if (m_WorkQuery == nullptr)
                throw EPollServerError(_T("Send Query is null!"));

            return m_WorkQuery->CheckResult();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQPollConnection::QueryStart(CPQQuery *AQuery) {
            m_WorkQuery = AQuery;
            m_WorkQuery->Connection(this);
            m_WorkQuery->SendQuery();
            if (!m_WorkQuery->CheckResult())
                m_ConnectionStatus = qsBusy;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQPollConnection::QueryStop() {
            FreeAndNil(m_WorkQuery);
            m_ConnectionStatus = qsReady;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQResult -------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQResult::CPQResult(CPQQuery *AQQuery, PGresult *AHandle): CCollectionItem(AQQuery) {
            m_Query = AQQuery;
            m_Handle = AHandle;
            m_Status = PGRES_FATAL_ERROR;
            m_OnStatus = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQResult::~CPQResult() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQResult::Clear() {
            if (m_Handle != nullptr)
                PQclear(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        ExecStatusType CPQResult::ResultStatus() {
            if (m_Handle != nullptr) {
                m_Status = PQresultStatus(m_Handle);
                DoStatus();
                return m_Status;
            }
            return PGRES_FATAL_ERROR;
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQResult::GetErrorMessage() {
            return PQresultErrorMessage(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQResult::StatusString() {
            return PQresStatus(m_Status);
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQResult::VerboseErrorMessage(PGVerbosity Verbosity, PGContextVisibility Context) {
            LPCSTR lpError = PQresultVerboseErrorMessage(m_Handle, Verbosity, Context);
            m_VerboseErrorMessage = lpError;
            PQfreemem((void *) lpError);
            return m_VerboseErrorMessage.c_str();
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQResult::ErrorField(int FieldCode) {
            return PQresultErrorField(m_Handle, FieldCode);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::nTuples() {
            return PQntuples(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::nFields() {
            return PQnfields(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQResult::fName(int ColumnNumber) {
            return PQfname(m_Handle, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::fNumber(LPCSTR ColumnName) {
            return PQfnumber(m_Handle, ColumnName);
        }
        //--------------------------------------------------------------------------------------------------------------

        Oid CPQResult::fTable(int ColumnNumber) {
            return PQftable(m_Handle, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::fTableCol(int ColumnNumber) {
            return PQftablecol(m_Handle, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::fFormat(int ColumnNumber) {
            return PQfformat(m_Handle, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        Oid CPQResult::fType(int ColumnNumber) {
            return PQftype(m_Handle, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::fMod(int ColumnNumber) {
            return PQfmod(m_Handle, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::fSize(int ColumnNumber) {
            return PQfsize(m_Handle, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::BinaryTuples() {
            return PQbinaryTuples(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        LPCSTR CPQResult::GetValue(int RowNumber, int ColumnNumber) {
            return PQgetvalue(m_Handle, RowNumber, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::GetIsNull(int RowNumber, int ColumnNumber) {
            return PQgetisnull(m_Handle, RowNumber, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::GetLength(int RowNumber, int ColumnNumber) {
            return PQgetlength(m_Handle, RowNumber, ColumnNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQResult::nParams() {
            return PQnparams(m_Handle);
        }
        //--------------------------------------------------------------------------------------------------------------

        Oid CPQResult::ParamType(int ParamNumber) {
            return PQparamtype(m_Handle, ParamNumber);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQResult::DoStatus() {
            if (m_OnStatus != nullptr) {
                m_OnStatus(this);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQQuery --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQQuery::CPQQuery(): CCollection(this) {
            m_Connection = nullptr;

            m_OnSendQuery = nullptr;
            m_OnExecuted = nullptr;

            m_OnResultStatus = nullptr;
            m_OnResult = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQQuery::CPQQuery(CPQConnection *AConnection): CPQQuery() {
            m_Connection = AConnection;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQQuery::~CPQQuery() {
            Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQResult *CPQQuery::GetResult(int Index) {
            return (CPQResult *) inherited::GetItem(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQQuery::SetConnection(CPQConnection *Value) {
            if (m_Connection != Value) {
                m_Connection = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQQuery::Clear() {
            CCollection::Clear();
            m_SQL.Clear();
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQQuery::CheckResult() {
            CPQResult *LResult;
            PGresult *Result;
            m_Connection->ConsumeInput();
            if (m_Connection->Flush() && !m_Connection->IsBusy()) {

                Result = PQgetResult(m_Connection->Handle());
                while (Result) {
                    LResult = new CPQResult(this, Result);
                    LResult->OnStatus(std::bind(&CPQQuery::DoResultStatus, this, _1));
                    DoResult(LResult, LResult->ResultStatus());
                    Result = PQgetResult(m_Connection->Handle());
                };

                DoExecuted();

                return true;
            }
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQQuery::SendQuery() {
            if (m_Connection == nullptr)
                throw EDBError(_T("Not set connection!"));

            if (!m_Connection->Connected())
                throw EDBError(_T("Not connected!"));

            if (m_SQL.Count() == 0)
                throw EDBError(_T("Empty SQL query!"));

            if (PQsendQuery(m_Connection->Handle(), m_SQL.Text().c_str()) == 0) {
                throw EDBError("PQsendQuery failed: %s", m_Connection->GetErrorMessage());
            }

            DoSendQuery();

            m_Connection->Flush();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQQuery::DoSendQuery() {
            if (m_OnSendQuery != nullptr) {
                try {
                    m_OnSendQuery(this);
                } catch (...) {
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQQuery::DoResultStatus(CPQResult *AResult) {
            if (m_OnResultStatus != nullptr) {
                try {
                    m_OnResultStatus(AResult);
                } catch (...) {
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQQuery::DoResult(CPQResult *AResult, ExecStatusType AExecStatus) {
            if (m_OnResult != nullptr) {
                try {
                    m_OnResult(AResult, AExecStatus);
                } catch (...) {
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQQuery::DoExecuted() {
            if (m_OnExecuted != nullptr) {
                try {
                    m_OnExecuted(this);
                } catch (...) {
                }
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQPollQuery ----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery::CPQPollQuery(CPQConnectPoll *AServer): CPQQuery(), CCollectionItem(AServer->PollQueryManager()) {
            m_Server = AServer;
            m_PollConnection = nullptr;

            m_OnExecuted = nullptr;
            m_OnException = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQPollQuery::QueryStart() {
            try {
                auto LConnection = m_Server->GetReadyConnection();

                if (LConnection != nullptr) {
                    try {
                        LConnection->QueryStart(this);
                    } catch (Exception::Exception &E) {
                        DoException(&E);
                        LConnection->QueryStop();
                    }
                } else {

                    if (m_Server->Queue()->Count() == 0xFFFF)
                        throw EPollServerError(_T("Request queue is full!"));

                    return AddToQueue();
                }

                return -1;
            } catch (Exception::Exception &E) {
                DoException(&E);
            }

            return POLL_QUERY_START_ERROR;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CPQPollQuery::AddToQueue() {
            if (m_PollConnection != nullptr) {
                return m_Server->Queue()->AddToQueue(m_PollConnection, this);
            }
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQPollQuery::RemoveFromQueue() {
            if (m_PollConnection != nullptr) {
                m_Server->Queue()->RemoveFromQueue(m_PollConnection, this);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQPollQuery::DoExecuted() {
            if (m_OnExecuted != nullptr) {
                try {
                    m_OnExecuted(this);
                } catch (...) {
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQPollQuery::DoException(Exception::Exception *AException) {
            if (m_OnException != nullptr) {
                try {
                    m_OnException(this, AException);
                } catch (...) {
                }
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnectPollEvent ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQConnectPollEvent::CPQConnectPollEvent(): CObject() {
            m_OnReceiver = nullptr;
            m_OnProcessor = nullptr;

            m_OnStatus = nullptr;
            m_OnPollingStatus = nullptr;

            m_OnConnectException = nullptr;
            m_OnServerException = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPollEvent::DoReceiver(CPQConnection *AConnection, const PGresult *AResult) {
            if (m_OnReceiver != nullptr) {
                m_OnReceiver(AConnection, AResult);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPollEvent::DoProcessor(CPQConnection *AConnection, LPCSTR AMessage) {
            if (m_OnProcessor != nullptr) {
                m_OnProcessor(AConnection, AMessage);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPollEvent::DoStatus(CPQConnection *AConnection) {
            if (m_OnStatus != nullptr) {
                m_OnStatus(AConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPollEvent::DoPollingStatus(CPQConnection *AConnection) {
            if (m_OnPollingStatus != nullptr) {
                m_OnPollingStatus(AConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPollEvent::DoConnectException(CPQConnection *AConnection,
                Exception::Exception *AException) {
            if (m_OnConnectException != nullptr) {
                m_OnConnectException(AConnection, AException);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnectPoll --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQConnectPoll::CPQConnectPoll(size_t ASizeMin, size_t ASizeMax): CPQConnectPollEvent(), CEPollClient() {
            m_Active = false;

            m_SizeMin = ASizeMin;
            m_SizeMax = ASizeMax;

            m_Queue = new CQueue;

            m_PollQueryManager = new CPQPollQueryManager();
            m_PollManager = new CPollManager;

            m_OnEventHandlerException = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQConnectPoll::CPQConnectPoll(const CPQConnInfo &AConnInfo, size_t ASizeMin, size_t ASizeMax):
            CPQConnectPoll(ASizeMin, ASizeMax) {
            m_ConnInfo = AConnInfo;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQConnectPoll::~CPQConnectPoll() {
            StopAll();
            FreeAndNil(m_Queue);
            FreeAndNil(m_PollManager);
            FreeAndNil(m_PollQueryManager);
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollConnection *CPQConnectPoll::GetConnection(CPollEventHandler *AHandler) {
            return dynamic_cast<CPQPollConnection *> (AHandler->Binding());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::Start() {
            for (size_t I = 0; I < m_SizeMin; ++I) {
                if (!NewConnection())
                    break;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::Stop(CPollEventHandler *AHandler) {
            delete AHandler;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::StopAll() {
            for (int I = m_EventHandlers->Count() - 1; I >= 0; --I) {
                Stop(m_EventHandlers->Handlers(I));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::SetActive(bool Value) {
            if (m_Active != Value) {
                m_Active = Value;
                if (Value) {
                    Start();
                }  else {
                    StopAll();
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CPollEventHandler *CPQConnectPoll::NewEventHandler(CPQConnection *AConnection) {
            CPollEventHandler *LEventHandler = nullptr;

            try {
                LEventHandler = m_EventHandlers->Add(AConnection->Socket());

                if (ExternalPollStack()) {
                    LEventHandler->OnTimeOutEvent(std::bind(&CPQConnectPoll::DoTimeOut, this, _1));
                    LEventHandler->OnReadEvent(std::bind(&CPQConnectPoll::DoRead, this, _1));
                    LEventHandler->OnWriteEvent(std::bind(&CPQConnectPoll::DoWrite, this, _1));
                }

                LEventHandler->Binding(AConnection, true);
                LEventHandler->Start(etIO);

            } catch (Exception::Exception &E) {
                DoServerException(&E);
                FreeAndNil(LEventHandler);
            }

            return LEventHandler;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQConnectPoll::OnChangeSocket(CPQConnection *AConnection, CSocket AOldSocket, CSocket ANewSocket) {
            bool Result = true;
            CPollEventHandler *LEventHandler = nullptr;

            if (AOldSocket != SOCKET_ERROR) {
                LEventHandler = m_EventHandlers->FindHandlerBySocket(AOldSocket);
                if (Assigned(LEventHandler)) {
                    LEventHandler->Stop();
                }
            }

            if (ANewSocket != SOCKET_ERROR) {
                Result = (NewEventHandler(AConnection) != nullptr);
            }

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQConnectPoll::NewConnection() {
            int PingCount = 0;
            auto LConnection = new CPQPollConnection(m_ConnInfo, m_PollManager);

            try {

                m_ConnInfo.PingValid(m_ConnInfo.Ping() == PQPING_OK);
                while (!m_ConnInfo.PingValid() && (PingCount < 3)) {
                    sleep(1);
                    PingCount++;
                    m_ConnInfo.PingValid(m_ConnInfo.Ping() == PQPING_OK);
                }

                if (!m_ConnInfo.PingValid()) {
                    switch (m_ConnInfo.Ping()) {
                        case PQPING_OK:
                            m_ConnInfo.PingValid(true);
                            break;

                        case PQPING_REJECT:
                            throw EDBConnectionError(
                                    _T("The server is running but is in a state that disallows connections "
                                       "(startup, shutdown, or crash recovery)."));

                        case PQPING_NO_RESPONSE:
                            throw EDBConnectionError(_T("The server could not be contacted."));

                        case PQPING_NO_ATTEMPT:
                            throw EDBConnectionError(
                                    _T("No attempt was made to contact the server, because the supplied parameters "
                                       "were obviously incorrect or there was some client-side problem "
                                       "(for example, out of memory)."));
                    }
                }

                LConnection->OnChangeSocket(std::bind(&CPQConnectPoll::OnChangeSocket, this, _1, _2, _3));

                if (m_OnReceiver != nullptr) {
                    LConnection->OnReceiver(std::bind(&CPQConnectPoll::DoReceiver, this, _1, _2));
                }

                if (m_OnProcessor != nullptr) {
                    LConnection->OnProcessor(std::bind(&CPQConnectPoll::DoProcessor, this, _1, _2));
                }

                if (m_OnStatus != nullptr) {
                    LConnection->OnStatus(std::bind(&CPQConnectPoll::DoStatus, this, _1));
                }

                if (m_OnPollingStatus != nullptr) {
                    LConnection->OnPollingStatus(std::bind(&CPQConnectPoll::DoPollingStatus, this, _1));
                }

                if (m_OnConnected != nullptr) {
                    LConnection->OnConnected(std::bind(&CPQConnectPoll::DoConnected, this, _1));
                }

                if (m_OnDisconnected != nullptr) {
                    LConnection->OnDisconnected(std::bind(&CPQConnectPoll::DoDisconnected, this, _1));
                }

                LConnection->ConnectStart();
                LConnection->ConnectPoll();

                return true;
            } catch (Exception::Exception &E) {
                DoConnectException(LConnection, &E);
                delete LConnection;
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollConnection *CPQConnectPoll::GetReadyConnection() {
            CPQPollConnection *LConnection = nullptr;
            CPQPollConnection *LResult = nullptr;

            for (int I = 0; I < m_EventHandlers->Count(); ++I) {
                LConnection = GetConnection(m_EventHandlers->Handlers(I));

                if (LConnection->Connected()) {
                    if (LConnection->ConnectionStatus() == qsReady) {
                        LResult = LConnection;
                        break;
                    }
                } else {
                    if (LConnection->Status() == CONNECTION_BAD) {
                        LConnection->ConnectionStatus(qsReset);
                        LConnection->ResetStart();
                        LConnection->ResetPoll();
                    }
                }
            }

            if (LResult == nullptr && (m_PollManager->Count() < m_SizeMax)) {
                if (!NewConnection())
                    throw Exception::EDBConnectionError(_T("Unable to create new database connection."));
            }

            return LResult;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::DoTimeOut(CPollEventHandler *AHandler) {
            Stop(AHandler);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::DoConnect(CPollEventHandler *AHandler) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::DoRead(CPollEventHandler *AHandler) {
            auto LConnection = GetConnection(AHandler);
            try {
                switch (LConnection->ConnectionStatus()) {
                    case qsConnect:
                        LConnection->ConnectPoll();
                        break;

                    case qsReset:
                        LConnection->ResetPoll();
                        break;

                    case qsReady:
                        // Connection closed gracefully
                        m_ConnInfo.PingValid(false);

                        LConnection->ConnectionStatus(qsError);
                        LConnection->Disconnect();
                        break;

                    case qsBusy:
                        if (LConnection->CheckResult())
                            LConnection->QueryStop();
                        break;

                    default:
                        m_ConnInfo.PingValid(false);
                        break;
                }
            } catch (Exception::Exception &E) {
                DoConnectException(LConnection, &E);
                m_ConnInfo.PingValid(false);
                LConnection->Disconnect();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::DoWrite(CPollEventHandler *AHandler) {
            auto LConnection = GetConnection(AHandler);
            try {
                switch (LConnection->ConnectionStatus()) {
                    case qsConnect:
                        if (LConnection->Connected()) {
                            LConnection->ConnectionStatus(qsReady);
                            CheckQueue();
                        } else {
                            LConnection->ConnectPoll();
                        }
                        break;

                    case qsReset:
                        if (LConnection->Connected()) {
                            LConnection->ConnectionStatus(qsReady);
                            CheckQueue();
                        } else {
                            LConnection->ResetPoll();
                        }
                        break;

                    case qsReady:
                        LConnection->Flush();

                        if (m_Queue->Count() == 0 && m_PollManager->Count() > m_SizeMax) {
                            LConnection->Disconnect();
                        }

                        CheckQueue();
                        break;

                    case qsBusy:
                        LConnection->Flush();
                        break;

                    default:
                        m_ConnInfo.PingValid(false);
                        break;
                }
            } catch (Exception::Exception &E) {
                DoConnectException(LConnection, &E);
                m_ConnInfo.PingValid(false);
                LConnection->Disconnect();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQConnectPoll::CheckQueue() {
            CPQPollQuery *LPollQuery;
            if (m_Queue->Count() > 0) {
                LPollQuery = (CPQPollQuery *) m_Queue->First()->First();
                LPollQuery->RemoveFromQueue();
                LPollQuery->QueryStart();
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //- CPQServer --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CPQServer::GetQuery() {
            return new CPQPollQuery(this);
        }
        //--------------------------------------------------------------------------------------------------------------

        CPQPollQuery *CPQServer::FindQueryByConnection(CPollConnection *APollConnection) {
            for (int I = 0; I < PollQueryManager()->QueryCount(); ++I) {
                if (PollQueryManager()->Queries(I)->PollConnection() == APollConnection)
                    return PollQueryManager()->Queries(I);
            }
            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::DoServerException(Exception::Exception *AException) {
            if (m_OnServerException != nullptr) {
                m_OnServerException(this, AException);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQServer::DoCommand(CTCPConnection *AConnection) {
            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQServer::DoExecute(CTCPConnection *AConnection) {
            return CEPollClient::DoExecute(AConnection);
        }
    }
}
}
