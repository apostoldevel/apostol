/*++

Programm name:

  Apostol

Module Name:

  Bitfinex.hpp

Notices:

  Apostol Web Service. Module Exchange. The Bitfinex APIs.

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_BITFINEX_HPP
#define APOSTOL_BITFINEX_HPP

#include "Apostol.hpp"
#include "Exchange.hpp"
//----------------------------------------------------------------------------------------------------------------------

#define BITFINEX_HOST   "https://api.bitfinex.com"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Bitfinex {

        class CBitfinex: public CExchangeApi {
        private:

            void PairToSymbol(const CString& Pair, CString& Symbol) override;

        public:

            CBitfinex(const CString &Uri, const CString &ApiKey, const CString &SecretKey,
                     bool Enabled = true): CExchangeApi(etBitfinex, Uri.IsEmpty() ? BITFINEX_HOST : Uri, ApiKey, SecretKey, Enabled) {
            };

            void GetServerTime(CJSON &Result) override;
            void GetTicker(const CString& Symbol, CJSON &Result) override;

            void GetOrderBook(const CStringList &Params, CString &Result) override;
            void NewOrder(const CStringList &Params, CString &Result) override;

        };
        //--------------------------------------------------------------------------------------------------------------

    }
}

using namespace Apostol::Bitfinex;
}

#endif //APOSTOL_BITFINEX_HPP
