/*++

Programm name:

  Apostol

Module Name:

  Process.cpp

Notices:

  Apostol Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Apostol.hpp"
#include "Process.hpp"
//----------------------------------------------------------------------------------------------------------------------

void
signal_error(int signo, siginfo_t *siginfo, void *ucontext)
{
    void*       addr;
    void*       trace[16];
    int         i;
    int         size;
    char      **msg;

    GLog->Error(LOG_CRIT, 0, "signal: %d (%s), addr: 0x%xL", signo, sys_siglist[signo], siginfo->si_addr);

#if __WORDSIZE == 64
    addr = (void*)((ucontext_t*)ucontext)->uc_mcontext.gregs[REG_RIP];
#else
    addr = (void*)((ucontext_t*)ucontext)->uc_mcontext.gregs[REG_EIP];
#endif

    size = backtrace(trace, 16);
    trace[1] = addr;

    msg = backtrace_symbols(trace, size);
    if (msg)
    {
        GLog->Error(LOG_DEBUG, 0, "-= backtrace log =-");

        for (i = 1; i < size; ++i)
        {
            GLog->Error(LOG_DEBUG, 0, "%s", msg[i]);
        }

        GLog->Error(LOG_DEBUG, 0, "-= backtrace log =-");
        free(msg);
    }

    //sig_fatal = 1;
    exit(2);
}
//----------------------------------------------------------------------------------------------------------------------

void
signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{
    try
    {
        Application->SignalProcess()->SignalHandler(signo, siginfo, ucontext);
    }
    catch (std::exception& e)
    {
        log_failure(e.what());
    }
    catch (...)
    {
        log_failure("Unknown fatality error...");
    }
}
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Process {

        //--------------------------------------------------------------------------------------------------------------

        //-- CCustomProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CCustomProcess::CCustomProcess(CProcessType AType, CCustomProcess *AParent): CObject(), CGlobalComponent(),
            m_Type(AType), m_pParent(AParent) {

            m_Pid = MainThreadID;

            m_fDaemonized = false;
            m_NewBinary = 0;

            Name(PROCESS_TYPE_NAME[m_Type]);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomProcess::Assign(CCustomProcess *AProcess) {
            m_fDaemonized = AProcess->Daemonized();
            m_NewBinary = AProcess->NewBinary();
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CCustomProcess::ParentId() {
            if (Assigned(m_pParent))
                return m_pParent->Pid();
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomProcess::Terminate() {
            kill(m_Pid, signal_value(SIG_TERMINATE_SIGNAL));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomProcess::ExecuteProcess(CExecuteContext *AContext)
        {
            if (execve(AContext->path, AContext->argv, AContext->envp) == -1) {
                throw EOSError(errno, _T("execve() failed while executing %s \"%s\""), AContext->name, AContext->path);
            }

            exit(1);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSignal ---------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSignal::CSignal(CCollection *ACollection, int ASigno): CCollectionItem(ACollection), m_signo(ASigno) {
            m_code = nullptr;
            m_name = nullptr;
            m_handler = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignal::SetCode(LPCTSTR Value) {
            if (m_code != Value)
                m_code = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignal::SetName(LPCTSTR Value) {
            if (m_name != Value)
                m_name = Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignal::SetHandler(CSignalHandler Value) {
            if (m_handler != Value)
                m_handler = Value;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSignals --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CSignals::AddSignal(int ASigno, LPCTSTR ACode, LPCTSTR AName, CSignalHandler AHandler) {
            auto Signal = new CSignal(this, ASigno);

            Signal->Code(ACode);
            Signal->Name(AName);
            Signal->Handler(AHandler);
        }
        //--------------------------------------------------------------------------------------------------------------

        CSignal *CSignals::Get(int Index) {
            if ((Index < 0) || (Index >= Count()))
                throw ExceptionFrm(SListIndexError, Index);

            return (CSignal *) GetItem(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignals::Put(int Index, CSignal *Signal) {
            if ((Index < 0) || (Index >= Count()))
                throw ExceptionFrm(SListIndexError, Index);
            SetItem(Index, Signal);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignals::InitSignals() {

            CSignal *Signal;
            struct sigaction sa = {};

            for (int I = 0; I < Count(); ++I) {
                ZeroMemory(&sa, sizeof(struct sigaction));

                Signal = Get(I);

                if (Signal->Handler()) {
                    sa.sa_sigaction = Signal->Handler();
                    sa.sa_flags = SA_SIGINFO;
                } else {
                    sa.sa_handler = SIG_IGN;
                }

                if (Signal->Signo() && !Signal->Name()) {
                    Signal->Name(strsignal(Signal->Signo()));
                }

                sigemptyset(&sa.sa_mask);
                if (sigaction(Signal->Signo(), &sa, nullptr) == -1) {
                    throw EOSError(errno, "sigaction(%s) failed", Signal->Code());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        sigset_t *CSignals::SigAddSet(sigset_t *set) {
            if (Assigned(set)) {
                sigemptyset(set);
                for (int I = 0; I < Count(); ++I) {
                    if (sigaddset(set, Get(I)->Signo()) == -1) {
                        throw EOSError(errno, _T("call sigaddset() failed"));
                    }
                }
            }

            return set;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignals::SigProcMask(int How, const sigset_t *set, sigset_t *oset) {
            if (Assigned(set)) {
                if (sigprocmask(How, set, oset) == -1) {
                    throw EOSError(errno, _T("call sigprocmask() failed"));
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        int CSignals::IndexOfSigno(int Signo) {

            for (int I = 0; I < Count(); ++I) {
                if (Get(I)->Signo() == Signo)
                    return I;
            }

            return -1;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSignalProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSignalProcess::CSignalProcess(CProcessType AType, CCustomProcess *AParent):
            CCustomProcess(AType, AParent), CSignals(), m_pSignalProcess(this) {

            sig_reap = 0;
            sig_sigio = 0;
            sig_sigalrm = 0;
            sig_terminate = 0;
            sig_quit = 0;
            sig_debug_quit = 0;
            sig_reconfigure = 0;
            sig_reopen = 0;
            sig_noaccept = 0;
            sig_change_binary = 0;

            sig_exiting = 0;
            sig_restart = 0;
            sig_noaccepting = 0;

            CreateSignals();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::SetSignalProcess(CSignalProcess *Value) {
            if (m_pSignalProcess != Value) {
                m_pSignalProcess = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::CreateSignals() {
            if (Type() == ptSignaller) {

                AddSignal(signal_value(SIG_RECONFIGURE_SIGNAL), "SIG" value(SIG_RECONFIGURE_SIGNAL), "reload", nullptr);
                AddSignal(signal_value(SIG_REOPEN_SIGNAL), "SIG" value(SIG_REOPEN_SIGNAL), "reopen", nullptr);
                AddSignal(signal_value(SIG_TERMINATE_SIGNAL), "SIG" value(SIG_TERMINATE_SIGNAL), "stop", nullptr);
                AddSignal(signal_value(SIG_SHUTDOWN_SIGNAL), "SIG" value(SIG_SHUTDOWN_SIGNAL), "quit", nullptr);

            } else {

                AddSignal(signal_value(SIG_RECONFIGURE_SIGNAL), "SIG" value(SIG_RECONFIGURE_SIGNAL),
                          "reload", signal_handler);

                AddSignal(signal_value(SIG_REOPEN_SIGNAL), "SIG" value(SIG_REOPEN_SIGNAL),
                          "reopen", signal_handler);

                AddSignal(signal_value(SIG_NOACCEPT_SIGNAL), "SIG" value(SIG_NOACCEPT_SIGNAL),
                          "", signal_handler);

                AddSignal(signal_value(SIG_TERMINATE_SIGNAL), "SIG" value(SIG_TERMINATE_SIGNAL),
                          "stop", signal_handler);

                AddSignal(signal_value(SIG_SHUTDOWN_SIGNAL), "SIG" value(SIG_SHUTDOWN_SIGNAL),
                          "quit", signal_handler);

                AddSignal(signal_value(SIG_CHANGEBIN_SIGNAL), "SIG" value(SIG_CHANGEBIN_SIGNAL),
                          "", signal_handler);

                AddSignal(SIGINT, "SIGINT", nullptr, signal_handler);

                AddSignal(SIGIO, "SIGIO", nullptr, signal_handler);

                AddSignal(SIGCHLD, "SIGCHLD", nullptr, signal_handler);

                AddSignal(SIGSYS, "SIGSYS, SIG_IGN", nullptr, nullptr);

                AddSignal(SIGPIPE, "SIGPIPE, SIG_IGN", nullptr, nullptr);
            }

            if (Type() >= ptWorker) {

                AddSignal(SIGFPE, "SIGFPE", nullptr, signal_error);

                AddSignal(SIGILL, "SIGILL", nullptr, signal_error);

                AddSignal(SIGSEGV, "SIGSEGV", nullptr, signal_error);

                AddSignal(SIGBUS, "SIGBUS", nullptr, signal_error);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::SignalHandler(int signo, siginfo_t *siginfo, void *ucontext) {

            LPCTSTR      action;
            LPCTSTR      sigcode;
            int_t        ignore;
            int          err;

            ignore = 0;

            err = errno;

            int I = IndexOfSigno(signo);
            if (I >= 0)
                sigcode = Signals(I)->Code();
            else
                sigcode = _T("signal code not found");

            action = _T("");

            log_debug3(LOG_DEBUG_CORE, Log(), 0, "signal handler %d (%s), process name: %s", signo, sigcode, ProcessName());

            switch (Type()) {
                case ptMaster:
                case ptSingle:
                    switch (signo) {

                        case signal_value(SIG_SHUTDOWN_SIGNAL):
                            sig_quit = 1;
                            action = _T(", shutting down");
                            break;

                        case signal_value(SIG_TERMINATE_SIGNAL):
                        case SIGINT:
                            sig_terminate = 1;
                            action = _T(", exiting");
                            break;

                        case signal_value(SIG_NOACCEPT_SIGNAL):
                            if (Daemonized()) {
                                sig_noaccept = 1;
                                action = _T(", stop accepting connections");
                            }
                            break;

                        case signal_value(SIG_RECONFIGURE_SIGNAL):
                            sig_reconfigure = 1;
                            action = _T(", reconfiguring");
                            break;

                        case signal_value(SIG_REOPEN_SIGNAL):
                            sig_reopen = 1;
                            action = _T(", reopening logs");
                            break;

                        case signal_value(SIG_CHANGEBIN_SIGNAL):
                            if (getppid() == ParentId() || NewBinary() > 0) {

                                /*
                                 * Ignore the signal in the new binary if its parent is
                                 * not changed, i.e. the old binary's process is still
                                 * running.  Or ignore the signal in the old binary's
                                 * process if the new binary's process is already running.
                                 */

                                action = _T(", ignoring");
                                ignore = 1;
                                break;
                            }

                            sig_change_binary = 1;
                            action = _T(", changing binary");
                            break;

                        case SIGALRM:
                            sig_sigalrm = 1;
                            break;

                        case SIGIO:
                            sig_sigio = 1;
                            break;

                        case SIGCHLD:
                            sig_reap = 1;
                            break;

                        default:
                            break;
                    }

                    break;

                case ptWorker:
                case ptHelper:
                    switch (signo) {

                        case signal_value(SIG_NOACCEPT_SIGNAL):
                            if (!Daemonized()) {
                                break;
                            }
                            sig_debug_quit = 1;
                            /* fall through */
                        case signal_value(SIG_SHUTDOWN_SIGNAL):
                            sig_quit = 1;
                            action = _T(", shutting down");
                            break;

                        case signal_value(SIG_TERMINATE_SIGNAL):
                        case SIGINT:
                            sig_terminate = 1;
                            action = _T(", exiting");
                            break;

                        case signal_value(SIG_REOPEN_SIGNAL):
                            sig_reopen = 1;
                            action = _T(", reopening logs");
                            break;

                        case signal_value(SIG_RECONFIGURE_SIGNAL):
                        case signal_value(SIG_CHANGEBIN_SIGNAL):
                        case SIGIO:
                            action = _T(", ignoring");
                            break;
                        default:
                            break;
                    }

                    break;

                default:
                    break;
            }

            if (siginfo && siginfo->si_pid) {
                Log()->Error(LOG_NOTICE, 0,
                             _T("signal %d (%s) received from %P%s"),
                             signo, sigcode, siginfo->si_pid, action);

            } else {
                Log()->Error(LOG_NOTICE, 0,
                             _T("signal %d (%s) received%s"),
                             signo, sigcode, action);
            }

            if (ignore) {
                Log()->Error(LOG_NOTICE, 0,
                             _T("the changing binary signal is ignored: ")
                             _T("you should shutdown or terminate ")
                             _T("before either old or new binary's process"));
            }

            if (signo == SIGCHLD) {
                log_debug0(LOG_DEBUG_CORE, Log(), 0, "child process get status");
                ChildProcessGetStatus();
            }

            errno = err;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSignalProcess::Quit() {
            sig_quit = 1;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CServerProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CServerProcess::CServerProcess(CProcessType AType, CCustomProcess *AParent): CSignalProcess(AType, AParent) {
            m_pServer = nullptr;
            m_pPQServer = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::SetServer(CHTTPServer *Value) {
            if (m_pServer != Value) {

                if (Value == nullptr) {
                    delete m_pServer;
                }

                m_pServer = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::SetPQServer(CPQServer *Value) {
            if (m_pPQServer != Value) {

                if (Value != nullptr && m_pServer == nullptr)
                    throw Delphi::Exception::Exception("Set, please, PQ Server after HTTP Server");

                if (Value == nullptr && m_pServer != nullptr)
                    throw Delphi::Exception::Exception("Unset, please, PQ Server after HTTP Server");

                if (Value == nullptr) {
                    delete m_pPQServer;
                }

                m_pPQServer = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::InitializeServerHandlers() {

            if (Assigned(m_pServer)) {

                CCommandHandler *LCommand;

                m_pServer->CommandHandlers()->ParseParamsDefault(false);

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("GET"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoGet, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("POST"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoPost, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("PUT"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoPut, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("DELETE"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoDelete, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("OPTIONS"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoOptions, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("HEAD"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoHead, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("PATCH"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoPatch, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("TRACE"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoTrace, this, _1));

                LCommand = m_pServer->CommandHandlers()->Add();
                LCommand->Command(_T("CONNECT"));
                LCommand->OnCommand(std::bind(&CServerProcess::DoConnect, this, _1));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQReceiver(CPQConnection *AConnection, const PGresult *AResult) {
            CPQConnInfo &Info = AConnection->ConnInfo();
            if (Info.ConnInfo().IsEmpty()) {
                Log()->Postgres(LOG_EMERG, _T("Receiver message: %s"), PQresultErrorMessage(AResult));
            } else {
                Log()->Postgres(LOG_EMERG, "Receiver message(%d): postgresql://%s@%s:%s/%s - %s", AConnection->Socket(), Info["user"].c_str(),
                                Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str(), PQresultErrorMessage(AResult));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQProcessor(CPQConnection *AConnection, LPCSTR AMessage) {
            CPQConnInfo &Info = AConnection->ConnInfo();
            if (Info.ConnInfo().IsEmpty()) {
                Log()->Postgres(LOG_EMERG, _T("Processor message: %s"), AMessage);
            } else {
                Log()->Postgres(LOG_EMERG, "Processor message(%d): postgresql://%s@%s:%s/%s - %s", AConnection->Socket(), Info["user"].c_str(),
                                Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str(), AMessage);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQConnectException(CPQConnection *AConnection, Delphi::Exception::Exception *AException) {
            Log()->Postgres(LOG_EMERG, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQServerException(CPQServer *AServer, Delphi::Exception::Exception *AException) {
            Log()->Postgres(LOG_EMERG, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQStatus(CPQConnection *AConnection) {
            Log()->Postgres(LOG_DEBUG, AConnection->StatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQPollingStatus(CPQConnection *AConnection) {
            Log()->Postgres(LOG_DEBUG, AConnection->StatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQSendQuery(CPQQuery *AQuery) {
            for (int I = 0; I < AQuery->SQL().Count(); ++I) {
                Log()->Postgres(LOG_INFO, AQuery->SQL()[I].c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResultStatus(CPQResult *AResult) {
            Log()->Postgres(LOG_DEBUG, AResult->StatusString());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQResult(CPQResult *AResult, ExecStatusType AExecStatus) {
/*
            if (AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_SINGLE_TUPLE) {
                CString Print;

                Print = "(";
                Print += AResult->fName(0);
                for (int I = 1; I < AResult->nFields(); ++I) {
                    Print += ",";
                    Print += AResult->fName(I);
                }
                Print += ")";

                Log()->Postgres(LOG_INFO, "%s", Print.c_str());

                Print = "(";
                for (int Row = 0; Row < AResult->nTuples(); ++Row) {

                    if (AResult->GetIsNull(Row, 0)) {
                        Print += "<null>";
                    } else {
                        if (AResult->fFormat(0) == 0) {
                            Print += AResult->GetValue(Row, 0);
                        } else {
                            Print += "<binary>";
                        }
                    }

                    for (int Col = 1; Col < AResult->nFields(); ++Col) {
                        Print += ",";
                        if (AResult->GetIsNull(Row, Col)) {
                            Print += "<null>";
                        } else {
                            if (AResult->fFormat(Col) == 0) {
                                Print += AResult->GetValue(Row, Col);
                            } else {
                                Print += "<binary>";
                            }
                        }
                    }
                }

                Print += ")";

                Log()->Postgres(LOG_INFO, "%s", Print.c_str());
            } else {
                Log()->Postgres(LOG_EMERG, AResult->GetErrorMessage());
            }
*/
            if (!(AExecStatus == PGRES_TUPLES_OK || AExecStatus == PGRES_SINGLE_TUPLE)) {
                Log()->Postgres(LOG_EMERG, AResult->GetErrorMessage());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQConnect(CPQConnection *AConnection) {
            CPQConnInfo &Info = AConnection->ConnInfo();
            if (!Info.ConnInfo().IsEmpty()) {
                Log()->Postgres(LOG_NOTICE, "Connect [%d]: postgresql://%s@%s:%s/%s", AConnection->PID(), Info["user"].c_str(),
                               Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPQDisconnect(CPQConnection *AConnection) {
            CPQConnInfo &Info = AConnection->ConnInfo();
            if (!Info.ConnInfo().IsEmpty()) {
                Log()->Postgres(LOG_NOTICE, "Disconnect [%d]: postgresql://%s@%s:%s/%s", AConnection->PID(), Info["user"].c_str(),
                               Info["host"].c_str(), Info["port"].c_str(), Info["dbname"].c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerListenException(CSocketServer *AServer, Delphi::Exception::Exception *AException) {
            Log()->Error(LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerException(CTCPServerConnection *AConnection,
                                             Delphi::Exception::Exception *AException) {
            Log()->Error(LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerEventHandlerException(CPollEventHandler *AHandler,
                Delphi::Exception::Exception *AException) {
            Log()->Error(LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerConnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPConnection *>(Sender);
            if (LConnection != nullptr) {
                Log()->Message(_T("client \"%s:%d\" open connection"), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoServerDisconnected(CObject *Sender) {
            auto LConnection = dynamic_cast<CHTTPConnection *>(Sender);

            if (LConnection != nullptr) {
                auto LPollQuery = PQServer()->FindQuryByConnection(LConnection);
                if (LPollQuery != nullptr) {
                    LPollQuery->PollConnection(nullptr);
                }

                Log()->Message(_T("client \"%s:%d\" closed connection"), LConnection->Socket()->Binding()->PeerIP(),
                               LConnection->Socket()->Binding()->PeerPort());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoDebug(CSocketServer *AServer, CTCPServerConnection *AConnection, LPCTSTR AFormat,
                                     va_list args) {
            Log()->Debug(0, AFormat, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoAccessLog(CTCPServerConnection *AConnection) {
            auto LConnection = dynamic_cast<CHTTPConnection *> (AConnection);
            auto LRequest = LConnection->Request();
            auto LReply = LConnection->Reply();

            LPCTSTR lpReferer;
            LPCTSTR lpUserAgent;
            //LPCTSTR lpContentLength;

            TCHAR szTime[PATH_MAX / 4] = {0};

            time_t wtime;
            struct tm *wtm;

            wtime = time(&wtime);
            wtm = localtime(&wtime);

            if ((wtm != nullptr) && (strftime(szTime, sizeof(szTime), "%d/%b/%Y:%T %z", wtm) != 0)) {

                const CString &Referer = LRequest->Headers.Values(_T("referer"));
                const CString &UserAgent = LRequest->Headers.Values(_T("user-agent"));
                //const CString &ContentLength = LReply->Headers.Values(_T("content-length"));

                if (!Referer.IsEmpty()) lpReferer = Referer.c_str(); else lpReferer = _T("-");
                if (!UserAgent.IsEmpty()) lpUserAgent = UserAgent.c_str(); else lpUserAgent = _T("-");
                //if (!ContentLength.IsEmpty()) lpContentLength = ContentLength.c_str(); else lpContentLength = _T("-");

                Log()->Access(_T("%s %s %s [%s] \"%s %s HTTP/%d.%d\" %d %d \"%s\" \"%s\"\r\n"), AConnection->Socket()->Binding()->PeerIP(),
                        _T("-"), _T("-"), szTime, LRequest->Method.c_str(), LRequest->Uri.c_str(), LRequest->VMajor,
                        LRequest->VMinor, LReply->Status, LReply->Content.Size(), lpReferer, lpUserAgent);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoNoCommandHandler(CSocketServer *AServer, LPCTSTR AData, CTCPServerConnection *AConnection) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (AConnection), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoOptions(CCommand *ACommand) {
            auto LConnection = dynamic_cast<CHTTPConnection *> (ACommand->Connection());
            auto LRequest = LConnection->Request();

            if (LRequest->Uri == _T("/quit"))
                SignalProcess()->Quit();

            CReply::SendStockReply(LConnection, CReply::ok);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoGet(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoHead(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPost(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPut(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoPatch(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoDelete(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoTrace(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CServerProcess::DoConnect(CCommand *ACommand) {
            CReply::SendStockReply(dynamic_cast<CHTTPConnection *> (ACommand->Connection()), CReply::not_implemented);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CModuleProcess --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CModuleProcess::CModuleProcess(CProcessType AType, CCustomProcess *AParent) : CServerProcess(AType, AParent) {
            m_pModules = new CStringList(true);
        }
        //--------------------------------------------------------------------------------------------------------------

        CModuleProcess::~CModuleProcess() {
            delete m_pModules;
        }
        //--------------------------------------------------------------------------------------------------------------

        CApostolModule *CModuleProcess::GetModule(CHTTPConnection *AConnection) const {

            CApostolModule *LModule = nullptr;

            auto LRequest = AConnection->Request();

            const CString &UserAgent = LRequest->Headers.Values(_T("user-agent"));

            if (!UserAgent.IsEmpty()) {
                int Index = m_pModules->IndexOf(UserAgent);
                if (Index != -1) {
                    LModule = (CApostolModule *) m_pModules->Objects(Index);
                }
            }

            if (LModule == nullptr) {
                LModule = CreateModule(UserAgent);
                if (Assigned(LModule)) {
                    if (!UserAgent.IsEmpty()) {
                        m_pModules->AddObject(UserAgent, LModule);
                        LModule->FreeAfterExecute(false);
                    }
                }
            }

            return LModule;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CModuleProcess::DoExecute(CTCPServerConnection *AConnection) {
            auto LConnection = dynamic_cast<CHTTPConnection *> (AConnection);
            auto LModule = GetModule(LConnection);

            if (LModule == nullptr) {
                CReply::SendStockReply(LConnection, CReply::forbidden);
                return;
            }

            try {
                LModule->Execute(LConnection);
            } catch (Delphi::Exception::Exception &E) {
                DoServerException(LConnection, &E);
                CReply::SendStockReply(LConnection, CReply::internal_server_error);
            }

            if (LModule->FreeAfterExecute()) {
                LModule->Free();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}

