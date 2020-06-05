/*++

Program name:

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

#include "WebService/WebService.hpp"
//----------------------------------------------------------------------------------------------------------------------

static void CreateWorkers(CModuleProcess *AProcess) {
    CWebService::CreateModule(AProcess);
}

#endif //APOSTOL_MODULES_HPP
