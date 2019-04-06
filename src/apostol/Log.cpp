/*++

Programm name:

  Apostol

Module Name:

  Log.cpp

Notices:

  Apostol Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Apostol.hpp"
#include "Log.hpp"

//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Log {

        static unsigned long GLogCount = 0;
        //--------------------------------------------------------------------------------------------------------------

        CLog *GLog = nullptr;
        //--------------------------------------------------------------------------------------------------------------

        inline void AddLog() {
            if (GLogCount == 0) {
                GLog = GLog->CreateLog();
            }

            GLogCount++;
        };
        //--------------------------------------------------------------------------------------------------------------

        inline void RemoveLog() {
            GLogCount--;

            if (GLogCount == 0) {
                GLog->DestroyLog();
                GLog = nullptr;
            }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CFile -----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CFile::CFile(LPCSTR AFileName, int AFlags): CObject() {

            m_hHandle = INVALID_FILE;
            m_iFlags = AFlags;
            m_uOffset = 0;

            m_lpFileName = nullptr;
            m_OnFilerError = nullptr;

            SetFileName(AFileName);
        };
        //--------------------------------------------------------------------------------------------------------------

        CFile::~CFile() {
            Close(true);
            SetFileName(nullptr);
        };
        //--------------------------------------------------------------------------------------------------------------

        void CFile::SetFileName(LPCSTR AValue) {

            if (m_lpFileName != nullptr) {
                ::free(m_lpFileName);
                m_lpFileName = nullptr;
            }

            if (AValue != nullptr) {

                size_t len = strlen(AValue);

                m_lpFileName = (char *) ::malloc(len + 1);
                m_lpFileName = strcpy(m_lpFileName, AValue);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CFile::Open() {

            if (m_lpFileName == nullptr) {
                throw EFilerError(_T("Error open file: file name cannot be empty"));
            }

            Close();

            if (strncmp(m_lpFileName, "stderr", 7) == 0) {
                m_hHandle = STDERR_FILENO;
            } else {
                m_hHandle = ::open(m_lpFileName, m_iFlags, FILE_DEFAULT_ACCESS);

                if (m_hHandle == INVALID_FILE) {
                    throw EFilerError(errno, _T("Could not open file: \"%s\" error: "), m_lpFileName);
                }
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        void CFile::Close(bool ASafe) {
            if ((m_hHandle != INVALID_FILE) && (m_hHandle != STDERR_FILENO)) {
                if (::close(m_hHandle) == FILE_ERROR) {
                    if (ASafe) {
                        DoFilerError(errno, FILE_CLOSE_MESSAGE, m_lpFileName);
                    } else {
                        throw EFilerError(errno, FILE_CLOSE_MESSAGE, m_lpFileName);
                    }
                } else {
                    m_hHandle = INVALID_FILE;
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        ssize_t CFile::Read(char *buf, size_t size, size_t offset)
        {
            ssize_t  n;

            n = pread(m_hHandle, buf, size, offset);

            if (n == -1) {
                throw EReadError(errno, _T("pread() \"%s\" failed: "), m_lpFileName);
            }

            m_uOffset += n;

            return n;
        }
        //--------------------------------------------------------------------------------------------------------------

        ssize_t CFile::Write(char *buf, size_t size, size_t offset)
        {
            ssize_t n;
            size_t  written;
            int     err;

            written = 0;

            for ( ;; ) {
                n = pwrite(m_hHandle, buf + written, size, offset);

                if (n == -1) {
                    err = errno;

                    if (err == EINTR) {
                        continue;
                    }

                    throw EWriteError(errno, _T("pwrite() \"%s\" failed"), m_lpFileName);
                }

                m_uOffset += n;
                written += n;

                if ((size_t) n == size) {
                    return written;
                }

                offset += n;
                size -= n;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CFile::DoFilerError(int AError, LPCTSTR lpFormat, ...) {
            if (m_OnFilerError) {
                va_list args;
                va_start(args, lpFormat);
                m_OnFilerError((Pointer) this, AError, lpFormat, args);
                va_end(args);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CLogComponent ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CLogComponent::CLogComponent() {
            AddLog();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogComponent::~CLogComponent() {
            RemoveLog();
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CLog ------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CLog::CLog(): CSysErrorComponent(), CCollection(this) {
#ifdef _DEBUG
            m_uLevel = LOG_DEBUG_ALL;
#else
            m_uLevel = LOG_NOTICE;
#endif
            m_CurrentIndex = -1;
            m_fUseStdErr = true;
        }
        //--------------------------------------------------------------------------------------------------------------

        CLog::CLog(LPCSTR  AFileName, u_int ALevel): CLog() {
            AddLogFile(AFileName, ALevel);
        };
        //--------------------------------------------------------------------------------------------------------------

        char *CLog::StrError(int AError, char *ADest, size_t ASize)
        {
            const CString &Msg = GSysError->StrError(AError);
            ASize = Min(ASize, Msg.Size());

            return MemCopy(ADest, Msg.c_str(), ASize);
        }
        //--------------------------------------------------------------------------------------------------------------

        char *CLog::ErrNo(char *ADest, char *ALast, int AError)
        {
            if (ADest > ALast - 50) {

                /* leave a space for an error code */

                ADest = ALast - 50;
                *ADest++ = '.';
                *ADest++ = '.';
                *ADest++ = '.';
            }

            ADest = ld_slprintf(ADest, ALast, " (%d: ", AError);
            ADest = StrError(AError, ADest, ALast - ADest);

            if (ADest < ALast) {
                *ADest++ = ')';
            }

            return ADest;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::ErrorCore(u_int ALevel, int AError, LPCSTR AFormat, CLogType ALogType, va_list args)
        {
            char        *p, *last, *msg;
            ssize_t      n;
            uint_t       wrote_stderr;
            bool         debug_connection;
            char         errstr[LOG_MAX_ERROR_STR] = {0};

            char         timestr[LOG_MAX_ERROR_STR] = {0};
            time_t       itime;
            struct tm   *timeinfo;

            last = errstr + LOG_MAX_ERROR_STR;

            itime = time(&itime);
            timeinfo = localtime(&itime);

            strftime(timestr, LOG_MAX_ERROR_STR, "%Y/%m/%d %H:%M:%S", timeinfo);

            p = MemCopy(errstr, timestr, strlen(timestr));
            p = ld_slprintf(p, last, " [%V] ", &err_levels[ALevel]);

            /* pid#tid */
            p = ld_slprintf(p, last, "%P#" LOG_TID_T_FMT ": ", log_pid, log_tid);

            msg = p;

            p = ld_vslprintf(p, last, AFormat, args);

            if (AError) {
                p = ErrNo(p, last, AError);
            }

            if (p > last - LINEFEED_SIZE) {
                p = last - LINEFEED_SIZE;
            }

            linefeed(p);

            wrote_stderr = 0;
            debug_connection = (Level() & LOG_DEBUG_CONNECTION) != 0;

            CLogFile *logfile = First();

            while (logfile) {

                if (logfile->Level() < ALevel || logfile->LogType() != ALogType) {
                    goto next;
                }

                if (Level() < ALevel && !debug_connection) {
                    break;
                }

                if (itime == m_DiskFullTime) {

                    /*
                     * on FreeBSD writing to a full filesystem with enabled softupdates
                     * may block process for much longer time than writing to non-full
                     * filesystem, so we skip writing to a log for one second
                     */

                    goto next;
                }

                n = write_fd(logfile->Handle(), errstr, p - errstr);

                if (n == -1 && errno == ENOSPC) {
                    DiskFullTime(itime);
                }

                if (logfile->Handle() == STDERR_FILENO) {
                    wrote_stderr = 1;
                }

                next:

                logfile = Next();
            }

            if (!UseStdErr() || ALevel > LOG_WARN || wrote_stderr)
            {
                return;
            }

            msg -= (9 + err_levels[ALevel].len + 3);

            (void) ld_sprintf(msg, "apostol: [%V] ", &err_levels[ALevel]);

            (void) write_console(STDERR_FILENO, msg, p - msg);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Message(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            ErrorCore(LOG_NOTICE, 0, AFormat, ltError, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Message(LPCSTR AFormat, va_list args) {
            //va_list list;
            //va_copy(list, args);
            ErrorCore(LOG_NOTICE, 0, AFormat, ltError, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Error(u_int ALevel, int AErrNo, LPCSTR AFormat, ...) {
            va_list args;
            if (m_uLevel >= ALevel) {
                va_start(args, AFormat);
                ErrorCore(ALevel, AErrNo, AFormat, ltError, args);
                va_end(args);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Error(u_int ALevel, int AErrNo, LPCSTR AFormat, va_list args) {
            if (m_uLevel >= ALevel) {
                //va_list list;
                //va_copy(list, args);
                ErrorCore(ALevel, AErrNo, AFormat, ltError, args);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Debug(int AErrNo, LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            ErrorCore(LOG_DEBUG, AErrNo, AFormat, ltError, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Debug(int AErrNo, LPCSTR AFormat, va_list args) {
            //va_list list;
            //va_copy(list, args);
            ErrorCore(LOG_DEBUG, AErrNo, AFormat, ltError, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Access(LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Access(AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Access(LPCSTR AFormat, va_list args) {
            TCHAR szBuffer[MAX_ERROR_STR + sizeof(TCHAR) + 1] = {0};
            size_t LSize = MAX_ERROR_STR;
            CLogFile *logfile = First();
            while (logfile) {
                if (logfile->LogType() == ltAccess) {
                    chVERIFY(SUCCEEDED(StringPCchVPrintf(szBuffer, &LSize, AFormat, args)));
                    write_fd(logfile->Handle(), szBuffer, LSize);
                }
                logfile = Next();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Postgres(u_int ALevel, LPCSTR AFormat, ...) {
            va_list args;
            va_start(args, AFormat);
            Postgres(ALevel, AFormat, args);
            va_end(args);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::Postgres(u_int ALevel, LPCSTR AFormat, va_list args) {
            ErrorCore(ALevel, 0, AFormat, ltPostgres, args);
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::AddLogFile(LPCSTR AFileName, u_int ALevel) {
            CLogFile *LogFile = nullptr;

            m_CurrentIndex = 0;
            while (m_CurrentIndex < Count() && (LogFile == nullptr)) {
                if (SameText(Current()->FileName(), AFileName))
                    LogFile = Current();
                m_CurrentIndex++;
            }

            if (LogFile == nullptr) {
                LogFile = new CLogFile(this, AFileName);
                LogFile->Level(ALevel);
                LogFile->Open();
            }

            if (LogFile->Level() < ALevel)
                LogFile->Level(ALevel);

            return LogFile;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::CheckCurrentIndex() {
            if (m_CurrentIndex < -1 ) {
                m_CurrentIndex = -1;
            }
            if (m_CurrentIndex > Count() ) {
                m_CurrentIndex = Count();
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::SetCurrentIndex(int AIndex) {
            m_CurrentIndex = AIndex;
            CheckCurrentIndex();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::First() {
            SetCurrentIndex(0);
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Last() {
            SetCurrentIndex(Count() - 1);
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Prior() {
            m_CurrentIndex--;
            CheckCurrentIndex();
            if (m_CurrentIndex == -1)
                return nullptr;
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Next() {
            m_CurrentIndex++;
            CheckCurrentIndex();
            if (m_CurrentIndex == Count())
                return nullptr;
            return Current();
        }
        //--------------------------------------------------------------------------------------------------------------

        CLogFile *CLog::Current() {
            if ((m_CurrentIndex == -1) || (m_CurrentIndex == Count()))
                return nullptr;
            return (CLogFile *) GetItem(m_CurrentIndex);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::SetLevel(u_int AValue) {
            if (AValue == LOG_DEBUG)
                m_uLevel = LOG_DEBUG_ALL;
            else
                m_uLevel = AValue;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CLog::RedirectStdErr() {

            CLogFile *log = First();
            while (log && log->Handle() == STDERR_FILENO) {
                log = Next();
            }

            if (Assigned(log)) {
                if (dup2(log->Handle(), STDERR_FILENO) == -1) {
                    throw EOSError(errno, "dup2(STDERR) failed");
                }
            }
        }

    }
}
}
