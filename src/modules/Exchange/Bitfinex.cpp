/*++

Programm name:

  Apostol

Module Name:

  Bitfinex.cpp

Notices:

  Apostol Web Service. Module Exchange. The Bitfinex APIs.

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Bitfinex.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Bitfinex {

        void CBitfinex::PairToSymbol(const CString &Pair, CString &Symbol) {

            CString Str;

            size_t Pos = Pair.Find("-");

            if (Pos == CString::npos) {
                throw Delphi::Exception::ExceptionFrm(_T("Invalid format Pair: \"%s\""), Pair.c_str());
            }

            Symbol = Pair.SubString(0, Pos);
            Str = Pair.SubString(Pos + 1);

            if (Str == "USDT") {
                Str = "USD";
            }

            Symbol += Str;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitfinex::GetServerTime(CJSON &Result) {

            Log()->Debug(0, "[%s] GetServerTime", Name().c_str());

            CString url;

            url += "/v1/pubticker/";

            CString str_result;

            curl_api(url, str_result);

            if (!str_result.IsEmpty()) {
                try {
                    Result.Clear();
                    Result<< str_result;
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_EMERG, 0, "[%s] GetServerTime: Error: %s", Name().c_str());
                }
                Log()->Debug(0, "[%s] GetServerTime: Done!", Name().c_str());

            } else {
                Log()->Error(APP_LOG_EMERG, 0, "[%s] GetServerTime: Failed to get anything.", Name().c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitfinex::GetTicker(const CString &Symbol, CJSON &Result) {

            Log()->Debug(0, "[%s] GetTicker", Name().c_str());

            CString url(Uri());

            url += "/v1/pubticker/";
            url += Symbol;

            CString str_result;
            curl_api(url, str_result);

            if (!str_result.IsEmpty()) {
                try {
                    Result.Clear();
                    Result << str_result;
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_EMERG, 0, "[%s] GetTicker: Error: %s", Name().c_str(), e.what());
                }
                Log()->Debug(0, "[%s] GetTicker: Done!", Name().c_str());
            } else {
                Log()->Error(APP_LOG_EMERG, 0, "[%s] GetTicker: Failed to get anything.", Name().c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitfinex::GetOrderBook(const CStringList &Params, CString &Result) {

            Log()->Debug(0, "[%s] Order book", Name().c_str());

            CString Symbol;
            PairToSymbol(Params["pair"], Symbol);

            CString url(Uri());
            url += "/v1/book/";

            url.Append(Symbol.Lower());

            Log()->Debug(0, "[%s] uri: %s", Name().c_str(), url.c_str());

            curl_api(url, Result);

            if (!Result.IsEmpty()) {
                Log()->Debug(0, "[%s] Order book: Done!", Name().c_str());
            } else {
                Log()->Debug(0, "[%s] Order book: Failed to get anything", Name().c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBitfinex::NewOrder(const CStringList &Params, CString &Result) {

            Log()->Debug(0, "[%s] Order", Name().c_str());

            if (ApiKey().IsEmpty()) {
                throw Delphi::Exception::ExceptionFrm(_T("[%s] API Key has not been set"), Name().c_str());
            }

            if (SecretKey().IsEmpty()) {
                throw Delphi::Exception::ExceptionFrm(_T("[%s] Secret Key has not been set"), Name().c_str());
            }

            CString Symbol;
            PairToSymbol(Params["pair"], Symbol);

            CString action = "POST";

            CString url(Uri());
            url += "/v1/order/new";

            CString payload;

            payload.Format(R"({"request": "%s", "nonce": "%lu", "symbol": "%s", "amount": "%s", "price": "%d", "exchange": "%s", "side": "%s", "type": "%s"})",
                           "/v1/order/new",
                           get_current_ms_epoch(),
                           Symbol.c_str(),
                           Params["amount"].c_str(),
                           1000,
                           "bitfinex",
                           Params["type"].Lower().c_str(),
                           "market"
            );

            CString payload_enc = base64_encode(payload);

            CString signature = hmac_sha384( SecretKey(), payload_enc );

            CStringList extra_http_header;

            extra_http_header.Add(CString("Content-Type: "));
            extra_http_header.Last().Append( "application/json" );

            extra_http_header.Add(CString("Accept: "));
            extra_http_header.Last().Append( "application/json" );

            extra_http_header.Add(CString("X-BFX-APIKEY: "));
            extra_http_header.Last().Append( ApiKey() );

            extra_http_header.Add(CString("X-BFX-SIGNATURE: "));
            extra_http_header.Last().Append( signature );

            extra_http_header.Add(CString("X-BFX-PAYLOAD: "));
            extra_http_header.Last().Append( payload_enc );

            Log()->Debug(0, "[%s] uri: %s", Name().c_str(), url.c_str());
            Log()->Debug(0, "[%s] post data: %s", Name().c_str(), payload.c_str());
            Log()->Debug(0, "[%s] signature: %s", Name().c_str(), signature.c_str());

            curl_api_with_header( url, Result, extra_http_header, payload, action ) ;

            if (!Result.IsEmpty()) {
                Log()->Debug(0, "[%s] Order: Done!", Name().c_str());
            } else {
                Log()->Debug(0, "[%s] Order: Failed to get anything", Name().c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

    }
}
}
