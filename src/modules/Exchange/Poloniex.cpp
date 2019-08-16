/*++

Programm name:

  Apostol

Module Name:

  Poloniex.cpp

Notices:

  Apostol Web Service. Module Exchange. The Poloniex APIs.

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Poloniex.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Poloniex {

        void CPoloniex::PairToSymbol(const CString &Pair, CString &Symbol) {

            size_t Pos = Pair.Find("-");

            if (Pos == CString::npos) {
                throw Delphi::Exception::ExceptionFrm(_T("Invalid format Pair: \"%s\""), Pair.c_str());
            }

            Symbol = Pair.SubString(Pos + 1);
            Symbol += "_";
            Symbol += Pair.SubString(0, Pos);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPoloniex::GetServerTime(CJSON &Result) {
            throw Delphi::Exception::ExceptionFrm(_T("[%s] No support."), Name().c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPoloniex::GetTicker(const CString &Symbol, CJSON &Result) {

            Log()->Debug(0, "[%s] GetTicker", Name().c_str());

            CString url(Uri());
            url += "/public?command=returnTicker";

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

        void CPoloniex::GetOrderBook(const CStringList &Params, CString &Result) {

            Log()->Debug(0, "[%s] Order book", Name().c_str());

            CString Symbol;
            PairToSymbol(Params["pair"], Symbol);

            CString url(Uri());
            url += "/public?";

            CString QueryString("command=returnOrderBook&currencyPair=");
            QueryString.Append(Symbol);

            QueryString.Append("&depth="); // Max: 100
            QueryString.Append(Params["limit"]);

            url.Append(QueryString);

            Log()->Debug(0, "[%s] uri: %s", Name().c_str(), url.c_str());

            curl_api(url, Result);

            if (!Result.IsEmpty()) {
                Log()->Debug(0, "[%s] Order book: Done!", Name().c_str());
            } else {
                Log()->Debug(0, "[%s] Order book: Failed to get anything", Name().c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPoloniex::NewOrder(const CStringList &Params, CString &Result) {

            Log()->Debug(0, "[%s] Order", Name().c_str());

            if (ApiKey().IsEmpty()) {
                throw Delphi::Exception::ExceptionFrm(_T("[%s] API Key has not been set"), Name().c_str());
            }

            if (SecretKey().IsEmpty()) {
                throw Delphi::Exception::ExceptionFrm(_T("[%s] Secret Key has not been set"), Name().c_str());
            }

            CString action = "POST";

            CString Symbol;
            PairToSymbol(Params["pair"], Symbol);

            CString Rate(Params["rate"]);

            if (Rate.IsEmpty()) {
                CJSON Json;
                GetTicker(Symbol, Json);
                Rate = Json[Symbol]["last"].AsSiring();
            }

            CString url(Uri());
            url += "/tradingApi";

            CString post_data("command=");
            post_data.Append( Params["type"].Lower() );

            post_data.Append("&currencyPair=");
            post_data.Append( Symbol );

            post_data.Append("&rate=");
            post_data.Append( Rate );

            post_data.Append("&amount=");
            post_data.Append( Params["amount"] );

            const CString& fillOrKill = Params["fillOrKill"];

            if ( !fillOrKill.IsEmpty() && fillOrKill != "0" ) {
                post_data.Append("&fillOrKill=");
                post_data.Append( fillOrKill );
            }

            const CString& immediateOrCancel = Params["immediateOrCancel"];

            if ( !immediateOrCancel.IsEmpty() && immediateOrCancel != "0" ) {
                post_data.Append("&immediateOrCancel=");
                post_data.Append( immediateOrCancel );
            }

            const CString& postOnly = Params["postOnly"];

            if ( !postOnly.IsEmpty() && postOnly != "0" ) {
                post_data.Append("&postOnly=");
                post_data.Append( postOnly );
            }

            post_data.Append("&nonce=");
            post_data.Append( to_string(get_current_ms_epoch()) );

            CString signature = hmac_sha512( SecretKey(), post_data );

            CStringList extra_http_header;

            extra_http_header.Add(CString("Key: "));
            extra_http_header.Last().Append(ApiKey());

            extra_http_header.Add(CString("Sign: "));
            extra_http_header.Last().Append(signature);

            Log()->Debug(0, "[%s] uri: %s", Name().c_str(), url.c_str());
            Log()->Debug(0, "[%s] post data: %s", Name().c_str(), post_data.c_str());
            Log()->Debug(0, "[%s] signature: %s", Name().c_str(), signature.c_str());

            curl_api_with_header( url, Result, extra_http_header, post_data, action );

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
