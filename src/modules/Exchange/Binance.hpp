/*++

Programm name:

  Apostol

Module Name:

  Poloniex.hpp

Notices:

  Apostol Web Service. Module Exchange. The Binance APIs.

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_BINANCE_HPP
#define APOSTOL_BINANCE_HPP

#include "Apostol.hpp"
#include "Exchange.hpp"
//----------------------------------------------------------------------------------------------------------------------

#define BINANCE_HOST    "https://api.binance.com"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Binance {

        class CBinance: public CExchangeApi {
        private:

            void PairToSymbol(const CString& Pair, CString& Symbol) override;

        public:

            CBinance(const CString &Uri, const CString &ApiKey, const CString &SecretKey,
                    bool Enabled = true): CExchangeApi(etBinance, Uri.IsEmpty() ? BINANCE_HOST : Uri, ApiKey, SecretKey, Enabled) {
            };

            void GetServerTime(CJSON &Result) override;
            void GetTicker(const CString& Symbol, CJSON &Result) override;

            void GetOrderBook(const CStringList &Params, CString &Result) override;
            void NewOrder(const CStringList &Params, CString &Result) override;

        };
        //--------------------------------------------------------------------------------------------------------------

    }
}

using namespace Apostol::Binance;
}

#endif //APOSTOL_BINANCE_HPP
