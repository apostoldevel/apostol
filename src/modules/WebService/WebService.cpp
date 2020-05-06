/*++

Program name:

  Apostol WebService

Module Name:

  WebService.cpp

Notices:

  Module WebService

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Core.hpp"
#include "WebService.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace WebService {

        CString to_string(unsigned long Value) {
            TCHAR szString[_INT_T_LEN + 1] = {0};
            sprintf(szString, "%lu", Value);
            return CString(szString);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CWebService::CWebService(CModuleManager *AManager): CApostolModule(AManager) {
            m_Version = -1;
            m_Headers.Add("Authorization");
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::InitMethods() {
#if defined(_GLIBCXX_RELEASE) && (_GLIBCXX_RELEASE >= 9)
            m_Methods.AddObject(_T("GET")    , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoGet(Connection); }));
            m_Methods.AddObject(_T("POST")   , (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoPost(Connection); }));
            m_Methods.AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true , [this](auto && Connection) { DoOptions(Connection); }));
            m_Methods.AddObject(_T("PUT")    , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("DELETE") , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("TRACE")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("HEAD")   , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("PATCH")  , (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
            m_Methods.AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, [this](auto && Connection) { MethodNotAllowed(Connection); }));
#else
            m_Methods.AddObject(_T("GET"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoGet, this, _1)));
            m_Methods.AddObject(_T("POST"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoPost, this, _1)));
            m_Methods.AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true, std::bind(&CWebService::DoOptions, this, _1)));
            m_Methods.AddObject(_T("PUT"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("DELETE"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("TRACE"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("HEAD"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("PATCH"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CWebService::MethodNotAllowed, this, _1)));
#endif
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::InitRoots(const CSites &Sites) {
            for (int i = 0; i < Sites.Count(); ++i) {
                const auto& Site = Sites[i];
                if (Site.Name != "default") {
                    const auto& Hosts = Site.Config["hosts"];
                    const auto& Root = Site.Config["root"].AsString();
                    if (!Hosts.IsNull()) {
                        for (int l = 0; l < Hosts.Count(); ++l)
                            m_Roots.AddPair(Hosts[l].AsString(), Root);
                    } else {
                        m_Roots.AddPair(Site.Name, Root);
                    }
                }
            }
            m_Roots.AddPair("*", Sites.Default().Config["root"].AsString());
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CWebService::GetRoot(const CString &Host) const {
            auto Index = m_Roots.IndexOfName(Host);
            if (Index == -1)
                return m_Roots["*"].Value;
            return m_Roots[Index].Value;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json) {
            Json.Format(R"({"error": {"code": %u, "message": "%s"}})", ErrorCode, Delphi::Json::EncodeJsonString(AException.what()).c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Redirect(CHTTPServerConnection *AConnection, const CString& Location, bool SendNow) {
            auto LReply = AConnection->Reply();

            LReply->AddHeader(_T("Location"), Location);
            Log()->Message("Redirected to %s.", Location.c_str());

            AConnection->SendStockReply(CReply::moved_temporarily, SendNow);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::SendResource(CHTTPServerConnection *AConnection, const CString &Path, bool SendNow) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            const CString& LFullPath = GetRoot(LRequest->Location.Host()) + Path;
            const CString& LResource = LFullPath.back() == '/' ? LFullPath + "index.html" : LFullPath;

            if (!FileExists(LResource.c_str())) {
                AConnection->SendStockReply(CReply::not_found, SendNow);
                return;
            }

            TCHAR szFileExt[PATH_MAX] = {0};
            auto fileExt = ExtractFileExt(szFileExt, LResource.c_str());

            LReply->Content.LoadFromFile(LResource.c_str());
            AConnection->SendReply(CReply::ok, Mapping::ExtToType(fileExt), SendNow);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            const auto& LAuthorization = LRequest->Headers.Values(_T("Authorization"));
            if (LAuthorization.IsEmpty())
                return false;

            try {
                Authorization << LAuthorization;

                if (Authorization.Schema == asSession) {
                    LReply->AddHeader(_T("Authorization"), _T("Session ") + Authorization.Session);
                }

                return Authorization.Schema != asUnknown;
            } catch (std::exception &e) {
                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckCookies(CHTTPServerConnection *AConnection, CAuthorization &Authorization) {
            auto LRequest = AConnection->Request();

            const auto& LSession = LRequest->Cookies.Values(_T("AWS-Session"));
            if (LSession.IsEmpty())
                return false;

            try {
                Authorization << _T("Session ") + LSession;
                return Authorization.Schema != asUnknown;
            } catch (std::exception &e) {
                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CWebService::CheckAuthorizedData(CHTTPServerConnection *AConnection, const CLocation &Location,
                                             CAuthorization &Authorization) {

            if (Location.pathname.SubString(0, 6) == "/auth/")
                return -1;

            if (!CheckCookies(AConnection, Authorization) && !CheckAuthorization(AConnection, Authorization)) {
                return 0;
            }

            return 1;
        }
        //--------------------------------------------------------------------------------------------------------------

        int CWebService::AuthorizedRequest(CHTTPServerConnection *AConnection, const CLocation &Location,
                                           const CAuthorization &Authorization) {
            SendResource(AConnection, Location.pathname);
            return -1;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoAPI(CHTTPServerConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->ContentType = CReply::json;

            CStringList LRouts;
            SplitColumns(LRequest->URI, LRouts, '/');

            if (LRouts.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const auto& LService = LRouts[0].Lower();
            const auto& LVersion = LRouts[1].Lower();
            const auto& LCommand = LRouts[2].Lower();

            if (LVersion == "v1") {
                m_Version = 1;
            } else if (LVersion == "v2") {
                m_Version = 2;
            }

            if (LService != "api" || (m_Version == -1)) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            try {
                if (LCommand == "ping") {

                    AConnection->SendStockReply(CReply::ok);

                } else if (LCommand == "time") {

                    LReply->Content << "{\"serverTime\": " << to_string(MsEpoch()) << "}";

                    AConnection->SendReply(CReply::ok);

                } else {

                    AConnection->SendStockReply(CReply::not_found);
                    return;

                }
            } catch (std::exception &e) {
                ExceptionToJson(0, e, LReply->Content);

                AConnection->CloseConnection(true);
                AConnection->SendReply(CReply::bad_request);

                Log()->Error(APP_LOG_EMERG, 0, e.what());
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoGet(CHTTPServerConnection *AConnection) {
            auto LServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto LRequest = AConnection->Request();

            // Decode url to path.
            CString url;
            if (!CHTTPServer::URLDecode(LRequest->URI, url)) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            CLocation Location(url);
            auto& requestPath = Location.pathname;

            // Request path must be absolute and not contain "..".
            if (requestPath.empty() || requestPath.front() != '/' || requestPath.find("..") != CString::npos) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            if (m_Roots.Count() == 0)
                InitRoots(LServer->Sites());

            // If path ends in slash.
            if (requestPath.back() == '/') {
                CAuthorization Authorization;
                const auto auth = CheckAuthorizedData(AConnection, Location, Authorization);
                if (auth == 1) {
                    AuthorizedRequest(AConnection, Location, Authorization);
                    return;
                } else if (auth == 0) {
                    requestPath = "/auth/";
                } else if (auth == -1) {
                    requestPath += "index.html";
                }
            } else if (requestPath.SubString(0, 5) == "/api/") {
                DoAPI(AConnection);
                return;
            }

            SendResource(AConnection, requestPath);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::DoPost(CHTTPServerConnection *AConnection) {
            DoAPI(AConnection);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Heartbeat() {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::Execute(CHTTPServerConnection *AConnection) {
            int i = 0;

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();
#ifdef _DEBUG
            CApostolModule::DebugConnection(AConnection);
#endif
            LReply->Clear();
            LReply->ContentType = CReply::html;

            CMethodHandler *Handler;
            for (i = 0; i < m_Methods.Count(); ++i) {
                Handler = (CMethodHandler *) m_Methods.Objects(i);
                if (Handler->Allow()) {
                    const CString& Method = m_Methods.Strings(i);
                    if (Method == LRequest->Method) {
                        CORS(AConnection);
                        Handler->Handler(AConnection);
                        break;
                    }
                }
            }

            if (i == m_Methods.Count()) {
                AConnection->SendStockReply(CReply::not_implemented);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::BeforeExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebService::AfterExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebService::CheckUserAgent(const CString &Value) {
            return true;
        }

    }
}
}