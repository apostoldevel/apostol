/*++

Programm name:

  Apostol

Module Name:

  Modules.hpp

Notices:

  Apostol Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_MODULES_HPP
#define APOSTOL_MODULES_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Modules {


        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CApostolModule: public CObject, public CGlobalComponent {
        private:

            bool m_bFreeAfterExecute;

        protected:

            virtual void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) abstract;
            virtual void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) abstract;

        public:

            CApostolModule(): CObject(), CGlobalComponent(), m_bFreeAfterExecute(true) {};

            ~CApostolModule() override = default;

            virtual void Execute(CHTTPConnection *AConnection) abstract;

            CPQPollQuery *GetQuery(CPollConnection *AConnection);

            bool FreeAfterExecute() { return m_bFreeAfterExecute; };
            void FreeAfterExecute(bool Value) { m_bFreeAfterExecute = Value; };

        };

    }
}

using namespace Apostol::Modules;
}
//----------------------------------------------------------------------------------------------------------------------

#include "WebServer/WebServer.hpp"
#include "Client365/Client365.hpp"
#include "Exchange/Exchange.hpp"
//----------------------------------------------------------------------------------------------------------------------

static CApostolModule* CreateModule(const CString &UserAgent) {

    if (!UserAgent.IsEmpty()) {
        if ((UserAgent.Find(_T("Client365/")) != CString::npos)) {
            return CClient365::CreateModule();
        }

        if ((UserAgent.Find(_T("Exchange/")) != CString::npos)) {
            return CExchange::CreateModule();
        }
    }

    return CWebServer::CreateModule();
}

#endif //APOSTOL_MODULES_HPP
