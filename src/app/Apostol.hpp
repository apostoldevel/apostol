/*++

Program name:

  apostol

Module Name:

  Apostol.hpp

Notices:

  Apostol

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_APOSTOL_HPP
#define APOSTOL_APOSTOL_HPP

#include "../../version.h"
//----------------------------------------------------------------------------------------------------------------------

#define APP_VERSION      AUTO_VERSION
#define APP_VER          APP_NAME "/" APP_VERSION
//----------------------------------------------------------------------------------------------------------------------

#include "Header.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Application {

        class CApostol: public CApplication {
        protected:

            void ParseCmdLine() override;
            void ShowVersionInfo() override;

            void StartProcess() override;

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
}

#endif //APOSTOL_APOSTOL_HPP

