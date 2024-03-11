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

#include "PGHTTP/PGHTTP.hpp"
#include "WebServer/WebServer.hpp"
//----------------------------------------------------------------------------------------------------------------------

static inline void CreateWorkers(CModuleProcess *AProcess) {
    CPGHTTP::CreateModule(AProcess);
    CWebServer::CreateModule(AProcess);
}

#endif //APOSTOL_WORKERS_HPP
