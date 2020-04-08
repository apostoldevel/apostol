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

        class CWebServer: public CApostolModule {
        private:

            static void DebugRequest(CRequest *ARequest);
            static void DebugReply(CReply *AReply);
            static void DebugConnection(CHTTPServerConnection *AConnection);

        protected:

            void DoGet(CHTTPServerConnection *AConnection);

        public:

            explicit CWebServer(CModuleManager *AManager);

            ~CWebServer() override = default;

            static class CWebServer *CreateModule(CModuleManager *AManager) {
                return new CWebServer(AManager);
            }

            void InitMethods() override;

            void BeforeExecute(Pointer Data) override;
            void AfterExecute(Pointer Data) override;

            void Heartbeat() override;
            void Execute(CHTTPServerConnection *AConnection) override;

            bool CheckUserAgent(const CString& Value) override;

        };

    }
}

using namespace Apostol::WebServer;
}
#endif //APOSTOL_WEBSERVER_HPP
