/*++

Programm name:

  Apostol

Module Name:

  Poloniex.cpp

Notices:

  Apostol Web Service. Module Exchange. The Binance APIs.

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Binance.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Binance {

        void CBinance::PairToSymbol(const CString &Pair, CString &Symbol) {

            size_t Pos = Pair.Find("-");

            if (Pos == CString::npos) {
                throw Delphi::Exception::ExceptionFrm(_T("Invalid format Pair: \"%s\""), Pair.c_str());
            }

            Symbol = Pair.SubString(0, Pos);
            Symbol += Pair.SubString(Pos + 1);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBinance::GetServerTime(CJSON &Result) {

            Log()->Debug(0, "[%s] GetServerTime", Name().c_str());

            CString url;

            url += "/api/v1/time";

            CString str_result;
            curl_api(url, str_result);

            if (!str_result.IsEmpty()) {
                try {
                    Result.Clear();
                    Result << str_result;
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(APP_LOG_EMERG, 0, "[%s] GetServerTime: Error: %s", Name().c_str());
                }
                Log()->Debug(0, "[%s] GetServerTime: Done!", Name().c_str());

            } else {
                Log()->Error(APP_LOG_EMERG, 0, "[%s] GetServerTime: Failed to get anything.", Name().c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CBinance::GetTicker(const CString &Symbol, CJSON &Result) {

            Log()->Debug(0, "[%s] GetTicker", Name().c_str());

            CString url(Uri());

            url += "/api/v3/ticker/price";

            if (!Symbol.IsEmpty()) {
                url += "?symbol=";
                url += Symbol;
            }

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

        void CBinance::GetOrderBook(const CStringList &Params, CString &Result) {

            Log()->Debug(0, "[%s] Order book", Name().c_str());

            CString Symbol;
            PairToSymbol(Params["pair"], Symbol);

            CString url(Uri());
            url += "/api/v1/depth?";

            CString QueryString("symbol=");
            QueryString.Append(Symbol);

            QueryString.Append("&limit=");
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

        void CBinance::NewOrder(const CStringList &Params, CString &Result) {

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
            url += "/api/v3/order";

            CString post_data("symbol=");
            post_data.Append( Symbol );

            post_data.Append("&side=");
            post_data.Append( Params["type"] );

            post_data.Append("&type=");
            post_data.Append( "MARKET" );

            post_data.Append("&quantity=");
            post_data.Append( Params["amount"] );

            const CString& price = Params["price"];

            if ( !price.IsEmpty() && price != "0" ) {
                post_data.Append("&price=");
                post_data.Append( price );
            }

            const CString& newClientOrderId = Params["newClientOrderId"];

            if ( !newClientOrderId.IsEmpty() ) {
                post_data.Append("&newClientOrderId=");
                post_data.Append( newClientOrderId );
            }

            const CString& stopPrice = Params["stopPrice"];

            if ( !stopPrice.IsEmpty() && stopPrice != "0.0" ) {
                post_data.Append("&stopPrice=");
                post_data.Append( stopPrice );
            }

            const CString& icebergQty = Params["icebergQty"];

            if ( !icebergQty.IsEmpty() && icebergQty != "0.0" ) {
                post_data.Append("&icebergQty=");
                post_data.Append( icebergQty );
            }

            const CString& recvWindow = Params["recvWindow"];

            if ( !recvWindow.IsEmpty() && recvWindow != "0" ) {
                post_data.Append("&recvWindow=");
                post_data.Append( recvWindow );
            }

            post_data.Append("&timestamp=");
            post_data.Append( to_string( get_current_ms_epoch() ) );

            CString signature = hmac_sha256( SecretKey(), post_data );

            post_data.Append( "&signature=");
            post_data.Append( signature );

            CStringList extra_http_header;

            extra_http_header.Add(CString("X-MBX-APIKEY: "));
            extra_http_header.Last().Append(ApiKey());

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
