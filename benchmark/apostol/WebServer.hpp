/*++

Program name:

  Apostol Web Service

Module Name:

  WebServer.hpp

Notices:

  Module: Web Server (Benchmark)

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

    namespace Module {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebServer ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebServer: public CApostolModule {
        private:

            bool m_KeepAlive;

            void InitMethods() override;

        protected:

            void DoGet(CHTTPServerConnection *AConnection) override;

        public:

            explicit CWebServer(CModuleProcess *AProcess);

            ~CWebServer() override = default;

            static class CWebServer *CreateModule(CModuleProcess *AProcess) {
                return new CWebServer(AProcess);
            }

            bool Enabled() override;

        };
    }
}

using namespace Apostol::Module;
}
#endif //APOSTOL_WEBSERVER_HPP
