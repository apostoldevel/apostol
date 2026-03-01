/*++

Program name:

  Apostol Web Service

Module Name:

  WebServer.cpp

Notices:

  Module: Web Server (Benchmark)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Core.hpp"
#include "WebServer.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Module {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebServer ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWebServer::CWebServer(CModuleProcess *AProcess) : CApostolModule(AProcess, "web server", "module/WebServer") {
            m_Headers.Add("Authorization");
            m_KeepAlive = true;

            CWebServer::InitMethods();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::InitMethods() {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            m_Methods.AddObject(_T("GET")    , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoGet(Connection); }));
            m_Methods.AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoOptions(Connection); }));
            m_Methods.AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoHead(Connection); }));
            m_Methods.AddObject(_T("POST")   , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
#else
            m_Methods.AddObject(_T("GET")    , (CObject *) new CMethodHandler(true, std::bind(&CWebServer::DoGet, this, _1)));
            m_Methods.AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true, std::bind(&CWebServer::DoOptions, this, _1)));
            m_Methods.AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(true, std::bind(&CWebServer::DoHead, this, _1)));
            m_Methods.AddObject(_T("POST")   , (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::DoGet(CHTTPServerConnection *AConnection) {

            const auto &caRequest = AConnection->Request();
            auto &Reply = AConnection->Reply();

            CString sPath(caRequest.Location.pathname);

            // Request path must be absolute and not contain "..".
            if (sPath.empty() || sPath.front() != '/' || sPath.find(_T("..")) != CString::npos) {
                AConnection->SendStockReply(CHTTPReply::bad_request);
                return;
            }

            const bool close_connection = !m_KeepAlive || caRequest.Headers["Connection"] == "close";

            // --- Static endpoints ---

            if (sPath == "/api/v1/ping") {
                Reply.ContentType = CHTTPReply::json;
                Reply.Content = "{\"ok\":true,\"message\":\"OK\"}";
                Reply.CloseConnection = close_connection;
                AConnection->CloseConnection(close_connection);

                AConnection->SendReply(CHTTPReply::ok, nullptr, true);
                return;
            }

            if (sPath == "/api/v1/time") {
                Reply.ContentType = CHTTPReply::json;
                Reply.Content << "{\"serverTime\":" << CString::ToString((long) time(nullptr)) << "}";
                Reply.CloseConnection = close_connection;
                AConnection->CloseConnection(close_connection);

                AConnection->SendReply(CHTTPReply::ok, nullptr, true);
                return;
            }

            // --- Database endpoints ---

            if (sPath == "/api/v1/db/ping") {
                CStringList SQL;
                SQL.Add("SELECT json_build_object('ok', true, 'message', 'OK')::text AS json;");

                auto OnExecuted = [close_connection](CPQPollQuery *APollQuery) {
                    auto pConnection = dynamic_cast<CHTTPServerConnection *>(APollQuery->Binding());
                    if (pConnection != nullptr && pConnection->Connected()) {
                        auto &Reply = pConnection->Reply();
                        Reply.ContentType = CHTTPReply::json;
                        const auto pResult = APollQuery->Results(0);
                        if (pResult->ExecStatus() == PGRES_TUPLES_OK && pResult->nTuples() > 0)
                            Reply.Content = pResult->GetValue(0, 0);
                        Reply.CloseConnection = close_connection;
                        pConnection->CloseConnection(close_connection);
                        pConnection->SendReply(CHTTPReply::ok, nullptr, true);
                    }
                };

                auto OnException = [](CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {
                    auto pConnection = dynamic_cast<CHTTPServerConnection *>(APollQuery->Binding());
                    if (pConnection != nullptr && pConnection->Connected())
                        pConnection->SendStockReply(CHTTPReply::internal_server_error, true);
                };

                ExecSQL(SQL, AConnection, std::move(OnExecuted), std::move(OnException));
                return;
            }

            if (sPath == "/api/v1/db/time") {
                CStringList SQL;
                SQL.Add("SELECT json_build_object('serverTime', extract(epoch from now())::integer)::text AS json;");

                auto OnExecuted = [close_connection](CPQPollQuery *APollQuery) {
                    auto pConnection = dynamic_cast<CHTTPServerConnection *>(APollQuery->Binding());
                    if (pConnection != nullptr && pConnection->Connected()) {
                        auto &Reply = pConnection->Reply();
                        Reply.ContentType = CHTTPReply::json;
                        const auto pResult = APollQuery->Results(0);
                        if (pResult->ExecStatus() == PGRES_TUPLES_OK && pResult->nTuples() > 0)
                            Reply.Content = pResult->GetValue(0, 0);
                        Reply.CloseConnection = close_connection;
                        pConnection->CloseConnection(close_connection);
                        pConnection->SendReply(CHTTPReply::ok, nullptr, true);
                    }
                };

                auto OnException = [](CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) {
                    auto pConnection = dynamic_cast<CHTTPServerConnection *>(APollQuery->Binding());
                    if (pConnection != nullptr && pConnection->Connected())
                        pConnection->SendStockReply(CHTTPReply::internal_server_error, true);
                };

                ExecSQL(SQL, AConnection, std::move(OnExecuted), std::move(OnException));
                return;
            }

            // --- Default: serve static files ---

            CStringList TryFiles;
            TryFiles.Add("/index.html");
            SendResource(AConnection, sPath, nullptr, false, TryFiles);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebServer::Enabled() {
            if (m_ModuleStatus == msUnknown) {
                m_ModuleStatus = Config()->IniFile().ReadBool(SectionName().c_str(), "enable", true) ? msEnabled : msDisabled;
                m_KeepAlive = Config()->IniFile().ReadBool(SectionName().c_str(), "keepalive", true);
            }
            return m_ModuleStatus == msEnabled;
        }
        //--------------------------------------------------------------------------------------------------------------
    }
}
}
