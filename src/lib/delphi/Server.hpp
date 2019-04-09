/*++

Library name:

  libdelphi

Module Name:

  Server.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_SERVER_HPP
#define DELPHI_SERVER_HPP
//----------------------------------------------------------------------------------------------------------------------

#define DefaultServerName  DELPHI_LIB_VER
#define DefaultAllowedMethods  _T("OPTIONS, GET")
//----------------------------------------------------------------------------------------------------------------------

#ifndef WWWServerName
#define WWWServerName DefaultServerName
#endif

#ifndef WWWAllowedMethods
#define WWWAllowedMethods DefaultAllowedMethods
#endif
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Delphi {

    namespace Server {

        namespace Mapping {
            LPCTSTR ExtToType(LPCTSTR Ext);
            bool IsText(LPCTSTR MimeType);
        }
        //--------------------------------------------------------------------------------------------------------------

        typedef struct http_header {

            CString Name;
            CString Value;
            CStringList Options;

            http_header& operator= (const http_header& H) {
                if (this != &H) {
                    Name = H.Name;
                    Value = H.Value;
                    Options = H.Options;
                }
                return *this;
            };

            inline bool operator!= (const http_header& Value) { return Name != Value.Name; };
            inline bool operator== (const http_header& Value) { return Name == Value.Name; };

        } CHeader, *PHeader;

        //--------------------------------------------------------------------------------------------------------------

        //-- CHeaders --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CHeaders {
        private:

            TList<CHeader> *m_pList;

            CString m_NullValue;

            void Put(int Index, const CHeader& Header);

            CHeader& Get(int Index);
            const CHeader& Get(int Index) const;

        protected:

            int GetCount() const;

            const CString &GetValue(const CString &Name) const;
            const CString &GetValue(LPCTSTR Name) const;

        public:

            CHeaders();

            ~CHeaders();

            void Clear();

            int IndexOfName(const CString &Name) const;
            int IndexOfName(LPCTSTR Name) const;

            void Insert(int Index, const CHeader& Header);

            int Add(const CHeader& Header);

            void Delete(int Index);

            void SetCount(int NewCount);

            CHeader& First();

            CHeader& Last();

            int Count() const { return GetCount(); }

            void Assign(const CHeaders& Headers);

            const CString &Values(const CString& Name) const { return GetValue(Name); };
            const CString &Values(LPCTSTR Name) const { return GetValue(Name); };

            CHeader& Headers(int Index) { return Get(Index); }
            void Headers(int Index, const CHeader& Header) { Put(Index, Header); }

            CHeaders& operator= (const CHeaders& H) {
                if (this != &H)
                    Assign(H);
                return *this;
            };

            CHeader& operator[](int Index) { return Get(Index); }
            const CHeader& operator[](int Index) const { return Get(Index); }
        };
        //--------------------------------------------------------------------------------------------------------------

        typedef struct http_request {
            CString Method;
            CString Uri;
            CStringList Params;
            int VMajor;
            int VMinor;
            CHeaders Headers;
            size_t ContentLength = 0;
            CString Content;
        } CRequest, *PRequest;

        //--------------------------------------------------------------------------------------------------------------

        //-- CRequestParser --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef enum parcer_state
        {
            method_start,
            method,
            uri_start,
            uri,
            uri_param_start,
            uri_param,
            http_version_h,
            http_version_t_1,
            http_version_t_2,
            http_version_p,
            http_version_slash,
            http_version_major_start,
            http_version_major,
            http_version_minor_start,
            http_version_minor,
            expecting_newline_1,
            header_line_start,
            header_lws,
            header_name,
            space_before_header_value,
            header_value,
            header_value_options_start,
            header_value_options,
            expecting_newline_2,
            expecting_newline_3,
            content
        } CParcerState;
        //--------------------------------------------------------------------------------------------------------------

        /// Parser for incoming requests.
        class CRequestParser: public CObject {
        private:

            /// The current state of the parser.
            CParcerState m_State;

            /// The current result of the parser.
            int m_Result;

            /// Handle the next character of input.
            int Consume(CRequest *ARequest, char AInput);

            /// Check if a byte is an HTTP character.
            static bool IsChar(int c);

            /// Check if a byte is an HTTP control character.
            static bool IsCtl(int c);

            /// Check if a byte is defined as an HTTP tspecial character.
            static bool IsTSpecial(int c);

            /// Check if a byte is a digit.
            static bool IsDigit(int c);

        public:

            explicit CRequestParser();

            ~CRequestParser() override = default;

            /// Reset to initial parser state.
            void Reset();

            /// Parse some data. The int return value is "1" when a complete request
            /// has been parsed, "0" if the data is invalid, "-1" when more
            /// data is required.
            int Parse(CRequest *ARequest, LPTSTR ABegin, LPCTSTR AEnd);

            CParcerState State() { return m_State; };

            int Result() { return m_Result; };

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CReply ----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CHTTPConnection;
        //--------------------------------------------------------------------------------------------------------------

        typedef struct http_reply
        {
            /// The status of the reply.
            enum status_type
            {
                ok = 200,
                created = 201,
                accepted = 202,
                non_authoritative = 203,
                no_content = 204,
                multiple_choices = 300,
                moved_permanently = 301,
                moved_temporarily = 302,
                not_modified = 304,
                bad_request = 400,
                unauthorized = 401,
                forbidden = 403,
                not_found = 404,
                not_allowed = 405,
                internal_server_error = 500,
                not_implemented = 501,
                bad_gateway = 502,
                service_unavailable = 503,
                gateway_timeout = 504
            } Status;

            /// The content type of the reply.
            enum content_type
            {
                html = 0,
                json = 1,
                text = 2,
                sbin = 3,
            } ContentType = html;

            LPCTSTR ServerName;

            LPCTSTR AllowedMethods;

            bool CloseConnection = true;

            /// The headers to be included in the reply.
            CHeaders Headers;

            /// The content to be sent in the reply.
            CString Content;

            /// Clear content and headers.
            void Clear();

            void ToText();
            void ToJSON();

            /// Convert the reply into a vector of buffers. The buffers do not own the
            /// underlying memory blocks, therefore the reply object must remain valid and
            /// not be changed until the write operation has completed.
            void ToBuffers(CMemoryStream *AStream);

            /// Add header to headers.
            void AddHeader(LPCTSTR lpszName, LPCTSTR lpszValue);

            /// Get a prepare reply.
            static http_reply *GetReply(http_reply *AReply, status_type AStatus, LPCTSTR AContentType = nullptr);

            /// Get a stock reply.
            static http_reply *GetStockReply(http_reply *AReply, status_type AStatus);

        } CReply, *PReply;

        //--------------------------------------------------------------------------------------------------------------

        //-- CHTTPConnection -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CHTTPServer;
        //--------------------------------------------------------------------------------------------------------------

        enum CHTTPConnectionStatus { csConnected = 0, csWaitRequest, csRequestOk, csRequestError, csReplyReady, csReplySent };
        //--------------------------------------------------------------------------------------------------------------

        class CHTTPConnection: public CTCPServerConnection {
            typedef CTCPServerConnection inherited;

        private:

            CRequest *m_Request;

            CReply *m_Reply;

            CRequestParser *m_RequestParser;

            CHTTPConnectionStatus m_ConnectionStatus;

            bool m_CloseConnection;

            CNotifyEvent m_OnReply;

        protected:

            CRequest *GetRequest();

            CReply *GetReply();

            void DoReply();

        public:

            explicit CHTTPConnection(CHTTPServer *AServer);

            ~CHTTPConnection() override;

            void ParseInput();

            void Clear();

            CHTTPServer *HTTPServer() { return (CHTTPServer *) Server(); }

            CRequestParser *RequestParser() { return m_RequestParser; }

            CRequest *Request() { return GetRequest(); }

            CReply *Reply() { return GetReply(); };

            bool CloseConnection() { return m_CloseConnection; };
            void CloseConnection(bool Value) { m_CloseConnection = Value; };

            CHTTPConnectionStatus ConnectionStatus() { return m_ConnectionStatus; };
            void ConnectionStatus(CHTTPConnectionStatus Value) { m_ConnectionStatus = Value; };

            void SendReply(bool ASendNow = false);

            void SendReply(CReply::status_type AStatus, LPCTSTR AContentType = nullptr, bool ASendNow = false);

            void SendStockReply(CReply::status_type AStatus, bool ASendNow = false);

            const CNotifyEvent &OnReply() { return m_OnReply; }
            void OnReply(CNotifyEvent && Value) { m_OnReply = Value; }

        }; // CHTTPConnection

        //--------------------------------------------------------------------------------------------------------------

        //-- CHTTPServer -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CHTTPServer: public CPollServer {
        private:

            CString m_sDocRoot;

            void SetDocRoot(LPCTSTR AValue);

        protected:

            void DoTimeOut(CPollEventHandler* AHandler) override;

            void DoAccept(CPollEventHandler* AHandler) override;

            void DoRead(CPollEventHandler* AHandler) override;

            void DoWrite(CPollEventHandler* AHandler) override;

            void DoCommand(CTCPServerConnection *AConnection);

            bool DoExecute(CTCPServerConnection *AConnection) override;

            void DoReply(CObject *Sender);

        public:

            explicit CHTTPServer(unsigned short AListen, LPCTSTR lpDocRoot);

            ~CHTTPServer() override = default;

            bool URLDecode(const CString& In, CString& Out);

            CString& DocRoot() { return m_sDocRoot; };
            void DocRoot(LPCTSTR AValue) { SetDocRoot(AValue); };

        };

    }
}

using namespace Delphi::Server;
}

#endif //DELPHI_SERVER_HPP
