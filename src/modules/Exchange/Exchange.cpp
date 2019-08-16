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

#include <openssl/hmac.h>
#include "Apostol.hpp"
#include "Exchange.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include "Binance.hpp"
#include "Poloniex.hpp"
#include "Bitfinex.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Exchange {

        CString b2a_hex(const unsigned char *byte_arr, int size) {
            const static CString HexCodes = "0123456789abcdef";
            CString HexString;
            for ( int i = 0; i < size; ++i ) {
                unsigned char BinValue = byte_arr[i];
                HexString += HexCodes[(BinValue >> 4) & 0x0F];
                HexString += HexCodes[BinValue & 0x0F];
            }
            return HexString;
        }
        //--------------------------------------------------------------------------------------------------------------

        CString hmac_sha256(const CString &key, const CString &data) {
            unsigned char* digest;
            digest = HMAC(EVP_sha256(), key.data(), key.length(), (unsigned char *) data.data(), data.length(), nullptr, nullptr);
            return b2a_hex( digest, SHA256_DIGEST_LENGTH );
        }
        //--------------------------------------------------------------------------------------------------------------

        CString hmac_sha384(const CString &key, const CString &data) {
            unsigned char* digest;
            digest = HMAC(EVP_sha384(), key.data(), key.length(), (unsigned char *) data.data(), data.length(), nullptr, nullptr);
            return b2a_hex( digest, SHA384_DIGEST_LENGTH );
        }
        //--------------------------------------------------------------------------------------------------------------

        CString hmac_sha512(const CString &key, const CString &data) {
            unsigned char* digest;
            digest = HMAC(EVP_sha512(), key.data(), key.length(), (unsigned char *) data.data(), data.length(), nullptr, nullptr);
            return b2a_hex( digest, SHA512_DIGEST_LENGTH );
        }
        //--------------------------------------------------------------------------------------------------------------

        CString to_string(unsigned long Value) {
            TCHAR szString[_INT_T_LEN + 1] = {0};
            sprintf(szString, "%lu", Value);
            return CString(szString);
        }
        //--------------------------------------------------------------------------------------------------------------

        unsigned long get_current_ms_epoch() {
            struct timeval tv {0};
            gettimeofday(&tv, nullptr);
            return tv.tv_sec * 1000 + tv.tv_usec / 1000;
        }
        //--------------------------------------------------------------------------------------------------------------

        CExchange::CExchange(CModuleManager *AManager) : CApostolModule(AManager) {
            curl_global_init(CURL_GLOBAL_DEFAULT);
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
            curl_global_cleanup();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::InitExchanges() {

            CExchangeApi *Exchange = nullptr;
            CString ConfFileName;

            ConfFileName = Config()->ConfPrefix() + "exchange.conf";

            if (FileExists(ConfFileName.c_str())) {

                CIniFile ConfFile(ConfFileName.c_str());

                CStringList Sections;
                CStringList Section;

                ConfFile.ReadSections(&Sections);

                for (int i = 0; i < Sections.Count(); i++) {

                    Exchange = nullptr;

                    ConfFile.ReadSectionValues(Sections[i].c_str(), &Section);

                    const CString &Uri = Section.Values("uri");
                    const CString &ApiKey = Section.Values("ApiKey");
                    const CString &SecretKey = Section.Values("SecretKey");

                    if (Sections[i].Lower() == "binance") {
                        Exchange = (CExchangeApi *) new CBinance(Uri, ApiKey, SecretKey, ConfFile.ReadBool(Sections[i].c_str(), "enabled", true));
                    }

                    if (Sections[i].Lower() == "poloniex") {
                        Exchange = (CExchangeApi *) new CPoloniex(Uri, ApiKey, SecretKey, ConfFile.ReadBool(Sections[i].c_str(), "enabled", true));
                    }

                    if (Sections[i].Lower() == "bitfinex") {
                        Exchange = (CExchangeApi *) new CBitfinex(Uri, ApiKey, SecretKey, ConfFile.ReadBool(Sections[i].c_str(), "enabled", true));
                    }

                    if (Assigned(Exchange)) {
                        m_Exchanges->AddObject(Sections[i], (CObject *) Exchange);
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        // Curl's callback
        size_t CExchangeApi::curl_cb(void *content, size_t size, size_t nmemb, CString *buffer)
        {
            Log()->Debug(0, "[curl_api] curl_cb");

            buffer->Append((char *) content, size * nmemb);

            //Log()->Debug(0, "[curl_api] Buffer:\n%s", buffer->c_str());

            Log()->Debug(0, "[curl_api] curl_cb: Done!");

            return size * nmemb;
        }
        //--------------------------------------------------------------------------------------------------------------

        // Do the curl
        void CExchangeApi::curl_api_with_header(const CString &url, const CString &str_result,
                const CStringList &extra_http_header, const CString &post_data, const CString &action) {

            Log()->Debug(0, "[curl_api] curl_api_with_header");

            CURLcode res;

            if ( m_curl ) {

                curl_easy_reset(m_curl);

                curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
                curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, CExchangeApi::curl_cb);
                curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &str_result);
                curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);
                curl_easy_setopt(m_curl, CURLOPT_ENCODING, "gzip");

                if ( extra_http_header.Count() > 0 ) {

                    struct curl_slist *chunk = nullptr;
                    for ( int i = 0; i < extra_http_header.Count(); i++ ) {
                        chunk = curl_slist_append(chunk, extra_http_header[i].c_str());
                    }

                    curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, chunk);
                }

                if ( action == "GET" ) {

                    curl_easy_setopt(m_curl, CURLOPT_HTTPGET, TRUE );

                } else if ( action == "POST" || action == "PUT" || action == "DELETE" ) {

                    if ( action == "PUT" || action == "DELETE" ) {
                        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, action.c_str() );
                    } else {
                        curl_easy_setopt(m_curl, CURLOPT_HTTPPOST, TRUE );

                        if (!post_data.IsEmpty()) {
                            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, post_data.c_str());
                        }
                    }
                }

                clock_t start = clock();

                res = curl_easy_perform(m_curl);

                Log()->Debug(0, "[curl_api] curl_easy_perform runtime: %.2f ms.", (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));

                /* Check for errors */
                if ( res != CURLE_OK ) {
                    Log()->Error(APP_LOG_EMERG, 0, "[curl_api] curl_easy_perform() failed: %s" , curl_easy_strerror(res) );
                }

            }

            Log()->Debug(0, "[curl_api] curl_api_with_header: Done!" ) ;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchangeApi::curl_api(CString &url, CString &result_json) {
            curl_api_with_header(url, result_json, CStringList(), CString(), "GET");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::GetTradingPairs(CHTTPServerConnection *AConnection) {
            auto LReply = AConnection->Reply();

            Log()->Debug(0, "GetTradingPairs");

            LReply->Content << R"(["BTC-USDT", "ETH-USDT", "XRP-USDT", "ETH-BTC", "XRP-BTC", "XRP-ETH"])";

            Log()->Debug(0, "Result: %s\n", LReply->Content.c_str());

            AConnection->SendReply(CReply::ok);

            Log()->Debug(0, "GetTradingPairs: Done!");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Quote(const CStringList& Params, CHTTPServerConnection *AConnection) {

            Log()->Debug(0, "Quote");

            auto LReply = AConnection->Reply();

            CStringList Results;
            CExchangeApi *Exchange;

            for (int i = 0; i < m_Exchanges->Count(); i++) {
                Exchange = (CExchangeApi *) m_Exchanges->Objects(i);
                if (Exchange->Enabled()) {
                    Results.AddObject(CString(), (CObject *) Exchange);
                    Exchange->GetOrderBook(Params, Results[i]);
                }
            }

            FindBestPrice(Params, Results, LReply->Content);

            AConnection->SendReply(CReply::ok);

            Log()->Debug(0, "[Quote] Content size: %d", LReply->Content.size());
            Log()->Debug(0, "[Quote] Done!");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Trade(const CStringList &Params, CHTTPServerConnection *AConnection) {

            Log()->Debug(0, "[Trade] Start");

            auto LReply = AConnection->Reply();

            CExchangeApi *Exchange;
            for (int i = 0; i < m_Exchanges->Count(); i++) {

                Exchange = (CExchangeApi *) m_Exchanges->Objects(i);

                if (Exchange->Enabled() && (Params["exchange"].Lower() == Exchange->Name().Lower())) {
                    SaveOrder(AConnection, Params);
                    Exchange->NewOrder(Params, LReply->Content);
                    UpdateOrder(AConnection, Params, LReply->Content);
                }
            }

            if (!LReply->Content.IsEmpty()) {
                Log()->Debug(0, "[Trade] Content size: %d", LReply->Content.size());
            }

            AConnection->SendReply(CReply::ok);

            Log()->Debug(0, "[Trade] Done!");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Split(const CStringList &Params, CHTTPServerConnection *AConnection) {

            Log()->Debug(0, "Split");

            auto LReply = AConnection->Reply();

            CStringList Results;
            CExchangeApi *Exchange;

            for (int i = 0; i < m_Exchanges->Count(); i++) {
                Exchange = (CExchangeApi *) m_Exchanges->Objects(i);
                if (Exchange->Enabled()) {
                    Results.AddObject(CString(), (CObject *) Exchange);
                    Exchange->GetOrderBook(Params, Results[i]);
                }
            }

            OrderSplit(Params, Results, LReply->Content);

            AConnection->SendReply(CReply::ok);

            Log()->Debug(0, "[Split] Content size: %d", LReply->Content.size());
            Log()->Debug(0, "[Split] Done!");
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CExchange::CalcBestPrice(const CJSONValue &Data, CAmountPrice &Price) {

            DebugMessage("Exchange: %s\n", Price.Exchange.c_str());
            DebugMessage("Currency: %f, Asset: %f\n", Price.Currency, Price.Asset);

            for (int j = 0; j < Data.Count(); j++) {

                const CJSONValue &Next = Data[j];

                if (Price.Asset > Next[1].AsDouble()) {

                    Price.Asset -= Next[1].AsDouble();
                    Price.Currency += Next[0].AsDouble() * Next[1].AsDouble();

                    DebugMessage("[C] Price: %.2f (%s), Amount: %.8f (%s)\n", Next[0].AsDouble(),
                                 Next[0].AsSiring().c_str(), Next[1].AsDouble(), Next[1].AsSiring().c_str());
                    DebugMessage("[C] Currency: %.4f, Asset: %.4f\n", Price.Currency, Price.Asset);

                } else {

                    Price.Currency += Next[0].AsDouble() * Price.Asset;
                    Price.Asset = 0;

                    DebugMessage("[B] Price: %.2f (%s), Amount: %.8f (%s)\n", Next[0].AsDouble(),
                                 Next[0].AsSiring().c_str(), Next[1].AsDouble(), Next[1].AsSiring().c_str());
                    DebugMessage("[B] Currency: %.4f, Asset: %.4f\n", Price.Currency, Price.Asset);

                    break;
                }
            }

            return Price.Currency != 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CExchange::CalcSplitPrice(const CJSONValue &Data, CAmountPrice &Price, TList<CAmountPrice> &Exchanges) {

            DebugMessage("Exchange: %s\n", Price.Exchange.c_str());
            DebugMessage("Currency: %f, Asset: %f\n", Price.Currency, Price.Asset);

            int Index;

            for (int i = 0; i < Data.Count(); i++) {

                const CJSONValue &Next = Data[i];

                Index = 0;
                while ((Index < Exchanges.Count()) && (Exchanges[Index].Exchange != Next[2].AsSiring()))
                    Index++;

                if (Index == Exchanges.Count())
                    continue;

                if (Price.Asset > Next[1].AsDouble()) {

                    Price.Asset -= Next[1].AsDouble();
                    Price.Currency += Next[0].AsDouble() * Next[1].AsDouble();

                    Exchanges[Index].Asset -= Next[1].AsDouble();
                    Exchanges[Index].Currency += Next[0].AsDouble() * Next[1].AsDouble();

                    DebugMessage("[R] Exchange: %s, Price: %.2f (%s), Amount: %.8f (%s)\n", Next[2].AsSiring().c_str(),
                            Next[0].AsDouble(), Next[0].AsSiring().c_str(), Next[1].AsDouble(), Next[1].AsSiring().c_str());

                    DebugMessage("[D] Exchange: %s, Currency: %.4f, Asset: %.4f\n", Exchanges[Index].Exchange.c_str(),
                            Exchanges[Index].Currency, Exchanges[Index].Asset);
                    DebugMessage("[A] Exchange: %s, Currency: %.4f, Asset: %.4f\n", "All", Price.Currency, Price.Asset);

                } else {

                    Exchanges[Index].Asset -= Price.Asset;
                    Exchanges[Index].Currency += Next[0].AsDouble() * Price.Asset;

                    Price.Currency += Next[0].AsDouble() * Price.Asset;
                    Price.Asset = 0;

                    DebugMessage("[Q] Exchange: %s, Price: %.2f (%s), Amount: %.8f (%s)\n", Next[2].AsSiring().c_str(),
                                 Next[0].AsDouble(), Next[0].AsSiring().c_str(), Next[1].AsDouble(), Next[1].AsSiring().c_str());

                    DebugMessage("[Q] Exchange: %s, Currency: %.4f, Asset: %.4f\n", Exchanges[Index].Exchange.c_str(),
                                 Exchanges[Index].Currency, Exchanges[Index].Asset);
                    DebugMessage("[Q] Exchange: %s, Currency: %.4f, Asset: %.4f\n", "All", Price.Currency, Price.Asset);

                    break;
                }
            }

            return Price.Currency != 0;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::FindBestPrice(const CStringList &Params, const CStringList &Requests, CString &Result) {

            const CString &type = Params.Values("type");
            const CString &amount = Params.Values("amount");

            double Amount = StrToDouble(amount.c_str());

            CString AsksBids;
            if (type == "BUY") {
                AsksBids = "asks";
            } else if (type == "SELL") {
                AsksBids = "bids";
            }

            CExchangeApi *Exchange;
            TList<CJSON> Json;

            int Count = 0;

            Result = "[";

            for (int i = 0; i < Requests.Count(); i++) {

                // Convert string JSON to object JSON
                Json.Add(CJSON(Requests[i]));

                // Get JSON array "asks" or "bids"
                const CJSONValue &Data = Json[i][AsksBids];

                if (Data.IsArray()) {
                    Exchange = (CExchangeApi *) Requests.Objects(i);

                    CAmountPrice Price = { Exchange->Name().Lower(), 0, Amount };

                    if (CalcBestPrice(Data, Price)) {

                        if (Count > 0) {
                            Result += ", ";
                        }

                        Result.Format(
                                R"({"asks_bids": "%s", "AveragePrice": %.4f, "TotalPrice": %.4f, "Exchange": "%s", "Remain": %.2f})",
                                AsksBids.c_str(),
                                Price.Currency / (Amount - Price.Asset),
                                Price.Currency,
                                Exchange->Name().c_str(),
                                Price.Asset);

                        Count++;
                    }
                }
            }

            Result += "]";
        }
        //--------------------------------------------------------------------------------------------------------------

        int CExchange::OrderBookValueCompare(const CJSONValue &Value1, const CJSONValue &Value2) {

            double V1 = Value1[0].AsDouble();
            double V2 = Value2[0].AsDouble();

            if (V1 == V2)
                return 0;
            if (V1 > V2)
                return 1;
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::OrderSplit(const CStringList &Params, const CStringList &Requests, CString &Result) {

            const CString &pair = Params.Values("pair");
            const CString &type = Params.Values("type");
            const CString &amount = Params.Values("amount");

            double Amount = StrToDouble(amount.c_str());

            CString AsksBids;
            if (type == "BUY") {
                AsksBids = "asks";
            } else if (type == "SELL") {
                AsksBids = "bids";
            }

            CExchangeApi *Exchange;
            CString ExchangeName;

            CJSONValue Split;

            TList<CJSON> Json;
            TList<CAmountPrice> PriceArray;

            CJSON Root(jvtObject);

            Root.Object().AddPair("filled_amount", amount);
            Root.Object().AddPair("filled_percent", amount);
            Root.Object().AddPair("operation_code", "OrderFilled");

            CJSONValue orderSpec(jvtObject);

            orderSpec.Object().AddPair("amount", amount);
            orderSpec.Object().AddPair("currency_pair", pair);
            orderSpec.Object().AddPair("order_type", "market");
            orderSpec.Object().AddPair("price", "0");
            orderSpec.Object().AddPair("side", type.Lower());

            Root.Object().AddPair("original_order_spec", orderSpec);

            CJSONValue routedOrders(jvtObject);
            CJSONValue singleMatches(jvtObject);

            for (int i = 0; i < Requests.Count(); i++) {

                // Convert string JSON to object JSON
                Json.Add(CJSON(Requests[i]));

                // Get JSON array "asks" or "bids"
                const CJSONValue &Data = Json[i][AsksBids];

                if (Data.IsArray()) {
                    Exchange = (CExchangeApi *) Requests.Objects(i);
                    ExchangeName = Exchange->Name().Lower();

                    PriceArray.Add({ ExchangeName, 0, Amount });

                    for (int j = 0; j < Data.Count(); j++) {
                        if (Data[j].IsArray()) {
                            Split.Array().Add(Data[j]);
                            Split.Array().Last().Array().Add(ExchangeName);
                        } else if (Data[j].IsObject()) {
                            CJSONValue V(jvtArray);
                            V.Array().Add(Data[j][0]);
                            V.Array().Add(Data[j][1]);
                            V.Array().Add(ExchangeName);
                            Split.Array().Add(V);
                        }
                    }

                    CAmountPrice Price = { ExchangeName, 0, Amount };

                    if (CalcBestPrice(Data, Price)) {

                        double Balance = Amount - Price.Asset;

                        CJSONValue singleMatch(jvtObject);

                        singleMatch.Object().AddPair("avg_price", Price.Currency / Balance);
                        singleMatch.Object().AddPair("filled_amount", Balance);
                        singleMatch.Object().AddPair("filled_percent", Balance / Amount * 100);

                        singleMatches.Object().AddPair("demo-" + ExchangeName, singleMatch);
                    }
                }
            }

            Split.ToString(Result);
            DebugMessage("[D] [O] %s\n", Result.c_str());
            Result.Clear();

            Split.Array().Sort(OrderBookValueCompare);

            Split.ToString(Result);
            DebugMessage("[D] [S] %s\n", Result.c_str());
            Result.Clear();

            double averagePrice = 0;
            double Total = 0;

            CAmountPrice Price = { "All", 0, Amount };

            if (CalcSplitPrice(Split, Price, PriceArray)) {

                for (int i = 0; i < PriceArray.Count(); i++) {

                    if (PriceArray[i].Currency == 0)
                        continue;

                    double resulting_average_price = 0;
                    double resulting_total = 0;

                    double Balance = Amount - PriceArray[i].Asset;

                    if (Balance != 0) {
                        resulting_average_price = PriceArray[i].Currency / Balance;
                        resulting_total = PriceArray[i].Currency;
                    }

                    CJSONValue singleOrder(jvtObject);

                    singleOrder.Object().AddPair("amount", Balance);
                    singleOrder.Object().AddPair("currency_pair", pair);
                    singleOrder.Object().AddPair("filled_amount", Balance);
                    singleOrder.Object().AddPair("filled_percent", Balance / Amount * 100);
                    singleOrder.Object().AddPair("market_name", "demo-" + PriceArray[i].Exchange);
                    singleOrder.Object().AddPair("order_type", "market");
                    singleOrder.Object().AddPair("price", "0");
                    singleOrder.Object().AddPair("resulting_average_price", resulting_average_price);
                    singleOrder.Object().AddPair("resulting_total", resulting_total);
                    singleOrder.Object().AddPair("side", type.Lower());

                    routedOrders.Object().AddPair("demo-" + PriceArray[i].Exchange, singleOrder);
                }

                double Balance = Amount - Price.Asset;

                if (Balance != 0) {
                    Total = Price.Currency;
                    averagePrice = Total / Balance;
                }
            }

            Root.Object().AddPair("resulting_average_price", averagePrice);
            Root.Object().AddPair("resulting_total", Total);
            Root.Object().AddPair("routed_orders", routedOrders);

            Root.Object().AddPair("single_market_matches", singleMatches);

            Root.ToString(Result);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::ExceptionToJson(Delphi::Exception::Exception *AException, CString &Json) {

            LPCTSTR lpMessage = AException->what();
            CString Message;
            TCHAR ch = 0;

            while (*lpMessage) {
                ch = *lpMessage++;
                if ((ch == '"') || (ch == '\\'))
                    Message.Append('\\');
                Message.Append(ch);
            }

            Json.Format(R"({"Error": "%s"})", Message.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Post(CHTTPServerConnection *AConnection) {

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

                if (LUri[2] == _T("Trade")) {

                    if (LRequest->Content.IsEmpty()) {
                        AConnection->SendStockReply(CReply::no_content);
                        return;
                    }

                    CJSON Json;
                    CStringList Params;

                    Json << LRequest->Content;

                    Params.AddPair("async_key", to_string(get_current_ms_epoch()));

                    Params.AddPair("pair", Json["pair"].AsSiring());
                    Params.AddPair("type", Json["type"].AsSiring());
                    Params.AddPair("amount", Json["amount"].AsSiring());
                    Params.AddPair("exchange", Json["exchange"].AsSiring());

                    // Poloniex default value
                    Params.AddPair("immediateOrCancel", "1");

                    Trade(Params, AConnection);

                    return;

                }

                AConnection->SendStockReply(CReply::not_found);

            } catch (Delphi::Exception::Exception &E) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(&E, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Get(CHTTPServerConnection *AConnection) {

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
                        Params.AddPair("limit", "1000");

                        Quote(Params, AConnection);

                        return;
                    }

                } else if (LUri[2] == _T("Split")) {

                    if (LUri.Count() == 6) {
                        CStringList Params;

                        Params.AddPair("pair", LUri[3]);
                        Params.AddPair("type", LUri[4]);
                        Params.AddPair("amount", LUri[5]);
                        Params.AddPair("limit", "1000");

                        Split(Params, AConnection);

                        return;
                    }
                }

                AConnection->SendStockReply(CReply::not_found);

            } catch (Delphi::Exception::Exception &E) {
                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(&E, LReply->Content);

                AConnection->SendReply(LStatus);
                Log()->Error(APP_LOG_EMERG, 0, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) {
            Log()->Error(APP_LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CExchange::SaveOrder(CHTTPServerConnection *AConnection, const CStringList &Params) {
            auto LQuery = GetQuery(AConnection);

            if (LQuery == nullptr) {
                Log()->Debug(0, "[Trade] Save order error: GetQuery() is null");
                return false;
            }

            LQuery->SQL().Add(CString());
            LQuery->SQL().Last().Format(R"(SELECT * FROM NewOrder(%s, '%s', '%s', '%s', %s);)",
                    Params["async_key"].c_str(),
                    Params["exchange"].c_str(),
                    Params["pair"].c_str(),
                    Params["type"].c_str(),
                    Params["amount"].c_str()
            );

            if (LQuery->QueryStart() != POLL_QUERY_START_ERROR) {
                return true;
            } else {
                delete LQuery;
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CExchange::UpdateOrder(CHTTPServerConnection *AConnection, const CStringList &Params, const CString &Response) {
            auto LQuery = GetQuery(AConnection);

            if (LQuery == nullptr) {
                Log()->Debug(0, "[Trade] Update order error: GetQuery() is null");
                return false;
            }

            LQuery->SQL().Add(CString());
            LQuery->SQL().Last().Format(R"(UPDATE orders SET response = '%s'::jsonb WHERE async_key = %s;)",
                                        Response.c_str(),
                                        Params["async_key"].c_str()
            );

            if (LQuery->QueryStart() != POLL_QUERY_START_ERROR) {
                return true;
            } else {
                delete LQuery;
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CExchange::Execute(CHTTPServerConnection *AConnection) {

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->Clear();
            LReply->ContentType = CReply::json;
            LReply->AddHeader("Access-Control-Allow-Origin", "*");

            if (LRequest->Method == _T("GET")) {
                Get(AConnection);
                return;
            }

            if (LRequest->Method == _T("POST")) {
                Post(AConnection);
                return;
            }

            AConnection->SendStockReply(CReply::not_implemented);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CExchange::CheckUrerArent(const CString &Value) {
            if (!Value.IsEmpty()) {
                return (Value.Find(_T("Exchange/")) != CString::npos);
            }
            return false;
        }

    }
}
}