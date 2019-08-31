/*++

Programm name:

  Apostol

Module Name:

  WebServer.hpp

Notices:

  Apostol Web Server

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_WEBSERVER_HPP
#define APOSTOL_WEBSERVER_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace WebServer {

        typedef std::function<void (CHTTPServerConnection *AConnection)> COnHeaderHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CHeaderHandler: CObject {
        private:

            bool m_Allow;
            COnHeaderHandlerEvent m_Handler;

        public:

            CHeaderHandler(bool Allow, COnHeaderHandlerEvent && Handler): CObject(),
                m_Allow(Allow), m_Handler(Handler) {};

            bool Allow() { return m_Allow; };

            void Handler(CHTTPServerConnection *AConnection) {
                if (m_Allow && m_Handler)
                    m_Handler(AConnection);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        class CWebServer: public CApostolModule {
        private:

            CStringList *m_Headers;

            CString m_AllowedMethods;

            void DoOptions(CHTTPServerConnection *AConnection);
            void DoGet(CHTTPServerConnection *AConnection);
            void MethodNotAllowed(CHTTPServerConnection *AConnection);

        protected:

            void InitHeaders();

            const CString& GetAllowedMethods();
#ifdef USE_POSTGRESQL
            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override {};
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override {};
#endif
        public:

            explicit CWebServer(CModuleManager *AManager);

            ~CWebServer() override;

            static class CWebServer *CreateModule(CModuleManager *AManager) {
                return new CWebServer(AManager);
            }

            void Execute(CHTTPServerConnection *AConnection) override;

            bool CheckUrerArent(const CString& Value) override { return true; };

            const CString& AllowedMethods() { return GetAllowedMethods(); };

        };

    }
}

using namespace Apostol::WebServer;
}
#endif //APOSTOL_WEBSERVER_HPP
