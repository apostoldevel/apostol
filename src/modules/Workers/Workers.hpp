/*++

Program name:

  apostol

Module Name:

  Workers.hpp

Notices:

  Apostol

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_WORKERS_HPP
#define APOSTOL_WORKERS_HPP
//----------------------------------------------------------------------------------------------------------------------

#include "WebServer/WebServer.hpp"
#include "VK/VK.hpp"
//----------------------------------------------------------------------------------------------------------------------

static inline void CreateWorkers(CModuleProcess *AProcess) {
    CVK::CreateModule(AProcess);
    CWebServer::CreateModule(AProcess);
}

#endif //APOSTOL_WORKERS_HPP
