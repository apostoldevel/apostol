/*++

Library name:

  apostol-core

Module Name:

  Application.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Application.hpp"

//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Application {

        CApplication *Application = nullptr;
        //--------------------------------------------------------------------------------------------------------------

        CCustomApplication::CCustomApplication(int argc, char *const *argv): CObject() {

            m_exitcode = 0;

            m_argc = argc;
            m_os_argv = (char **) argv;
            m_os_environ = environ;

            m_environ = nullptr;
            m_os_argv_last = nullptr;

            Initialize();
            SetEnviron();
        }
        //--------------------------------------------------------------------------------------------------------------

        CCustomApplication::~CCustomApplication() {
            //delete [] m_environ; //!!! don`t free m_environ
            m_environ = nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomApplication::Initialize() {

            for (int i = 0; i < m_argc; ++i) {
                m_argv.Add(m_os_argv[i]);
            }

            m_cmdline = m_argv[0];

            for (int i = 1; i < m_argc; ++i) {
                m_cmdline += " ";
                m_cmdline += m_argv[i];
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomApplication::SetEnviron() {

            int     i;
            char   *tmp;
            size_t  size = 0;

            for (i = 0; environ[i]; ++i) {
                size += strlen(environ[i]) + 1;
            }

            if (m_environ == nullptr)
                m_environ = new char[size];

            ZeroMemory(m_environ, size);
            tmp = m_environ;

            m_os_argv_last = m_os_argv[0];

            for (i = 0; m_os_argv[i]; ++i) {
                if (m_os_argv_last == m_os_argv[i]) {
                    m_os_argv_last = m_os_argv[i] + strlen(m_os_argv[i]) + 1;
                }
            }

            for (i = 0; environ[i]; ++i) {
                if (m_os_argv_last == environ[i]) {

                    size = strlen(environ[i]) + 1;
                    m_os_argv_last = environ[i] + size;

                    ld_cpystrn(tmp, environ[i], size);
                    environ[i] = tmp;
                    tmp += size;
                }
            }

            m_os_argv_last--;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CCustomApplication::SetHeader(LPCTSTR Value) {

            char        *p;

            m_os_argv[1] = nullptr;

            p = ld_cpystrn(m_os_argv[0], (char *) Value, m_os_argv_last - m_os_argv[0]);

            if (m_os_argv_last - p) {
                memset(p, '\0', m_os_argv_last - p);
            }

            m_header = Value;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessManager -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::Start(CApplicationProcess *AProcess) {
            if (Assigned(AProcess)) {
                DoBeforeStartProcess(AProcess);

                AProcess->BeforeRun();
                try {
                    AProcess->Run();
                } catch (std::exception& e) {
                    log_failure(e.what())
                } catch (...) {
                    log_failure("Unknown error...")
                }
                AProcess->AfterRun();

                DoAfterStartProcess(AProcess);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::Stop(int Index) {
            Process(Index)->Terminate();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessManager::StopAll() {
            for (int I = 0; I < Count(); I++)
                Stop(I);
        }
        //--------------------------------------------------------------------------------------------------------------

        CApplicationProcess *CProcessManager::FindProcessById(pid_t Pid) {
            CApplicationProcess *Item = nullptr;

            for (int I = 0; I < Count(); I++) {
                Item = Process(I);
                if (Item->Pid() == Pid)
                    return Item;
            }

            return nullptr;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CApplication ----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApplication::CApplication(int argc, char *const *argv): CProcessManager(),
            CApplicationProcess(nullptr, this, ptMain), CCustomApplication(argc, argv) {
            m_ProcessType = ptSingle;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::SetProcessType(CProcessType Value) {
            if (m_ProcessType != Value) {
                m_ProcessType = Value;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CreateLogFile() {
            CLogFile *LogFile;

            Log()->Level(LOG_STDERR);

            u_int Level;
            for (int I = 0; I < Config()->LogFiles().Count(); ++I) {

                const CString &Key = Config()->LogFiles().Names(I);
                const CString &Value = Config()->LogFiles().Values(Key);

                Level = GetLogLevelByName(Key.c_str());
                if (Level != 0) {
                    Log()->AddLogFile(Value.c_str(), Level);
                    if (Log()->Level() < Level)
                        Log()->Level(Level);
                }
            }

            const CString &Error = Config()->LogFiles().Values(_T("error"));
            if (Error.IsEmpty()) {
                Log()->AddLogFile(Config()->ErrorLog().c_str(), LOG_ERR);
            }

            LogFile = Log()->AddLogFile(Config()->AccessLog().c_str(), 0);
            LogFile->LogType(ltAccess);

#ifdef _DEBUG
            LogFile = Log()->AddLogFile(Config()->PostgresLog().c_str(), LOG_DEBUG);
#else
            LogFile = Log()->AddLogFile(Config()->PostgresLog().c_str(), LOG_NOTICE);
#endif
            LogFile->LogType(ltPostgres);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::Daemonize() {

            int  fd;

            switch (fork()) {
                case -1:
                    throw EOSError(errno, "fork() failed");

                case 0:
                    break;

                default:
                    exit(0);
            }

            if (setsid() == -1) {
                throw EOSError(errno, "setsid() failed");
            }

            Daemonized(true);

            umask(0);

            if (chdir("/") == -1) {
                throw EOSError(errno, "chdir(\"/\") failed");
            }

            fd = open("/dev/null", O_RDWR);
            if (fd == -1) {
                throw EOSError(errno, "open(\"/dev/null\") failed");
            }

            if (dup2(fd, STDIN_FILENO) == -1) {
                throw EOSError(errno, "dup2(STDIN) failed");
            }

            if (dup2(fd, STDOUT_FILENO) == -1) {
                throw EOSError(errno, "dup2(STDOUT) failed");
            }

#ifndef _DEBUG
            if (dup2(fd, STDERR_FILENO) == -1) {
                throw EOSError(errno, "dup2(STDERR) failed");
            }
#endif
            if (close(fd) == -1) {
                throw EOSError(errno, "close(\"/dev/null\") failed");
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CreateHTTPServer(CPollStack * APollStack) {
            CHTTPServer *LServer = nullptr;

            LServer = new CHTTPServer((ushort) Config()->Listen(), Config()->DocRoot().c_str());

            LServer->ServerName() = Title();

            LServer->PollStack(APollStack);

            LServer->OnExecute(std::bind(&CApplication::DoExecute, this, _1));

            LServer->OnDebug(std::bind(&CApplication::DoDebug, this, _1, _2, _3, _4));
            LServer->OnAccessLog(std::bind(&CApplication::DoAccessLog, this, _1));

            LServer->OnException(std::bind(&CApplication::DoServerException, this, _1, _2));
            LServer->OnListenException(std::bind(&CApplication::DoServerListenException, this, _1, _2));

            LServer->OnEventHandlerException(std::bind(&CApplication::DoServerEventHandlerException, this, _1, _2));

            LServer->OnConnected(std::bind(&CApplication::DoServerConnected, this, _1));
            LServer->OnDisconnected(std::bind(&CApplication::DoServerDisconnected, this, _1));

            LServer->OnNoCommandHandler(std::bind(&CApplication::DoNoCommandHandler, this, _1, _2, _3));

            LServer->ActiveLevel(alBinding);

            SetServer(LServer);

            InitializeServerHandlers();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CreatePQServer(CPollStack * APollStack) {
            CPQServer *LPQServer = nullptr;

            LPQServer = new CPQServer(Config()->PostgresPollMin(), Config()->PostgresPollMax());

            LPQServer->ConnInfo().ApplicationName() = "'" + Title() + "'"; //application_name;

            LPQServer->PollStack(APollStack);

            if (Config()->PostgresNotice()) {
                //LPQServer->OnReceiver(std::bind(&CApplication::DoPQReceiver, this, _1, _2));
                LPQServer->OnProcessor(std::bind(&CApplication::DoPQProcessor, this, _1, _2));
            }

            LPQServer->OnException(std::bind(&CApplication::DoPQConnectException, this, _1, _2));
            LPQServer->OnServerException(std::bind(&CApplication::DoPQServerException, this, _1, _2));

            LPQServer->OnEventHandlerException(std::bind(&CApplication::DoServerEventHandlerException, this, _1, _2));

            LPQServer->OnStatus(std::bind(&CApplication::DoPQStatus, this, _1));
            LPQServer->OnPollingStatus(std::bind(&CApplication::DoPQPollingStatus, this, _1));

            LPQServer->OnConnected(std::bind(&CApplication::DoPQConnect, this, _1));
            LPQServer->OnDisconnected(std::bind(&CApplication::DoPQDisconnect, this, _1));

            SetPQServer(LPQServer);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::MkDir(const CString &Dir) {
            if (!DirectoryExists(Dir.c_str()))
                if (!CreateDir(Dir.c_str(), 0700))
                    throw EOSError(errno, _T("mkdir \"%s\" failed "), Dir.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::CreateDirectories() {
            MkDir(Config()->Prefix());
            MkDir(Config()->Prefix() + _T("logs/"));
            MkDir(Config()->ConfPrefix());
            MkDir(Config()->CachePrefix());
            //MkDir(Config()->DocRoot());
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplication::ExecNewBinary(char *const *argv) {

            char **env, *var;
            char *p;
            uint_t i, n;

            pid_t pid;

            CExecuteContext ctx;

            ZeroMemory(&ctx, sizeof(CExecuteContext));

            ctx.path = argv[0];
            ctx.name = "new binary process";
            ctx.argv = argv;

            n = 2;
            env = (char **) Environ();

            var = new char[sizeof(APP_VAR) + Server()->Bindings()->Count() * (_INT32_LEN + 1) + 2];

            p = MemCopy(var, APP_VAR "=", sizeof(APP_VAR));

            for (i = 0; i < Server()->Bindings()->Count(); i++) {
                p = ld_sprintf(p, "%ud;", Server()->Bindings()->Handles(i)->Handle());
            }

            *p = '\0';

            env[n++] = var;

            env[n] = nullptr;

#if (_DEBUG)
            {
                char **e;
                for (e = env; *e; e++) {
                    log_debug1(LOG_DEBUG_CORE, Log(), 0, "env: %s", *e);
                }
            }
#endif

            ctx.envp = (char *const *) env;

            if (!RenamePidFile(false, "rename %s to %s failed "
                                      "before executing new binary process \"%s\"")) {
                return INVALID_PID;
            }

            pid = ExecProcess(&ctx);

            if (pid == INVALID_PID) {
                RenamePidFile(true, "rename() %s back to %s failed "
                                    "after an attempt to execute new binary process \"%s\"");
            }

            return pid;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::SetNewBinary(CApplicationProcess *AProcess) {
            AProcess->NewBinary(ExecNewBinary(m_os_argv));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::DoBeforeStartProcess(CApplicationProcess *AProcess) {

            AProcess->Pid(getpid());
            AProcess->Assign(this);

            if ((AProcess->Type() <= ptMaster) || (AProcess->Pid() != MainThreadID)) {
                MainThreadID = AProcess->Pid();
                SetSignalProcess(AProcess);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::DoAfterStartProcess(CApplicationProcess *AProcess) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::StartProcess() {
            CPollStack *LPollStack = nullptr;

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), CmdLine().c_str());

            if (Config()->Master() && m_ProcessType == ptSingle) {
                m_ProcessType = ptMaster;
            }

            if (m_ProcessType != ptSignaller) {
                LPollStack = new CPollStack();

                LPollStack->TimeOut(Config()->TimeOut());

                CreateHTTPServer(LPollStack);
                CreatePQServer(LPollStack);
            }

            if ( Config()->Daemon() ) {
                Daemonize();

                Log()->UseStdErr(false);
                Log()->RedirectStdErr();
            }

            Start(CApplicationProcess::Create(this, m_ProcessType));

            // Delete HTTPServer
            SetServer(nullptr);
            // Delete PQServer
            SetPQServer(nullptr);

            delete LPollStack;

            Log()->Debug(0, MSG_PROCESS_STOP, GetProcessName());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplication::Run() {

            ParseCmdLine();

            if (Config()->Flags().show_version) {
                ShowVersioInfo();
                ExitRun(0);
            }

            Config()->Reload();

            if (Config()->Flags().test_config) {

                if (!FileExists(Config()->ConfFile().c_str())) {
                    Log()->Error(LOG_EMERG, 0, "configuration file %s not found", Config()->ConfFile().c_str());
                    Log()->Error(LOG_EMERG, 0, "configuration file %s test failed", Config()->ConfFile().c_str());
                    ExitRun(1);
                }

                if (Config()->ErrorCount() == 0) {
                    Log()->Error(LOG_EMERG, 0, "configuration file %s test is successful", Config()->ConfFile().c_str());
                    ExitRun(0);
                }

                Log()->Error(LOG_EMERG, 0, "configuration file %s test failed", Config()->ConfFile().c_str());
                ExitRun(1);
            }

            Log()->Error(LOG_NOTICE, 0, "Config file: %s", Config()->ConfFile().c_str());

            CreateDirectories();

            //DefaultLocale.SetLocale(Config()->Locale().c_str());

            CreateLogFile();

            StartProcess();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CApplicationProcess ---------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApplicationProcess::CApplicationProcess(CCustomProcess *AParent, CApplication *AApplication,
                CProcessType AType): CModuleProcess(AType, AParent), CCollectionItem((CProcessManager *) AApplication),
                m_pApplication(AApplication) {
        }
        //--------------------------------------------------------------------------------------------------------------

        class CApplicationProcess *CApplicationProcess::Create(CApplication *AApplication, CProcessType AType) {

            switch (AType) {
                case ptSingle:
                    return new CProcessSingle(AApplication->SignalProcess(), AApplication);

                case ptMaster:
                    return new CProcessMaster(AApplication->SignalProcess(), AApplication);

                case ptSignaller:
                    return new CProcessSignaller(AApplication->SignalProcess(), AApplication);

                case ptNewBinary:
                    return new CProcessNewBinary(AApplication->SignalProcess(), AApplication);

                case ptWorker:
                    return new CProcessWorker(AApplication->SignalProcess(), AApplication);

                case ptHelper:
                    return new CProcessHelper(AApplication->SignalProcess(), AApplication);

                default:
                    throw ExceptionFrm(_T("Unknown process type: (%d)"), AType);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::Assign(CCustomProcess *AProcess) {
            CCustomProcess::Assign(AProcess);

            auto LProcess = dynamic_cast<CApplicationProcess *> (AProcess);

            SetServer(LProcess->Server());
            SetPQServer(LProcess->PQServer());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::BeforeRun() {
            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->CmdLine().c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::AfterRun() {
            Log()->Debug(0, MSG_PROCESS_STOP, GetProcessName());
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplicationProcess::SwapProcess(CProcessType Type, int Respawn, Pointer Data) {

            CApplicationProcess *LProcess;

            if (Respawn >= 0) {
                LProcess = Application()->Process(Respawn);
            } else {
                LProcess = CApplicationProcess::Create(Application(), Type);
                LProcess->Data(Data);
            }

            pid_t pid = fork();

            switch (pid) {

                case -1:
                    throw EOSError(errno, _T("fork() failed while spawning \"%s process\""), LProcess->GetProcessName());

                case 0:

                    Application()->Start(LProcess);
                    exit(0);

                default:
                    break;
            }

            LProcess->Pid(pid);

            LProcess->Exited(false);

            Log()->Error(LOG_NOTICE, 0, _T("start %s %P"), LProcess->GetProcessName(), LProcess->Pid());

            if (Respawn >= 0) {
                return pid;
            }

            LProcess->Exiting(false);

            LProcess->Respawn(Respawn == PROCESS_RESPAWN || Respawn == PROCESS_JUST_RESPAWN);
            LProcess->JustSpawn(Respawn == PROCESS_JUST_SPAWN || Respawn == PROCESS_JUST_RESPAWN);
            LProcess->Detached(Respawn == PROCESS_DETACHED);

            return pid;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::ChildProcessGetStatus() {
            int             status;
            const char     *process;
            pid_t           pid;
            int             err;
            int             i;
            bool            one;

            one = false;

            for ( ;; ) {
                pid = waitpid(-1, &status, WNOHANG);

                if (pid == 0) {
                    return;
                }

                if (pid == -1) {
                    err = errno;

                    if (err == EINTR) {
                        continue;
                    }

                    if (err == ECHILD && one) {
                        return;
                    }

                    /*
                     * Solaris always calls the signal handler for each exited process
                     * despite waitpid() may be already called for this process.
                     *
                     * When several processes exit at the same time FreeBSD may
                     * erroneously call the signal handler for exited process
                     * despite waitpid() may be already called for this process.
                     */

                    if (err == ECHILD) {
                        Log()->Error(LOG_INFO, err, "waitpid() failed");
                        return;
                    }

                    Log()->Error(LOG_ALERT, err, "waitpid() failed");
                    return;
                }

                one = true;
                process = "unknown process";

                CApplicationProcess *LProcess = this;
                for (i = 0; i < Application()->ProcessCount(); ++i) {
                    LProcess = Application()->Process(i);
                    if (LProcess->Pid() == pid) {
                        LProcess->Status(status);
                        LProcess->Exited(true);
                        process = LProcess->GetProcessName();
                        break;
                    }
                }

                if (WTERMSIG(status)) {
#ifdef WCOREDUMP
                    Log()->Error(LOG_ALERT, 0,
                                 "%s %P exited on signal %d%s",
                                 process, pid, WTERMSIG(status),
                                 WCOREDUMP(status) ? " (core dumped)" : "");
#else
                    Log()->Error(LOG_ALERT, 0,
                          "%s %P exited on signal %d",
                          process, pid, WTERMSIG(status));
#endif

                } else {
                    Log()->Error(LOG_NOTICE, 0,
                                 "%s %P exited with code %d",
                                 process, pid, WEXITSTATUS(status));
                }

                if (WEXITSTATUS(status) == 2 && LProcess->Respawn()) {
                    Log()->Error(LOG_ALERT, 0,
                                 "%s %P exited with fatal code %d "
                                 "and cannot be respawned",
                                 process, pid, WEXITSTATUS(status));
                    LProcess->Respawn(false);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        pid_t CApplicationProcess::ExecProcess(CExecuteContext *AContext)
        {
            return SwapProcess(ptNewBinary, PROCESS_DETACHED, AContext);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CApplicationProcess::RenamePidFile(bool Back, LPCTSTR lpszErrorMessage) {
            int Result;

            TCHAR szOldPid[PATH_MAX + 1] = {0};

            LPCTSTR lpszPid;
            LPCTSTR lpszOldPid;

            lpszPid = Config()->PidFile().c_str();
            Config()->PidFile().Copy(szOldPid, PATH_MAX);
            lpszOldPid = ChangeFileExt(szOldPid, Config()->PidFile().c_str(), APP_OLDPID_EXT);

            if (Back) {
                Result = ::rename(lpszOldPid, lpszPid);
                if (Result == INVALID_FILE)
                    Log()->Error(LOG_ALERT, errno, lpszErrorMessage, lpszOldPid, lpszPid, Application()->argv()[0].c_str());
            } else {
                Result = ::rename(lpszPid, lpszOldPid);
                if (Result == INVALID_FILE)
                    Log()->Error(LOG_ALERT, errno, lpszErrorMessage, lpszPid, lpszOldPid, Application()->argv()[0].c_str());
            }

            return (Result != INVALID_FILE);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::CreatePidFile() {

            size_t len;
            char pid[_INT64_LEN + 2];
            int create = Config()->Flags().test_config ? FILE_CREATE_OR_OPEN : FILE_TRUNCATE;

            if (Daemonized()) {

                CFile File(Config()->PidFile().c_str(), FILE_RDWR | create);

                File.setOnFilerError(std::bind(&CApplicationProcess::OnFilerError, this, _1, _2, _3, _4));

                File.Open();

                if (!Config()->Flags().test_config) {
                    len = ld_snprintf(pid, _INT64_LEN + 2, "%P%N", Pid()) - pid;
                    File.Write(pid, len, 0);
                }

                File.Close();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::DeletePidFile() {

            if (Daemonized()) {
                LPCTSTR lpszPid = Config()->PidFile().c_str();

                if (unlink(lpszPid) == FILE_ERROR) {
                    Log()->Error(LOG_ALERT, errno, _T("could not delete pid file: \"%s\" error: "), lpszPid);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::ServerStart() {
            Server()->DocRoot() = Config()->DocRoot();
            Server()->ActiveLevel(alActive);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::ServerStop() {
            Server()->ActiveLevel(alBinding);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::ServerShutDown() {
            Server()->ActiveLevel(alShutDown);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::PQServerStart() {
            if (Config()->PostgresConnect()) {
                PQServer()->ConnInfo().SetParameters(Config()->PostgresConnInfo());
                PQServer()->Active(true);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::PQServerStop() {
            PQServer()->Active(false);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CApplicationProcess::OnFilerError(Pointer Sender, int Error, LPCTSTR lpFormat, va_list args) {
            Log()->Error(LOG_ALERT, Error, lpFormat, args);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessSingle --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::BeforeRun() {
            Application()->Header(Application()->Name() + ": single process " + Application()->CmdLine());

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            ServerStart();
            PQServerStart();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::AfterRun() {
            ServerStop();
            PQServerStop();
            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::DoExit() {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::Reload() {
            ServerStop();
            PQServerStop();

            Config()->Reload();

            ServerStart();
            PQServerStart();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSingle::Run() {

            while (!(sig_terminate || sig_quit)) {

                log_debug0(LOG_DEBUG_EVENT, Log(), 0, "single cycle");

                try
                {
                    Server()->Wait();
                }
                catch (std::exception& e)
                {
                    Log()->Error(LOG_EMERG, 0, e.what());
                }

                if (sig_reconfigure) {
                    sig_reconfigure = 0;
                    Log()->Error(LOG_NOTICE, 0, "reconfiguring");

                    Reload();
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Error(LOG_NOTICE, 0, "reopening logs");
                    Log()->RedirectStdErr();
                }
            }

            Log()->Error(LOG_NOTICE, 0, "exiting single process");
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessMaster --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::BeforeRun() {
            Application()->Header(Application()->Name() + ": master process " + Application()->CmdLine());

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::AfterRun() {
            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::StartWorkerProcesses(int Type) {
            for (int I = 0; I < Config()->Workers(); ++I ) {
                SwapProcess(ptWorker, Type);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::DoExit() {
            DeletePidFile();

            Log()->Error(LOG_NOTICE, 0, _T("exiting master process"));
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CProcessMaster::ReapChildren() {

            bool live = false;
            CApplicationProcess *LProcess;

            for (int i = 0; i < Application()->ProcessCount(); ++i) {

                LProcess = Application()->Process(i);

                if (LProcess->Type() == ptMain)
                    continue;

                log_debug7(LOG_DEBUG_EVENT, Log(), 0,
                           "reap child: %i %P e:%d t:%d d:%d r:%d j:%d",
                           i,
                           LProcess->Pid(),
                           LProcess->Exiting() ? 1 : 0,
                           LProcess->Exited() ? 1 : 0,
                           LProcess->Detached() ? 1 : 0,
                           LProcess->Respawn() ? 1 : 0,
                           LProcess->JustSpawn() ? 1 : 0);

                if (LProcess->Exited()) {

                    if (LProcess->Respawn() && !LProcess->Exiting() && !(sig_terminate || sig_quit)) {
                        if (SwapProcess(ptWorker, i) == -1) {
                            Log()->Error(LOG_ALERT, 0, "could not respawn %s", LProcess->GetProcessName());
                            continue;
                        }

                        live = true;

                        continue;
                    }

                    if (LProcess->Pid() == NewBinary()) {

                        RenamePidFile(true, "rename() %s back to %s failed after the new binary process \"%s\" exited");

                        NewBinary(0);

                        if (sig_noaccepting) {
                            sig_restart = 1;
                            sig_noaccepting = 0;
                        }
                    }

                    Application()->DeleteProcess(i);

                } else if (LProcess->Exiting() || !LProcess->Detached()) {
                    live = true;
                }

            }

            return live;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::Run() {

            int sigio;
            sigset_t set, wset;
            struct itimerval itv = {};
            bool live;
            uint_t delay;

            CreatePidFile();

            SigProcMask(SIG_BLOCK, SigAddSet(&set), &wset);

            StartWorkerProcesses(PROCESS_RESPAWN);

            NewBinary(0);

            delay = 0;
            sigio = 0;

            CApplicationProcess *LProcess;
            for (int i = 0; i < Application()->ProcessCount(); ++i) {
                LProcess = Application()->Process(i);

                log_debug9(LOG_DEBUG_EVENT, Log(), 0,
                              "process (%s)\t: %P - %i %P e:%d t:%d d:%d r:%d j:%d",
                               LProcess->GetProcessName(),
                               Pid(),
                               i,
                               LProcess->Pid(),
                               LProcess->Exiting() ? 1 : 0,
                               LProcess->Exited() ? 1 : 0,
                               LProcess->Detached() ? 1 : 0,
                               LProcess->Respawn() ? 1 : 0,
                               LProcess->JustSpawn() ? 1 : 0);
            }

            live = true;
            while (!(live && (sig_terminate || sig_quit))) {

                if (delay) {
                    if (sig_sigalrm) {
                        sigio = 0;
                        delay *= 2;
                        sig_sigalrm = 0;
                    }

                    log_debug1(LOG_DEBUG_EVENT, Log(), 0, "termination cycle: %M", delay);

                    itv.it_interval.tv_sec = 0;
                    itv.it_interval.tv_usec = 0;
                    itv.it_value.tv_sec = delay / 1000;
                    itv.it_value.tv_usec = (delay % 1000 ) * 1000;

                    if (setitimer(ITIMER_REAL, &itv, nullptr) == -1) {
                        Log()->Error(LOG_ALERT, errno, "setitimer() failed");
                    }
                }

                log_debug0(LOG_DEBUG_EVENT, Log(), 0, "sigsuspend");

                sigsuspend(&wset);

                log_debug1(LOG_DEBUG_EVENT, Log(), 0, "wake up, sigio %i", sigio);

                if (sig_reap) {
                    sig_reap = 0;
                    log_debug0(LOG_DEBUG_EVENT, Log(), 0, "reap children");

                    live = ReapChildren();
                }

                if (sig_terminate) {
                    if (delay == 0) {
                        delay = 50;
                    }

                    if (sigio) {
                        sigio--;
                        continue;
                    }

                    sigio = Config()->Workers();

                    if (delay > 1000) {
                        SignalToProcesses(ptWorker, SIGKILL);
                    } else {
                        SignalToProcesses(ptWorker, signal_value(SIG_TERMINATE_SIGNAL));
                    }

                    continue;
                }

                if (sig_quit) {
                    SignalToProcesses(ptWorker, signal_value(SIG_SHUTDOWN_SIGNAL));
                    continue;
                }

                if (sig_reconfigure) {
                    sig_reconfigure = 0;

                    if (NewBinary() > 0) {
                        StartWorkerProcesses(PROCESS_RESPAWN);
                        sig_noaccepting = 0;

                        continue;
                    }

                    Log()->Error(LOG_NOTICE, 0, "reconfiguring");

                    StartWorkerProcesses(PROCESS_JUST_RESPAWN);

                    /* allow new processes to start */
                    usleep(100 * 1000);

                    live = true;
                    SignalToProcesses(ptWorker, signal_value(SIG_SHUTDOWN_SIGNAL));
                }

                if (sig_restart) {
                    sig_restart = 0;
                    StartWorkerProcesses(PROCESS_RESPAWN);
                    live = true;
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Error(LOG_NOTICE, 0, "reopening logs");
                    // TODO: reopen files;
                    SignalToProcesses(ptWorker, signal_value(SIG_REOPEN_SIGNAL));
                }

                if (sig_change_binary) {
                    sig_change_binary = 0;
                    Log()->Error(LOG_NOTICE, 0, "changing binary");
                    Application()->SetNewBinary(this);
                }

                if (sig_noaccept) {
                    sig_noaccept = 0;
                    sig_noaccepting = 1;
                    SignalToProcesses(ptWorker, signal_value(SIG_SHUTDOWN_SIGNAL));
                }
            }

            DoExit();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessMaster::SignalToProcesses(CProcessType Type, int Signo) {
            int err;
            CCustomProcess *LProcess;

            for (int i = 0; i < Application()->ProcessCount(); ++i) {
                LProcess = Application()->Process(i);

                if (LProcess->Type() != Type)
                    continue;

                log_debug7(LOG_DEBUG_EVENT, Log(), 0,
                           "signal child: %i %P e:%d t:%d d:%d r:%d j:%d",
                           i,
                           LProcess->Pid(),
                           LProcess->Exiting() ? 1 : 0,
                           LProcess->Exited() ? 1 : 0,
                           LProcess->Detached() ? 1 : 0,
                           LProcess->Respawn() ? 1 : 0,
                           LProcess->JustSpawn() ? 1 : 0);

                if (LProcess->Detached()) {
                    continue;
                }

                if (LProcess->JustSpawn()) {
                    LProcess->JustSpawn(false);
                    continue;
                }

                if (LProcess->Exiting() && (Signo == signal_value(SIG_SHUTDOWN_SIGNAL))) {
                    continue;
                }

                log_debug2(LOG_DEBUG_CORE, Log(), 0, "kill (%P, %d)", LProcess->Pid(), Signo);

                if (kill(LProcess->Pid(), Signo) == -1) {
                    err = errno;
                    Log()->Error(LOG_ALERT, err, "kill(%P, %d) failed", LProcess->Pid(), Signo);

                    if (err == ESRCH) {
                        LProcess->Exited(true);
                        LProcess->Exiting(false);
                        sig_reap = 1;
                    }

                    continue;
                }

                if (Signo != signal_value(SIG_REOPEN_SIGNAL)) {
                    LProcess->Exiting(true);
                }
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessSignaller -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessSignaller::SignalToProcess(pid_t pid) {

            CSignal *Signal;
            LPCTSTR lpszSignal = Config()->Signal().c_str();

            for (int I = 0; I < SignalsCount(); ++I) {
                Signal = Signals(I);
                if (SameText(lpszSignal, Signal->Name())) {
                    if (kill(pid, Signal->Signo()) == -1) {
                        Log()->Error(LOG_ALERT, errno, _T("kill(%P, %d) failed"), pid, Signal->Signo());
                    }
                    break;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessSignaller::Run() {

            ssize_t       n;
            pid_t         pid;
            char          buf[_INT64_LEN + 2] = {0};

            LPCTSTR lpszPid = Config()->PidFile().c_str();

            CFile File(lpszPid, FILE_RDONLY | FILE_OPEN);

            File.setOnFilerError(std::bind(&CApplicationProcess::OnFilerError, this, _1, _2, _3, _4));

            File.Open();

            n = File.Read(buf, _INT64_LEN + 2, 0);

            File.Close();

            while (n-- && (buf[n] == CR || buf[n] == LF)) { buf[n] = '\0'; }

            pid = (pid_t) strtol(buf, nullptr, 0);

            if (pid == (pid_t) 0) {
                throw ExceptionFrm(_T("invalid PID number \"%*s\" in \"%s\""), n, buf, lpszPid);
            }

            SignalToProcess(pid);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessNewBinary -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessNewBinary::Run() {
            auto LContext = (CExecuteContext *) Data();
            ExecuteProcess(LContext);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessWorker --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::BeforeRun() {
            sigset_t set;

            Application()->Header(Application()->Name() + ": worker process");

            Log()->Debug(0, MSG_PROCESS_START, GetProcessName(), Application()->Header().c_str());

            InitSignals();

            Config()->Reload();

            ServerStart();
            PQServerStart();

            SigProcMask(SIG_UNBLOCK, SigAddSet(&set));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::AfterRun() {
            ServerStop();
            PQServerStop();
            CApplicationProcess::AfterRun();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::DoExit() {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CProcessWorker::Run() {

            while (!sig_exiting) {

                log_debug0(LOG_DEBUG_EVENT, Log(), 0, "worker cycle");

                try
                {
                    Server()->Wait();
                }
                catch (std::exception& e)
                {
                    Log()->Error(LOG_EMERG, 0, e.what());
                }

                if (sig_terminate || sig_quit) {
                    if (sig_quit) {
                        sig_quit = 0;
                        Log()->Error(LOG_NOTICE, 0, "gracefully shutting down");
                        Application()->Header("worker process is shutting down");
                    }

                    Log()->Error(LOG_NOTICE, 0, "exiting worker process");

                    if (!sig_exiting) {
                        sig_exiting = 1;
                    }
                }

                if (sig_reopen) {
                    sig_reopen = 0;
                    Log()->Error(LOG_NOTICE, 0, "reopening logs");
                    //ReopenFiles(-1);
                }
            }

            Log()->Error(LOG_NOTICE, 0, "stop worker process");
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CProcessHelper --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        void CProcessHelper::Run() {

        }

    }
}

}
