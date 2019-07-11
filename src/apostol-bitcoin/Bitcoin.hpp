/*++

Programm name:

  abc

Module Name:

  Bitcoin.hpp

Notices:

  Apostol Bitcoin

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_BITCOIN_HPP
#define APOSTOL_BITCOIN_HPP

#include "../../version.h"
//----------------------------------------------------------------------------------------------------------------------

#define APP_VERSION      AUTO_VERSION
#define APP_VER          APP_NAME "/" APP_VERSION
//----------------------------------------------------------------------------------------------------------------------

#include "Core.hpp"
#include "Modules.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Bitcoin {

        class CApostolBitcoin: public CApplication {
        protected:

            void ParseCmdLine() override;
            void ShowVersioInfo() override;

        public:

            CApostolBitcoin(int argc, char *const *argv): CApplication(argc, argv) {

            };

            ~CApostolBitcoin() override = default;

            static class CApostolBitcoin *Create(int argc, char *const *argv) {
                return new CApostolBitcoin(argc, argv);
            };

            inline void Destroy() override { delete this; };

            void Run() override;

        };
    }
}

using namespace Apostol::Bitcoin;
}

#endif //APOSTOL_BITCOIN_HPP

