/*++

Programm name:

  Apostol

Module Name:

  Exchange.cpp

Notices:

  Apostol Web Service (Exchange)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Apostol.hpp"
#include "Exchange.hpp"
//----------------------------------------------------------------------------------------------------------------------

#define BINANCE_HOST    "https://api.binance.com"
#define POLONIEX_HOST   "https://poloniex.com"
#define BITFINEX_HOST   "https://api.bitfinex.com"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Exchange {

        CExchange::CExchange() : CApostolModule() {
            curl_global_init(CURL_GLOBAL_DEFAULT);
            m_curl = curl_easy_init();
            m_Exchanges = new CStringList(true);
            InitExchanges();
        }
        //--------------------------------------------------------------------------------------------------------------

        CExchange::~CExchange() {
            delete m_Exchanges;
            Cleanup();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Cleanup() {
            curl_easy_cleanup(m_curl);
            curl_global_cleanup();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::InitExchanges() {

            CExchangeHandler *Handler = nullptr;
            CString ConfFileName;

            ConfFileName = Config()->ConfPrefix() + "exchange.conf";

            if (FileExists(ConfFileName.c_str())) {

                CIniFile ConfFile(ConfFileName.c_str());

                CStringList Sections;
                CStringList Section;

                ConfFile.ReadSections(&Sections);

                for (int i = 0; i < Sections.Count(); i++) {

                    ConfFile.ReadSectionValues(Sections[i].c_str(), &Section);

                    const CString &uri = Section.Values("uri");
                    const CString &ApiKey = Section.Values("ApiKey");
                    const CString &SecretKey = Section.Values("SecretKey");

                    if (Sections[i].Lower() == "binance") {
                        Handler = new CExchangeHandler(etBinance, uri.IsEmpty() ? BINANCE_HOST : uri, ApiKey, SecretKey, ConfFile.ReadBool(Sections[i].c_str(), "enabled", true));
                        Handler->OnQuoteHandlerEvent(std::bind(&CExchange::BinanceQuoteHandler, this, _1, _2));
                        Handler->OnTradeHandlerEvent(std::bind(&CExchange::BinanceTradeHandler, this, _1, _2));
                    }

                    if (Sections[i].Lower() == "poloniex") {
                        Handler = new CExchangeHandler(etPoloniex, uri.IsEmpty() ? POLONIEX_HOST : uri, ApiKey, SecretKey, ConfFile.ReadBool(Sections[i].c_str(), "enabled", true));
                        Handler->OnQuoteHandlerEvent(std::bind(&CExchange::PoloniexQuoteHandler, this, _1, _2));
                        Handler->OnTradeHandlerEvent(std::bind(&CExchange::PoloniexTradeHandler, this, _1, _2));
                    }

                    if (Sections[i].Lower() == "bitfinex") {
                        Handler = new CExchangeHandler(etBitfinex, uri.IsEmpty() ? BITFINEX_HOST : uri, ApiKey, SecretKey, ConfFile.ReadBool(Sections[i].c_str(), "enabled", true));
                        Handler->OnQuoteHandlerEvent(std::bind(&CExchange::BitfinexQuoteHandler, this, _1, _2));
                        Handler->OnTradeHandlerEvent(std::bind(&CExchange::BitfinexTradeHandler, this, _1, _2));
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        // Curl's callback
        size_t CExchange::curl_cb(void *content, size_t size, size_t nmemb, CString *buffer)
        {
            Log()->Debug(0, "[curl_api] curl_cb");

            buffer->Append((char *) content, size * nmemb);

            Log()->Debug(0, "[curl_api] curl_cb: Done!");

            return size * nmemb;
        }

        // Do the curl
        void CExchange::curl_api_with_header(CString &url, CString &str_result, CStringList &extra_http_header,
                CString &post_data, CString &action) {

            Log()->Debug(0, "[curl_api] curl_api_with_header");

            CURLcode res;

            if( m_curl ) {

                curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CExchange::curl_cb);
                curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &str_result);
                curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);
                curl_easy_setopt(m_curl, CURLOPT_ENCODING, "gzip");

                if ( extra_http_header.Count() > 0 ) {

                    struct curl_slist *chunk = nullptr;
                    for ( int i = 0 ; i < extra_http_header.Count() ;i++ ) {
                        chunk = curl_slist_append(chunk, extra_http_header[i].c_str() );
                    }
                    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, chunk);
                }

                if ( !post_data.IsEmpty() || action == "POST" || action == "PUT" || action == "DELETE" ) {

                    if ( action == "PUT" || action == "DELETE" ) {
                        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, action.c_str() );
                    }
                    curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, post_data.c_str() );
                }

                res = curl_easy_perform(m_curl);

                /* Check for errors */
                if ( res != CURLE_OK ) {
                    Log()->Error(LOG_EMERG, 0, "[curl_api] curl_easy_perform() failed: %s" , curl_easy_strerror(res) ) ;
                }

            }

            Log()->Debug(0, "[curl_api] done" ) ;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::curl_api(CString &url, CString &result_json) {
            CStringList Header;

            CString action("GET");
            CString post_data("");

            curl_api_with_header(url, result_json, Header, post_data, action);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::GetServerTime(CString &exchange, CJSON &json_result) {

            Log()->Debug(0, "[%s] GetServerTime", exchange.c_str());

            CString url;

            if (exchange == "BINANCE") {
                url = BINANCE_HOST;
                url += "/api/v1/time";
            } else {
                throw Delphi::Exception::ExceptionFrm(_T("Unknown exchange: \"%s\""), exchange.c_str());
            }

            CString str_result;
            curl_api(url, str_result);

            if (!str_result.IsEmpty()) {
                try {
                    json_result.Clear();
                    json_result << str_result;
                } catch (Delphi::Exception::Exception &e) {
                    Log()->Error(LOG_EMERG, 0, "[%s] GetServerTime: Error: %s", exchange.c_str(), e.what());
                }
                Log()->Debug(0, "[%s] GetServerTime: Done!", exchange.c_str());

            } else {
                Log()->Error(LOG_EMERG, 0, "[%s] GetServerTime: Failed to get anything.", exchange.c_str());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::GetTradingPairs(CHTTPConnection *AConnection) {
            auto LReply = AConnection->Reply();

            Log()->Debug(0, "GetTradingPairs");

            LReply->Content << R"(["BTC-USDT", "ETH-USDT", "XRP-USDT", "ETH-BTC", "XRP-BTC", "XRP-ETH"])";

            Log()->Debug(0, "Get: %s\n", LReply->Content.c_str());

            AConnection->SendReply(CReply::ok);

            Log()->Debug(0, "GetTradingPairs: Done!");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::BinanceQuoteHandler(const CStringList &Params, CString &Result) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::BinanceTradeHandler(const CStringList &Params, CString &Result) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::PoloniexQuoteHandler(const CStringList &Params, CString &Result) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::PoloniexTradeHandler(const CStringList &Params, CString &Result) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::BitfinexQuoteHandler(const CStringList &Params, CString &Result) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::BitfinexTradeHandler(const CStringList &Params, CString &Result) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Quote(const CStringList& Params, CHTTPConnection *AConnection) {
            auto LReply = AConnection->Reply();

            Log()->Debug(0, "Quote");

            //https://api.binance.com/api/v1/depth?symbol=BTCUSDT&limit=100

            LReply->Content << R"(["BTC-USDT", "ETH-USDT", "XRP-USDT", "ETH-BTC", "XRP-BTC", "XRP-ETH"])";

            Log()->Debug(0, "Get: %s\n", LReply->Content.c_str());

            AConnection->SendReply(CReply::ok);

            Log()->Debug(0, "Quote: Done!");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Trade(const CStringList &Params, CHTTPConnection *AConnection) {
            //CString exchange("BINANCE");
            //GetServerTime(exchange, result);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::ExceptionToJson(Delphi::Exception::Exception *AException, CString &Json) {
            Json.Format(
                    _T("{\"error\": {\"errors\": [{\"domain\": \"%s\", \"reason\": \"%s\", \"message\": "
                       "\"%s\", \"locationType\": \"%s\", \"location\": \"%s\"}], \"code\": %u, "
                       "\"message\": \"%s\"}}"), _T("module"), _T("exception"),
                    AException->what(), _T("cURL"), _T("Exchange"), 500, _T("Internal Server Error"));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Post(CHTTPConnection *AConnection) {
            AConnection->SendStockReply(CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Get(CHTTPConnection *AConnection) {
            auto LRequest = AConnection->Request();
            int LVersion = -1;

            CStringList LUri;
            SplitColumns(LRequest->Uri.c_str(), LRequest->Uri.Size(), &LUri, '/');
            if (LUri.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            if (LUri[1] == _T("v1")) {
                LVersion = 1;
            }

            if (LUri[0] != _T("api") || (LVersion == -1)) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            auto LReply = AConnection->Reply();

            try {

                if (LUri[2] == _T("TradingPairs")) {

                    GetTradingPairs(AConnection);
                    return;

                } else if (LUri[2] == _T("Quote")) {

                    if (LUri.Count() == 6) {
                        CStringList Params;

                        Params.AddPair("pair", LUri[3]);
                        Params.AddPair("type", LUri[4]);
                        Params.AddPair("amount", LUri[5]);

                        Quote(Params, AConnection);

                        return;
                    }

                } else if (LUri[2] == _T("Trade")) {

                    if (LUri.Count() == 7) {
                        CStringList Params;

                        Params.AddPair("pair", LUri[3]);
                        Params.AddPair("type", LUri[4]);
                        Params.AddPair("amount", LUri[5]);
                        Params.AddPair("exchange", LUri[6]);

                        Trade(Params, AConnection);

                        return;
                    }
                }

                AConnection->SendStockReply(CReply::not_found);

            } catch (Delphi::Exception::Exception &E) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(&E, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Execute(CHTTPConnection *AConnection) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->Clear();
            LReply->ContentType = CReply::json;

            if (LRequest->Method == _T("POST")) {
                Post(AConnection);
                return;
            }

            if (LRequest->Method == _T("GET")) {
                Get(AConnection);
                return;
            }

            AConnection->SendStockReply(CReply::not_implemented);
        }

    }

}
}