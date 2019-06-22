/*++

Programm name:

  Apostol

Module Name:

  Exchange.hpp

Notices:

  Apostol Web Service (Exchange)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include <curl/curl.h>
#include "base64.hpp"
//----------------------------------------------------------------------------------------------------------------------

#ifndef APOSTOL_EXCHANGE_HPP
#define APOSTOL_EXCHANGE_HPP

extern "C++" {

namespace Apostol {

    namespace Exchange {

        class CExchangeHandler;

        enum CExchangeType { etBinance = 0, etPoloniex, etBitfinex };

        typedef std::function<void (CExchangeHandler *Header, const CStringList& Params, CString &Result)> COnQuoteHandlerEvent;
        typedef std::function<void (CExchangeHandler *Header, const CStringList& Params, CString &Result)> COnTradeHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CExchangeHandler: CObject {
        private:

            CExchangeType m_Type;

            bool m_Enabled;

            CString m_Name;
            CString m_Uri;

            CString m_ApiKey;
            CString m_SecretKey;

            COnQuoteHandlerEvent m_QuoteHandler;
            COnTradeHandlerEvent m_TradeHandler;

        public:

            CExchangeHandler(CExchangeType Type, const CString &Uri, const CString &ApiKey, const CString &SecretKey,
                             bool Enabled): CObject(), m_Type(Type), m_Enabled(Enabled) {

                const TCHAR *EXCHANGE_NAME[] = {"Binance", "Poloniex", "Bitfinex"};

                m_Name = EXCHANGE_NAME[Type];
                m_Uri = Uri;
                m_ApiKey = ApiKey;
                m_SecretKey = SecretKey;
            };

            CExchangeType Type() { return m_Type; };

            bool Enabled() { return m_Enabled; };

            void QuoteHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result) {
                if (m_Enabled && m_QuoteHandler)
                    m_QuoteHandler(Header, Params, Result);
            }

            void TradeHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result) {
                if (m_Enabled && m_TradeHandler)
                    m_TradeHandler(Header, Params, Result);
            }

            const CString& Name() const { return m_Name; };

            const CString& Uri() const { return m_Uri; };
            const CString& ApiKey() const { return m_ApiKey; };
            const CString& SecretKey() const { return m_SecretKey; };

            const COnQuoteHandlerEvent &OnQuoteHandlerEvent() { return m_QuoteHandler; }
            void OnQuoteHandlerEvent(COnQuoteHandlerEvent && Value) { m_QuoteHandler = Value; }

            const COnTradeHandlerEvent &OnTradeHandlerEvent() { return m_TradeHandler; }
            void OnTradeHandlerEvent(COnTradeHandlerEvent && Value) { m_TradeHandler = Value; }
        };
        //--------------------------------------------------------------------------------------------------------------

        struct CAmountPrice {
            CString Exchange;
            double Currency;
            double Asset;
        };

        class CExchange : public CApostolModule {
        private:

            CURL* m_curl;

            CStringList* m_Exchanges;

            void InitExchanges();

            void Post(CHTTPConnection *AConnection);
            void Get(CHTTPConnection *AConnection);

            bool SaveOrder(CHTTPConnection *AConnection, const CStringList& Params);
            bool UpdateOrder(CHTTPConnection *AConnection, const CStringList& Params, const CString& Response);

            static size_t curl_cb(void *content, size_t size, size_t nmemb, CString *buffer);
            void curl_api(CString &url, CString &result_json);
            void curl_api_with_header(const CString &url, const CString &str_result,
                 const CStringList &extra_http_header, const CString &post_data, const CString &action);

            void FindBestPrice(const CStringList& Params, const CStringList& Requests, CString& Result);
            bool CalcBestPrice(const CJSONValue &Data, CAmountPrice &Amount);
            bool CalcSplitPrice(const CJSONValue &Data, double Amount, TList<CAmountPrice> &Price);

            void OrderSplit(const CStringList& Params, const CStringList& Requests, CString& Result);

            void ExceptionToJson(Delphi::Exception::Exception *AException, CString& Json);

            static int OrderBookValueCompare(const CJSONValue& Value1, const CJSONValue& Value2);

        protected:

            void GetServerTime(CExchangeHandler *Exchange, CJSON &json_result);

            void GetTicker(CExchangeHandler *Exchange, const CString& Symbol, CJSON &Result);

            void GetTradingPairs(CHTTPConnection *AConnection);

            void Quote(const CStringList& Params, CHTTPConnection *AConnection);
            void Trade(const CStringList& Params, CHTTPConnection *AConnection);
            void Split(const CStringList& Params, CHTTPConnection *AConnection);

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override;

            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override;

        public:

            CExchange();

            ~CExchange() override;

            static class CExchange *CreateModule() {
                return new CExchange();
            }

            void Execute(CHTTPConnection *AConnection) override;

            void Cleanup();

            void BinanceQuoteHandler(CExchangeHandler *Exchange, const CStringList& Params, CString &Result);
            void BinanceTradeHandler(CExchangeHandler *Exchange, const CStringList& Params, CString &Result);

            void PoloniexQuoteHandler(CExchangeHandler *Exchange, const CStringList& Params, CString &Result);
            void PoloniexTradeHandler(CExchangeHandler *Exchange, const CStringList& Params, CString &Result);

            void BitfinexQuoteHandler(CExchangeHandler *Exchange, const CStringList& Params, CString &Result);
            void BitfinexTradeHandler(CExchangeHandler *Exchange, const CStringList& Params, CString &Result);

        };
    }
}

using namespace Apostol::Exchange;
}

#endif //APOSTOL_EXCHANGE_HPP
