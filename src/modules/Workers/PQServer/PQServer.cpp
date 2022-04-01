/*++

Program name:

  Apostol Web Service

Module Name:

  PQServer.cpp

Notices:

  Module: Postgres Query Server

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Core.hpp"
#include "PQServer.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include "jwt.h"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Workers {

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQServer ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CPQServer::CPQServer(CModuleProcess *AProcess) : CApostolModule(AProcess, "application server", "worker/PQServer") {
            m_Headers.Add("Authorization");

            CPQServer::InitMethods();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::InitMethods() {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            m_pMethods->AddObject(_T("GET")    , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoGet(Connection); }));
            m_pMethods->AddObject(_T("POST")   , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoPost(Connection); }));
            m_pMethods->AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoOptions(Connection); }));
            m_pMethods->AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_pMethods->AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
#else
            m_pMethods->AddObject(_T("GET")    , (CObject *) new CMethodHandler(true , std::bind(&CPQServer::DoGet, this, _1)));
            m_pMethods->AddObject(_T("POST")   , (CObject *) new CMethodHandler(true , std::bind(&CPQServer::DoPost, this, _1)));
            m_pMethods->AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true , std::bind(&CPQServer::DoOptions, this, _1)));
            m_pMethods->AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(false, std::bind(&CPQServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, std::bind(&CPQServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, std::bind(&CPQServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, std::bind(&CPQServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, std::bind(&CPQServer::MethodNotAllowed, this, _1)));
            m_pMethods->AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CPQServer::MethodNotAllowed, this, _1)));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {

            auto pResult = APollQuery->Results(0);

            try {
                if (pResult->ExecStatus() != PGRES_TUPLES_OK)
                    throw Delphi::Exception::EDBError(pResult->GetErrorMessage());

                CString errorMessage;

                auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());

                if (pConnection != nullptr && !pConnection->ClosedGracefully()) {
                    auto pReply = pConnection->Reply();

                    CHTTPReply::CStatusType status = CHTTPReply::ok;

                    try {
                        PQResultToJson(pResult, pReply->Content, "array", "result");
                    } catch (Delphi::Exception::Exception &E) {
                        errorMessage = E.what();
                        status = CHTTPReply::bad_request;
                        Log()->Error(APP_LOG_ERR, 0, "%s", E.what());
                    }

                    if (status == CHTTPReply::ok) {
                        pConnection->SendReply(status, nullptr, true);
                    } else {
                        ReplyError(pConnection, status, errorMessage);
                    }
                }
            } catch (Delphi::Exception::Exception &E) {
                QueryException(APollQuery, E);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::QueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {
            auto pConnection = dynamic_cast<CHTTPServerConnection *> (APollQuery->Binding());
            ReplyError(pConnection, CHTTPReply::internal_server_error, E.what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::DoPostgresQueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {
            QueryException(APollQuery, E);
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON CPQServer::ParamsToJson(const CStringList &Params) {
            CJSON Json;
            for (int i = 0; i < Params.Count(); i++) {
                Json.Object().AddPair(Params.Names(i), Params.Values(i));
            }
            return Json;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJSON CPQServer::HeadersToJson(const CHeaders &Headers) {
            CJSON Json;
            for (int i = 0; i < Headers.Count(); i++) {
                const auto &caHeader = Headers[i];
                Json.Object().AddPair(caHeader.Name(), caHeader.Value());
            }
            return Json;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::PQGet(CHTTPServerConnection *AConnection, const CString &Path) {

            CStringList SQL;

            const auto &caHeaders = HeadersToJson(AConnection->Request()->Headers).ToString();
            const auto &caParams = ParamsToJson(AConnection->Request()->Params).ToString();

            SQL.Add(CString()
                            .MaxFormatSize(256 + Path.Size() + caHeaders.Size())
                            .Format("SELECT * FROM http.get(%s, %s::jsonb, %s::jsonb);",
                                    PQQuoteLiteral(Path).c_str(),
                                    PQQuoteLiteral(caHeaders).c_str(),
                                    PQQuoteLiteral(caParams).c_str()
                            ));

            try {
                ExecSQL(SQL, AConnection);
            } catch (Delphi::Exception::Exception &E) {
                AConnection->CloseConnection(true);
                ReplyError(AConnection, CHTTPReply::bad_request, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::PQPost(CHTTPServerConnection *AConnection, const CString &Path, const CString &Body) {

            CStringList SQL;

            const auto &caHeaders = HeadersToJson(AConnection->Request()->Headers).ToString();
            const auto &caParams = ParamsToJson(AConnection->Request()->Params).ToString();
            const auto &caBody = Body.IsEmpty() ? "null" : PQQuoteLiteral(Body);

            SQL.Add(CString()
                            .MaxFormatSize(256 + Path.Size() + caHeaders.Size() + caParams.Size() + caBody.Size())
                            .Format("SELECT * FROM http.post(%s, %s::jsonb, %s::jsonb, %s::jsonb);",
                                    PQQuoteLiteral(Path).c_str(),
                                    PQQuoteLiteral(caHeaders).c_str(),
                                    PQQuoteLiteral(caParams).c_str(),
                                    caBody.c_str()
                            ));

            try {
                ExecSQL(SQL, AConnection);
            } catch (Delphi::Exception::Exception &E) {
                AConnection->CloseConnection(true);
                ReplyError(AConnection, CHTTPReply::bad_request, E.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::DoGet(CHTTPServerConnection *AConnection) {

            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            pReply->ContentType = CHTTPReply::json;

            const auto &path = pRequest->Location.pathname;

            if (path.IsEmpty()) {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            PQGet(AConnection, path);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::DoPost(CHTTPServerConnection *AConnection) {

            auto pRequest = AConnection->Request();
            auto pReply = AConnection->Reply();

            pReply->ContentType = CHTTPReply::json;

            const auto &path = pRequest->Location.pathname;

            if (path.IsEmpty()) {
                AConnection->SendStockReply(CHTTPReply::not_found);
                return;
            }

            const auto& caContentType = pRequest->Headers.Values(_T("Content-Type")).Lower();
            const auto bContentJson = (caContentType.Find(_T("application/json")) != CString::npos);

            CJSON Json;
            if (!bContentJson) {
                ContentToJson(pRequest, Json);
            }

            const auto& caBody = bContentJson ? pRequest->Content : Json.ToString();

            PQPost(AConnection, path, caBody);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CPQServer::Heartbeat() {

        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQServer::Enabled() {
            if (m_ModuleStatus == msUnknown)
                m_ModuleStatus = Config()->IniFile().ReadBool(SectionName(), "enable", true) ? msEnabled : msDisabled;
            return m_ModuleStatus == msEnabled;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CPQServer::CheckLocation(const CLocation &Location) {
            return Location.pathname.SubString(0, 5) == _T("/api/");
        }
        //--------------------------------------------------------------------------------------------------------------
    }
}
}