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

static void CreateModule(CModuleManager *AManager) {
    CClient365::CreateModule(AManager);
    CExchange::CreateModule(AManager);
    CWebServer::CreateModule(AManager);
}

#endif //APOSTOL_MODULES_HPP
