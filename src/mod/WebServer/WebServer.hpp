/*++

Programm name:

  Apostol

Module Name:

  WebServer.hpp

Notices:

  Apostol Web Service (WebServer)

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

        typedef std::function<void (CHTTPConnection *AConnection)> COnHeaderHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CHeaderHandler: CObject {
        private:

            bool m_Allow;
            COnHeaderHandlerEvent m_Handler;

        public:

            CHeaderHandler(bool Allow, COnHeaderHandlerEvent && Handler): CObject(),
                m_Allow(Allow), m_Handler(Handler) {};

            bool Allow() { return m_Allow; };

            void Handler(CHTTPConnection *AConnection) {
                if (m_Allow && m_Handler)
                    m_Handler(AConnection);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        class CWebServer: public CApostolModule {
        private:

            CStringList *m_Headers;

            CString m_AllowedMethods;

            void DoOptions(CHTTPConnection *AConnection);
            void DoGet(CHTTPConnection *AConnection);
            void MethodNotAllowed(CHTTPConnection *AConnection);

        protected:

            void InitHeaders();

            const CString& GetAllowedMethods();

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override {};
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override {};

        public:

            CWebServer();

            ~CWebServer() override;

            static class CWebServer *CreateModule() {
                return new CWebServer();
            }

            void Execute(CHTTPConnection *AConnection) override;

            const CString& AllowedMethods() { return GetAllowedMethods(); };

        };

    }
}

using namespace Apostol::WebServer;
}
#endif //APOSTOL_WEBSERVER_HPP
