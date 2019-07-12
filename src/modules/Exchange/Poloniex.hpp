/*++

Programm name:

  Apostol

Module Name:

  Poloniex.hpp

Notices:

  Apostol Web Service. Module Exchange. The Poloniex APIs.

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_POLONIEX_HPP
#define APOSTOL_POLONIEX_HPP

#include "Apostol.hpp"
#include "Exchange.hpp"
//----------------------------------------------------------------------------------------------------------------------

#define POLONIEX_HOST   "https://poloniex.com"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Poloniex {

        class CPoloniex: public CExchangeApi {
        private:

            void PairToSymbol(const CString& Pair, CString& Symbol) override;

        public:

            CPoloniex(const CString &Uri, const CString &ApiKey, const CString &SecretKey,
                      bool Enabled = true): CExchangeApi(etPoloniex, Uri.IsEmpty() ? POLONIEX_HOST : Uri, ApiKey, SecretKey, Enabled) {
            };

            void GetServerTime(CJSON &Result) override;
            void GetTicker(const CString& Symbol, CJSON &Result) override;

            void GetOrderBook(const CStringList &Params, CString &Result) override;
            void NewOrder(const CStringList &Params, CString &Result) override;

        };
        //--------------------------------------------------------------------------------------------------------------

    }
}

using namespace Apostol::Poloniex;
}

#endif //APOSTOL_POLONIEX_HPP
