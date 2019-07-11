/*++

Programm name:

  apostol

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

#include "Module.hpp"
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
