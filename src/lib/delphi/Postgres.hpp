/*++

Library name:

  libdelphi

Module Name:

  Postgres.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_POSTGRES_HPP
#define DELPHI_POSTGRES_HPP
//----------------------------------------------------------------------------------------------------------------------

#include "postgresql/libpq-fe.h"

extern "C++" {

namespace Delphi {

    namespace Postgres {

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnInfo -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQConnInfo: public CPersistent {
        private:

            PGPing m_Ping;

            CString m_ConnInfo;

            CStringList m_Parameters;

            CString m_ApplicationName;

            bool m_ExpandDBName;

            bool m_PingValid;

            void UpdateConnInfo();

            void AssignParameters(const CStringList& Strings);

        protected:

            PGPing GetPing();

            const CString& GetValue(LPCSTR Keyword) const;

            const CString& GetConnInfo();

        public:

            CPQConnInfo();

            explicit CPQConnInfo(LPCSTR AConnInfo);

            explicit CPQConnInfo(LPCSTR const *AKeywords, LPCSTR const *AValues, bool AExpandDBName = false);

            explicit CPQConnInfo(LPCSTR AHost, LPCSTR APort, LPCSTR ADataBase, LPCSTR AUserName, LPCSTR APassword,
                LPCSTR AOptions);

            const CString& ConnInfo() { return GetConnInfo(); };

            PGPing Ping() { return GetPing(); };

            bool PingValid() { return m_PingValid; };

            void PingValid(bool Value) { m_PingValid = Value; };

            void Add(LPCSTR Keyword, LPCSTR Value);

            void Clear();

            void Assign(const CPQConnInfo &Source);

            bool ExpandDBName() { return m_ExpandDBName; };
            void ExpandDBName(bool Value) { m_ExpandDBName = Value; };

            void SetParameters(const CStringList& Strings) { AssignParameters(Strings); };

            CStringList& GetParameters() { return m_Parameters; };
            const CStringList& GetParameters() const { return m_Parameters; };

            CString& ApplicationName() { return m_ApplicationName; };
            const CString& ApplicationName() const { return m_ApplicationName; };

            const CString& Parameters(LPCSTR Keyword) const { return GetValue(Keyword); };

            CPQConnInfo& operator=(const CPQConnInfo& Info) {
                if (&Info != this) {
                    Assign(Info);
                }
                return *this;
            }

            const CString& operator[](LPCSTR Keyword) const { return GetValue(Keyword); }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnection ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQConnection;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPQConnection *AConnection, const PGresult *AResult)> COnPQConnectionReceiverEvent;
        typedef std::function<void (CPQConnection *AConnection, LPCSTR AMessage)> COnPQConnectionProcessorEvent;

        typedef std::function<void (CPQConnection *AConnection)> COnPQConnectionEvent;
        typedef std::function<bool (CPQConnection *AConnection, CSocket AOldSocket, CSocket ANewSocket)> COnPQConnectionChangeSocketEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CPQConnectionEvent: public CPollConnection {
            friend CPQConnection;

        private:

            COnPQConnectionReceiverEvent m_OnReceiver;
            COnPQConnectionProcessorEvent m_OnProcessor;

            COnPQConnectionChangeSocketEvent m_OnChangeSocket;

            COnPQConnectionEvent m_OnStatus;
            COnPQConnectionEvent m_OnPollingStatus;

            COnPQConnectionEvent m_OnConnected;
            COnPQConnectionEvent m_OnDisconnected;

        protected:

            void DoStatus(CPQConnection *AConnection);
            void DoPollingStatus(CPQConnection *AConnection);
            void DoConnected(CPQConnection *AConnection);
            void DoDisconnected(CPQConnection *AConnection);

            bool DoChangeSocket(CPQConnection *AConnection, CSocket ANewSocket, CSocket AOldSocket);

        public:

            explicit CPQConnectionEvent(CPollManager *AManager);

            ~CPQConnectionEvent() override = default;

            const COnPQConnectionReceiverEvent &OnReceiver() { return m_OnReceiver; }
            void OnReceiver(COnPQConnectionReceiverEvent && Value) { m_OnReceiver = Value; }

            const COnPQConnectionProcessorEvent &OnProcessor() { return m_OnProcessor; }
            void OnProcessor(COnPQConnectionProcessorEvent && Value) { m_OnProcessor = Value; }

            const COnPQConnectionChangeSocketEvent &OnChangeSocket() { return m_OnChangeSocket; }
            void OnChangeSocket(COnPQConnectionChangeSocketEvent && Value) { m_OnChangeSocket = Value; }

            const COnPQConnectionEvent &OnStatus() { return m_OnStatus; }
            void OnStatus(COnPQConnectionEvent && Value) { m_OnStatus = Value; }

            const COnPQConnectionEvent &OnPollingStatus() { return m_OnPollingStatus; }
            void OnPollingStatus(COnPQConnectionEvent && Value) { m_OnPollingStatus = Value; }

            const COnPQConnectionEvent &OnConnected() { return m_OnConnected; }
            void OnConnected(COnPQConnectionEvent && Value) { m_OnConnected = Value; }

            const COnPQConnectionEvent &OnDisconnected() { return m_OnDisconnected; }
            void OnDisconnected(COnPQConnectionEvent && Value) { m_OnDisconnected = Value; }

        };
        //--------------------------------------------------------------------------------------------------------------

        class CPQConnection: public CPQConnectionEvent {
        private:

            CPQConnInfo m_ConnInfo;

            PGconn *m_Handle;

            ConnStatusType m_Status;

            PostgresPollingStatusType m_PollingStatus;

            CSocket m_Socket;

            bool m_TryConnect;

            bool GetConnected();

            void SetSocket(CSocket ASocket, int AFlags = -1);

            void SetReceiver();

            void SetProcessor();

        protected:

            void Clear();

            void Finish();

            void CheckConnection();

            ConnStatusType GetStatus();

            PostgresPollingStatusType GetPollingStatus();

            bool CheckStatus(int const AIgnore[], int ACount);

        public:

            explicit CPQConnection(CPollManager *AManager);

            explicit CPQConnection(const CPQConnInfo &AConnInfo, CPollManager *AManager);

            ~CPQConnection() override;

            void ConnInfo(const CPQConnInfo &AConnInfo) { m_ConnInfo = AConnInfo; };

            CPQConnInfo &ConnInfo() { return m_ConnInfo; };
            const CPQConnInfo &ConnInfo() const { return m_ConnInfo; };

            void CallReceiver(const PGresult *AResult);

            void CallProcessor(LPCSTR AMessage);

            LPCSTR GetErrorMessage();

            LPCSTR StatusString();

            LPCSTR PollingStatusString();

            ConnStatusType Status() { return GetStatus(); };

            PostgresPollingStatusType PollingStatus() { return GetPollingStatus(); };

            CSocket Socket() { return m_Socket; };

            int PID();

            void Connect();

            void ConnectStart();

            void ConnectPoll();

            void ResetStart();

            void ResetPoll();

            void ConsumeInput();

            bool IsBusy();

            bool Flush();

            void Disconnect() override;

            bool Connected() { return GetConnected(); };

            PGconn *Handle() { return m_Handle; };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQPollConnection -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQQuery;
        //--------------------------------------------------------------------------------------------------------------

        enum CPollConnectionStatus { qsConnect, qsReset, qsReady, qsBusy, qsError };
        //--------------------------------------------------------------------------------------------------------------

        class CPQPollConnection: public CPQConnection {
        private:

            CPQQuery *m_WorkQuery;

            CPollConnectionStatus m_ConnectionStatus;

        public:

            explicit CPQPollConnection(const CPQConnInfo &AConnInfo, CPollManager *AManager);

            void QueryStart(CPQQuery *AQuery);
            void QueryStop();

            CPollConnectionStatus ConnectionStatus() { return m_ConnectionStatus; };

            void ConnectionStatus(CPollConnectionStatus Value) { m_ConnectionStatus = Value; };

            bool CheckResult();
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQResult -------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQResult;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPQResult *AResult)> COnPQResultEvent;
        typedef std::function<void (CPQResult *AResult, ExecStatusType AExecStatus)> COnPQExecResultEvent;
        typedef std::function<void (CPQResult *AResult, Exception::Exception *AException)> COnPQResultExceptionEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CPQResult: public CCollectionItem {
        private:

            PGresult *m_Handle;

            CPQQuery *m_Query;

            ExecStatusType m_Status;

            CString m_VerboseErrorMessage;

            COnPQResultEvent m_OnStatus;

        protected:

            virtual void DoStatus();

        public:

            explicit CPQResult(CPQQuery *AQQuery, PGresult *AHandle);

            ~CPQResult() override;

            void Clear();

            PGresult *Handle() { return m_Handle; };

            CPQQuery *Query() { return m_Query; };

            ExecStatusType ResultStatus();

            ExecStatusType ExecStatus() { return m_Status; };

            int nTuples();

            int nFields();

            LPCSTR fName(int ColumnNumber);

            int fNumber(LPCSTR ColumnName);

            Oid fTable(int ColumnNumber);

            int fTableCol(int ColumnNumber);

            int fFormat(int ColumnNumber);

            Oid fType(int ColumnNumber);

            int fMod(int ColumnNumber);

            int fSize(int ColumnNumber);

            int BinaryTuples();

            LPCSTR GetValue(int RowNumber, int ColumnNumber);

            int GetIsNull(int RowNumber, int ColumnNumber);

            int GetLength(int RowNumber, int ColumnNumber);

            int nParams();

            Oid ParamType(int ParamNumber);

            LPCSTR GetErrorMessage();

            LPCSTR StatusString();

            LPCSTR VerboseErrorMessage(PGVerbosity Verbosity = PQERRORS_DEFAULT,
                    PGContextVisibility Context = PQSHOW_CONTEXT_ERRORS);

            LPCSTR ErrorField(int FieldCode);

            const COnPQResultEvent &OnStatus() { return m_OnStatus; }
            void OnStatus(COnPQResultEvent && Value) { m_OnStatus = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQQuery --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQQuery;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPQQuery *AQuery)> COnPQQueryExecutedEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CPQQuery: public CCollection {
            typedef CCollection inherited;

        private:

            CPQConnection *m_Connection;

            CStringList m_SQL;

            COnPQQueryExecutedEvent m_OnSendQuery;
            COnPQQueryExecutedEvent m_OnExecuted;

            COnPQResultEvent m_OnResultStatus;
            COnPQExecResultEvent m_OnResult;

            CPQResult *GetResult(int Index);

        protected:

            virtual void DoExecuted();

            void DoSendQuery();
            void DoResultStatus(CPQResult *AResult);
            void DoResult(CPQResult *AResult, ExecStatusType AExecStatus);

            void SetConnection(CPQConnection *Value);

        public:

            CPQQuery();

            explicit CPQQuery(CPQConnection *AConnection);

            ~CPQQuery() override;

            CPQConnection *Connection() { return m_Connection; };

            void Connection(CPQConnection *Value) { SetConnection(Value); };

            void Clear() override;

            bool CheckResult();

            int ResultCount() { return inherited::Count(); };

            void SendQuery();

            CStringList& SQL() { return m_SQL; }

            const CStringList& SQL() const { return m_SQL; }

            CPQResult *Results(int Index) { return GetResult(Index); };

            const COnPQQueryExecutedEvent &OnExecuted() { return m_OnExecuted; }
            void OnExecute(COnPQQueryExecutedEvent && Value) { m_OnExecuted = Value; }

            const COnPQQueryExecutedEvent &OnSendQuery() { return m_OnSendQuery; }
            void OnSendQuery(COnPQQueryExecutedEvent && Value) { m_OnSendQuery = Value; }

            const COnPQResultEvent &OnResultStatus() { return m_OnResultStatus; }
            void OnResultStatus(COnPQResultEvent && Value) { m_OnResultStatus = Value; }

            const COnPQExecResultEvent &OnResult() { return m_OnResult; }
            void OnResult(COnPQExecResultEvent && Value) { m_OnResult = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //- CPQPollQuery -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQConnectPoll;
        class CPQPollQuery;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPQPollQuery *APollQuery)> COnPQPollQueryExecutedEvent;
        typedef std::function<void (CPQPollQuery *APollQuery, Exception::Exception *AException)> COnPQPollQueryExceptionEvent;
        //--------------------------------------------------------------------------------------------------------------

        #define POLL_QUERY_START_ERROR 0x10000
        //--------------------------------------------------------------------------------------------------------------

        class CPQPollQuery: public CPQQuery, public CCollectionItem {
        private:

            CPQConnectPoll *m_Server;

            CPollConnection *m_PollConnection;

            COnPQPollQueryExecutedEvent m_OnExecuted;

            COnPQPollQueryExceptionEvent m_OnException;

        protected:

            void DoExecuted() override;

            void DoException(Exception::Exception *AException);

        public:

            explicit CPQPollQuery(CPQConnectPoll *AServer);

            int QueryStart();

            int AddToQueue();

            void RemoveFromQueue();

            CPQConnectPoll *Server() { return m_Server; };

            CPollConnection *PollConnection() { return m_PollConnection; };
            void PollConnection(CPollConnection *Value) { m_PollConnection = Value; };

            const COnPQPollQueryExecutedEvent &OnPollExecuted() { return m_OnExecuted; }
            void OnPollExecuted(COnPQPollQueryExecutedEvent && Value) { m_OnExecuted = Value; }

            const COnPQPollQueryExceptionEvent &OnException() { return m_OnException; }
            void OnException(COnPQPollQueryExceptionEvent && Value) { m_OnException = Value; }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQPollQueryManager ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQPollQueryManager: public CCollection {
        public:

            explicit CPQPollQueryManager(): CCollection(this) {

            };

            ~CPQPollQueryManager() override = default;

            int QueryCount() { return CCollection::Count(); };

            CPQPollQuery *Queries(int Index) { return (CPQPollQuery *) CCollection::Items(Index); };

            CPQPollQuery *operator[] (int Index) override { return Queries(Index); };

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQConnectPollEvent ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQServer;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPQConnection *AConnection, Exception::Exception *AException)> COnPQConnectionExceptionEvent;
        typedef std::function<void (CPQServer *AServer, Exception::Exception *AException)> COnPQServerExceptionEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CPQConnectPollEvent: public CObject {
        protected:

            COnPQConnectionReceiverEvent m_OnReceiver;
            COnPQConnectionProcessorEvent m_OnProcessor;

            COnPQConnectionEvent m_OnStatus;
            COnPQConnectionEvent m_OnPollingStatus;

            COnPQConnectionExceptionEvent m_OnConnectException;
            COnPQServerExceptionEvent m_OnServerException;

            virtual void DoReceiver(CPQConnection *AConnection, const PGresult *AResult);
            virtual void DoProcessor(CPQConnection *AConnection, LPCSTR AMessage);

            virtual void DoStatus(CPQConnection *AConnection);
            virtual void DoPollingStatus(CPQConnection *AConnection);

            virtual void DoConnectException(CPQConnection *AConnection, Exception::Exception *AException);
            virtual void DoServerException(Exception::Exception *AException) abstract;

        public:

            CPQConnectPollEvent();

            const COnPQConnectionReceiverEvent &OnReceiver() { return m_OnReceiver; }
            void OnReceiver(COnPQConnectionReceiverEvent && Value) { m_OnReceiver = Value; }

            const COnPQConnectionProcessorEvent &OnProcessor() { return m_OnProcessor; }
            void OnProcessor(COnPQConnectionProcessorEvent && Value) { m_OnProcessor = Value; }

            const COnPQConnectionEvent &OnStatus() { return m_OnStatus; }
            void OnStatus(COnPQConnectionEvent && Value) { m_OnStatus = Value; }

            const COnPQConnectionEvent &OnPollingStatus() { return m_OnPollingStatus; }
            void OnPollingStatus(COnPQConnectionEvent && Value) { m_OnPollingStatus = Value; }

            const COnPQConnectionExceptionEvent &OnConnectException() { return m_OnConnectException; }
            void OnConnectException(COnPQConnectionExceptionEvent && Value) { m_OnConnectException = Value; }

            const COnPQServerExceptionEvent &OnServerException() { return m_OnServerException; }
            void OnServerException(COnPQServerExceptionEvent && Value) { m_OnServerException = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //- CPQConnectPoll ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQConnectPoll: public CPQConnectPollEvent, public CEPollClient {
            friend CPQPollQuery;

        private:

            CPQConnInfo m_ConnInfo;

            CPQPollQueryManager *m_PollQueryManager;

            CPollManager *m_PollManager;

            CQueue *m_Queue;

            size_t m_SizeMin;
            size_t m_SizeMax;

            bool m_Active;

            COnPollEventHandlerExceptionEvent m_OnEventHandlerException;

            void CheckQueue();

            CPollEventHandler *NewEventHandler(CPQConnection *AConnection);

            CPQPollConnection *GetConnection(CPollEventHandler *AHandler);

            bool OnChangeSocket(CPQConnection *AConnection, CSocket AOldSocket, CSocket ANewSocket);

        protected:

            void Start();

            void Stop(CPollEventHandler *AHandler);

            void StopAll();

            CPQPollConnection *GetReadyConnection();

            bool NewConnection();

            void DoTimeOut(CPollEventHandler *AHandler) override;
            void DoConnect(CPollEventHandler *AHandler) override;
            void DoRead(CPollEventHandler *AHandler) override;
            void DoWrite(CPollEventHandler *AHandler) override;

            void SetActive(bool Value);

        public:

            explicit CPQConnectPoll(size_t ASizeMin, size_t ASizeMax);

            explicit CPQConnectPoll(const CPQConnInfo &AConnInfo, size_t ASizeMin = 5, size_t ASizeMax = 10);

            ~CPQConnectPoll() override;

            void ConnInfo(const CPQConnInfo &AConnInfo) { m_ConnInfo = AConnInfo; };

            CPQConnInfo &ConnInfo() { return m_ConnInfo; };
            const CPQConnInfo &ConnInfo() const { return m_ConnInfo; };

            bool Active() { return m_Active; };
            void Active(bool Value) { SetActive(Value); };

            CQueue *Queue() { return m_Queue; };

            CPQPollQueryManager *PollQueryManager() { return m_PollQueryManager; };

            size_t SizeMin() { return m_SizeMin; }
            void SizeMin(size_t Value) { m_SizeMin = Value; }

            size_t SizeMax() { return m_SizeMax; }
            void SizeMax(size_t Value) { m_SizeMax = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //- CPQServer --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQServer: public CPQConnectPoll {
        protected:

            bool DoCommand(CTCPConnection *AConnection) override;
            bool DoExecute(CTCPConnection *AConnection) override;

            void DoServerException(Exception::Exception *AException) override;

        public:

            explicit CPQServer(size_t ASizeMin = 5, size_t ASizeMax = 10): CPQConnectPoll(ASizeMin, ASizeMax) {};

            ~CPQServer() override = default;

            CPQPollQuery *GetQuery();

            CPQPollQuery *FindQueryByConnection(CPollConnection *APollConnection);

        };
    }
}

using namespace Delphi::Postgres;
}

#endif //DELPHI_POSTGRES_HPP
