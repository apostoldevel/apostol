/*++

Programm name:

  Apostol

Module Name:

  Apostol.hpp

Notices:

  Apostol Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_APOSTOL_HPP
#define APOSTOL_APOSTOL_HPP

#include "Header.hpp"
#include "../../version.h"
//----------------------------------------------------------------------------------------------------------------------

#define AWS_NAME         _T(APOSTOL_NAME)
#define AWS_DESCRIPTION  _T(APOSTOL_DESCRIPTION)
#define AWS_VERSION      _T(AUTO_VERSION)
#define AWS_VER          AWS_NAME _T("/") AWS_VERSION
#define AWS_VAR          _T("APOSTOL")
#define AWS_OLDPID_EXT   _T(".oldbin")
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Apostol {

        class CApostol: public CApplication {
        protected:

            void ParseCmdLine() override;
            void ShowVersioInfo() override;

        public:

            CApostol(int argc, char *const *argv): CApplication(argc, argv) {

            };

            ~CApostol() override = default;

            static class CApostol *Create(int argc, char *const *argv) {
                return new CApostol(argc, argv);
            };

            inline void Destroy() override { delete this; };

            void Run() override;

        };
    }
}

using namespace Apostol::Apostol;
}

#endif //APOSTOL_APOSTOL_HPP

