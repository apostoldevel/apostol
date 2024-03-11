/*++

Program name:

  apostol

Module Name:

  Helpers.hpp

Notices:

  Apostol

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_HELPERS_HPP
#define APOSTOL_HELPERS_HPP
//----------------------------------------------------------------------------------------------------------------------

#include "PGFetch/PGFetch.hpp"

static inline void CreateHelpers(CModuleProcess *AProcess) {
    CPGFetch::CreateModule(AProcess);
}

#endif //APOSTOL_HELPERS_HPP
