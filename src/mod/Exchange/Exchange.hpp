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

            CString m_Uri;

            CString m_ApiKey;
            CString m_SecretKey;

            COnQuoteHandlerEvent m_QuoteHandler;
            COnTradeHandlerEvent m_TradeHandler;

        public:

            CExchangeHandler(CExchangeType Type, const CString &Uri, const CString &ApiKey, const CString &SecretKey,
                             bool Enabled): CObject(), m_Type(Type), m_Enabled(Enabled) {
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

            const CString& Uri() const { return m_Uri; };
            const CString& ApiKey() const { return m_ApiKey; };
            const CString& SecretKey() const { return m_SecretKey; };

            const COnQuoteHandlerEvent &OnQuoteHandlerEvent() { return m_QuoteHandler; }
            void OnQuoteHandlerEvent(COnQuoteHandlerEvent && Value) { m_QuoteHandler = Value; }

            const COnTradeHandlerEvent &OnTradeHandlerEvent() { return m_TradeHandler; }
            void OnTradeHandlerEvent(COnTradeHandlerEvent && Value) { m_TradeHandler = Value; }
        };
        //--------------------------------------------------------------------------------------------------------------

        class CExchange : public CApostolModule {
        private:

            CURL* m_curl;

            CStringList* m_Exchanges;

            void InitExchanges();

            void Post(CHTTPConnection *AConnection);
            void Get(CHTTPConnection *AConnection);

            static size_t curl_cb(void *content, size_t size, size_t nmemb, CString *buffer);
            void curl_api(CString &url, CString &result_json);
            void curl_api_with_header(CString &url, CString &str_result, CStringList &extra_http_header,
                    CString &post_data, CString &action);

            void ExceptionToJson(Delphi::Exception::Exception *AException, CString& Json);

        protected:

            void GetServerTime(CString &exchange, CJSON &json_result);

            void GetTradingPairs(CHTTPConnection *AConnection);

            void Quote(const CStringList& Params, CHTTPConnection *AConnection);
            void Trade(const CStringList& Params, CHTTPConnection *AConnection);

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

            void BinanceQuoteHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result);
            void BinanceTradeHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result);

            void PoloniexQuoteHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result);
            void PoloniexTradeHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result);

            void BitfinexQuoteHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result);
            void BitfinexTradeHandler(CExchangeHandler *Header, const CStringList& Params, CString &Result);

        };
    }
}

using namespace Apostol::Exchange;
}

#endif //APOSTOL_EXCHANGE_HPP
