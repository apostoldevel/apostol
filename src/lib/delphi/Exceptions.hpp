/*++

Library name:

  libdelphi

Module Name:

  Exceptions.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_EXCEPTION_HPP
#define DELPHI_EXCEPTION_HPP

extern "C++" {

namespace Delphi {

    namespace Exception {

        class LIB_DELPHI Exception : public std::exception, public CSysErrorComponent {
            typedef CSysErrorComponent inherited;

        private:

            CString m_Message;
            int m_SysError;

        public:

            Exception(): inherited(), m_SysError(0) {
                Clear();
            };

            Exception(const Exception& Value) noexcept : Exception() {
                if (this != &Value) {
                    m_SysError = Value.m_SysError;
                    m_Message = Value.m_Message;
                };
            };

            explicit Exception(LPCTSTR lpMsg): inherited(), m_SysError(0) {
                Clear();

                if (lpMsg != nullptr) {
                    m_Message += lpMsg;
                }
            };

            explicit Exception(int SysError): inherited(), m_SysError(0) {
                Clear();
                SystemError(SysError);
            };

            explicit Exception(LPCTSTR lpMsg, int SysError): inherited(), m_SysError(0) {
                Clear();

                if (lpMsg != nullptr) {
                    m_Message += lpMsg;
                }

                SystemError(SysError);
            };

            ~Exception() override = default;

            void FormatMessage(LPCTSTR lpFormat, va_list argList) {
                Clear();

                if (lpFormat != nullptr) {
                    m_Message.Format(lpFormat, argList);
                }
            };

            void Clear() {
                m_SysError = 0;
                m_Message.Clear();
            };

            void SystemError(int SysError) {

                const CString &S = GSysError->StrError(SysError);

                m_SysError = SysError;

                if (m_Message.IsEmpty()) {
                    m_Message.Format(_T("%d: %s"), m_SysError, S.c_str());
                } else {
                    m_Message.Format(_T("(%d: %s)"), m_SysError, S.c_str());
                }
            };

            CString& Message() noexcept { return m_Message; };
            const CString& Message() const noexcept { return m_Message; };

            int ErrorCode() { return m_SysError; };

            LPCSTR what() const noexcept override { return m_Message.c_str(); };
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI ExceptionFrm : public Exception {
            typedef Exception inherited;

        public:

            ExceptionFrm() : inherited() {};

            explicit ExceptionFrm(LPCTSTR lpFormat, ...) : inherited() {
                va_list argList;
                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);
            };

            ~ExceptionFrm() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EExternal : public Exception {
            typedef Exception inherited;

        public:

            EExternal() : inherited() {};

            explicit EExternal(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EExternal(int SysError) : inherited(SysError) {};

            EExternal(LPCTSTR lpMsg, int SysError) : inherited(lpMsg, SysError) {};

            ~EExternal() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EExternalException : public EExternal {
            typedef EExternal inherited;

        public:

            EExternalException() : inherited() {};

            explicit EExternalException(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EExternalException(int SysError) : inherited(SysError) {};

            EExternalException(LPCTSTR lpMsg, int SysError) : inherited(lpMsg, SysError) {};

            ~EExternalException() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EIntError : public EExternal {
            typedef EExternal inherited;

        public:

            EIntError() : inherited() {};

            explicit EIntError(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EIntError(int SysError) : inherited(SysError) {};

            EIntError(LPCTSTR lpMsg, int SysError) : inherited(lpMsg, SysError) {};

            ~EIntError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EDivByZero : public EIntError {
            typedef EIntError inherited;

        public:

            EDivByZero() : inherited() {};

            explicit EDivByZero(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EDivByZero(int SysError) : inherited(SysError) {};

            EDivByZero(LPCTSTR lpMsg, int SysError) : inherited(lpMsg, SysError) {};

            ~EDivByZero() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI ERangeError : public EIntError {
            typedef EIntError inherited;

        public:

            ERangeError() : inherited() {};

            explicit ERangeError(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit ERangeError(int SysError) : inherited(SysError) {};

            ERangeError(LPCTSTR lpMsg, int SysError) : inherited(lpMsg, SysError) {};

            ~ERangeError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EStreamError : public Exception {
            typedef Exception inherited;

        public:

            EStreamError() : inherited() {};

            explicit EStreamError(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EStreamError(int SysError) : inherited(SysError) {};

            EStreamError(int SysError, LPCTSTR lpFormat, ...) : inherited() {
                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);

                SystemError(SysError);
            };

            ~EStreamError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EFilerError : public EStreamError {
            typedef EStreamError inherited;

        public:

            EFilerError() : inherited() {};

            explicit EFilerError(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EFilerError(int SysError) : inherited(SysError) {};

            EFilerError(int SysError, LPCTSTR lpFormat, ...) : inherited() {
                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);

                SystemError(SysError);
            };

            ~EFilerError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EReadError : public EFilerError {
            typedef EFilerError inherited;

        public:

            EReadError() : inherited() {};

            explicit EReadError(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EReadError(int SysError) : inherited(SysError) {};

            EReadError(int SysError, LPCTSTR lpFormat, ...) : inherited() {
                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);

                SystemError(SysError);
            };

            ~EReadError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EWriteError : public EFilerError {
            typedef EFilerError inherited;

        public:

            EWriteError() : inherited() {};

            explicit EWriteError(LPCTSTR lpMsg) : inherited(lpMsg) {};

            explicit EWriteError(int SysError) : inherited(SysError) {};

            EWriteError(int SysError, LPCTSTR lpFormat, ...) : inherited() {
                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);

                SystemError(SysError);
            };

            ~EWriteError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EOSError : public Exception {
            typedef Exception inherited;

        public:

            explicit EOSError(int SysError) : inherited(SysError) {};

            EOSError(LPCTSTR lpMsg, int SysError) : inherited(lpMsg, SysError) {};

            EOSError(int SysError, LPCTSTR lpFormat, ...) : inherited() {
                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);

                SystemError(SysError);
            };

            ~EOSError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EConvertError : public Exception {
            typedef Exception inherited;

        public:

            EConvertError() : inherited() {};

            explicit EConvertError(LPCTSTR lpFormat, ...) : inherited() {
                va_list argList;
                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);
            };

            ~EConvertError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI ESocketError : public Exception {
            typedef Exception inherited;

        private:

            int m_LastError;

        public:

            ESocketError() : inherited(), m_LastError(0) {};

            explicit ESocketError(LPCTSTR lpMsg) : inherited(lpMsg), m_LastError(0) {};

            explicit ESocketError(int SysError) : inherited(SysError) { m_LastError = SysError; };

            ESocketError(int SysError, LPCTSTR lpFormat, ...) : inherited() {

                m_LastError = SysError;

                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);

                SystemError(SysError);
            };

            ~ESocketError() override = default;

            int GetLastError() { return m_LastError; }
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI ETCPServerError : public Exception {
            typedef Exception inherited;

        public:

            ETCPServerError() : inherited() {};

            explicit ETCPServerError(LPCTSTR lpMsg) : Exception(lpMsg) {};

            ~ETCPServerError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EHTTPServerError : public Exception {
            typedef Exception inherited;

        public:

            EHTTPServerError() : inherited() {};

            explicit EHTTPServerError(LPCTSTR lpMsg) : Exception(lpMsg) {};

            ~EHTTPServerError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EPollServerError : public Exception {
            typedef Exception inherited;

        public:

            EPollServerError() : inherited() {};

            explicit EPollServerError(LPCTSTR lpMsg) : Exception(lpMsg) {};

            ~EPollServerError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EDBError : public Exception {
            typedef Exception inherited;

        public:

            EDBError() : inherited() {};

            explicit EDBError(LPCTSTR lpFormat, ...) : inherited() {

                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);
            };

            ~EDBError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EDBConnectionError : public EDBError {
            typedef EDBError inherited;

        public:

            EDBConnectionError() : inherited() {};

            explicit EDBConnectionError(LPCTSTR lpFormat, ...) : inherited() {

                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);
            };

            ~EDBConnectionError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI EJSONParseSyntaxError : public Exception {
            typedef Exception inherited;

        public:

            EJSONParseSyntaxError() : inherited() {};

            explicit EJSONParseSyntaxError(LPCTSTR lpFormat, ...) : inherited() {

                va_list argList;

                va_start(argList, lpFormat);
                FormatMessage(lpFormat, argList);
                va_end(argList);
            };

            ~EJSONParseSyntaxError() override = default;
        };
        //--------------------------------------------------------------------------------------------------------------

    } // namespace Exception;
}

using namespace Delphi::Exception;
}

#endif //DELPHI_EXCEPTION_HPP
