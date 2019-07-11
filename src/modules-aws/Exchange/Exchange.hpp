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

#ifndef APOSTOL_EXCHANGE_HPP
#define APOSTOL_EXCHANGE_HPP
//----------------------------------------------------------------------------------------------------------------------

#define SHA256_DIGEST_LENGTH   32
#define SHA384_DIGEST_LENGTH   48
#define SHA512_DIGEST_LENGTH   64
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Exchange {

        enum CExchangeType { etBinance = 0, etPoloniex, etBitfinex };

        CString b2a_hex(const unsigned char *byte_arr, int size);
        //--------------------------------------------------------------------------------------------------------------

        CString hmac_sha256(const CString& key, const CString& data);
        //--------------------------------------------------------------------------------------------------------------

        CString hmac_sha384(const CString& key, const CString& data);
        //--------------------------------------------------------------------------------------------------------------

        CString hmac_sha512(const CString& key, const CString& data);
        //--------------------------------------------------------------------------------------------------------------

        CString to_string(unsigned long Value);
        //--------------------------------------------------------------------------------------------------------------

        unsigned long get_current_ms_epoch( );
        //--------------------------------------------------------------------------------------------------------------

        class CExchangeApi: public CObject, public CGlobalComponent {
        private:

            CURL* m_curl;

            CExchangeType m_Type;

            bool m_Enabled;

            CString m_Name;
            CString m_Uri;

            CString m_ApiKey;
            CString m_SecretKey;

            virtual void PairToSymbol(const CString& Pair, CString& Symbol) abstract;

        protected:

            static size_t curl_cb(void *content, size_t size, size_t nmemb, CString *buffer);
            void curl_api(CString &url, CString &result_json);
            void curl_api_with_header(const CString &url, const CString &str_result,
                    const CStringList &extra_http_header, const CString &post_data, const CString &action);

        public:

            CExchangeApi(CExchangeType Type, const CString &Uri, const CString &ApiKey, const CString &SecretKey,
                             bool Enabled): CObject(), m_Type(Type), m_Enabled(Enabled) {

                const TCHAR *EXCHANGE_NAME[] = {"Binance", "Poloniex", "Bitfinex"};

                m_curl = curl_easy_init();

                m_Name = EXCHANGE_NAME[Type];
                m_Uri = Uri;
                m_ApiKey = ApiKey;
                m_SecretKey = SecretKey;
            };

            ~CExchangeApi() override {
                curl_easy_cleanup(m_curl);
            }

            CExchangeType Type() { return m_Type; };

            bool Enabled() { return m_Enabled; };

            const CString& Name() const { return m_Name; };

            const CString& Uri() const { return m_Uri; };
            const CString& ApiKey() const { return m_ApiKey; };
            const CString& SecretKey() const { return m_SecretKey; };

            virtual void GetServerTime(CJSON &Result) abstract;
            virtual void GetTicker(const CString& Symbol, CJSON &Result) abstract;

            virtual void GetOrderBook(const CStringList &Params, CString &Result) abstract;
            virtual void NewOrder(const CStringList &Params, CString &Result) abstract;
        };
        //--------------------------------------------------------------------------------------------------------------

        struct CAmountPrice {
            CString Exchange;
            double Currency;
            double Asset;
        };

        class CExchange : public CApostolModule {
        private:

            CStringList* m_Exchanges;

            void InitExchanges();

            void Post(CHTTPConnection *AConnection);
            void Get(CHTTPConnection *AConnection);

            bool SaveOrder(CHTTPConnection *AConnection, const CStringList& Params);
            bool UpdateOrder(CHTTPConnection *AConnection, const CStringList& Params, const CString& Response);

            void FindBestPrice(const CStringList& Params, const CStringList& Requests, CString& Result);
            bool CalcBestPrice(const CJSONValue &Data, CAmountPrice &Price);
            bool CalcSplitPrice(const CJSONValue &Data, CAmountPrice &Price, TList<CAmountPrice> &Exchanges);

            void OrderSplit(const CStringList& Params, const CStringList& Requests, CString& Result);

            void ExceptionToJson(Delphi::Exception::Exception *AException, CString& Json);

            static int OrderBookValueCompare(const CJSONValue& Value1, const CJSONValue& Value2);

        protected:

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

        };
    }
}

using namespace Apostol::Exchange;
}

#endif //APOSTOL_EXCHANGE_HPP
