/*++

Programm name:

  Apostol

Module Name:

  WebServer.cpp

Notices:

  Apostol Web Server

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/
#include "Apostol.hpp"
#include "WebServer.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace WebServer {

        CWebServer::CWebServer(CModuleManager *AManager): CApostolModule(AManager) {
            InitMethods();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::InitMethods() {
            m_Methods.AddObject(_T("GET"), (CObject *) new CMethodHandler(true, std::bind(&CWebServer::DoGet, this, _1)));
            m_Methods.AddObject(_T("OPTIONS"), (CObject *) new CMethodHandler(true, std::bind(&CWebServer::DoOptions, this, _1)));
            m_Methods.AddObject(_T("POST"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("PUT"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("DELETE"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("TRACE"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("HEAD"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("PATCH"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Methods.AddObject(_T("CONNECT"), (CObject *) new CMethodHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::DebugRequest(CRequest *ARequest) {
            DebugMessage("[%p] Request:\n%s %s HTTP/%d.%d\n", ARequest, ARequest->Method.c_str(), ARequest->Uri.c_str(), ARequest->VMajor, ARequest->VMinor);

            for (int i = 0; i < ARequest->Headers.Count(); i++)
                DebugMessage("%s: %s\n", ARequest->Headers[i].Name.c_str(), ARequest->Headers[i].Value.c_str());

            if (!ARequest->Content.IsEmpty())
                DebugMessage("\n%s\n", ARequest->Content.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::DebugReply(CReply *AReply) {
            DebugMessage("[%p] Reply:\nHTTP/%d.%d %d %s\n", AReply, AReply->VMajor, AReply->VMinor, AReply->Status, AReply->StatusText.c_str());

            for (int i = 0; i < AReply->Headers.Count(); i++)
                DebugMessage("%s: %s\n", AReply->Headers[i].Name.c_str(), AReply->Headers[i].Value.c_str());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::DebugConnection(CHTTPServerConnection *AConnection) {
            DebugMessage("\n[%p] [%s:%d] [%d] ", AConnection, AConnection->Socket()->Binding()->PeerIP(),
                         AConnection->Socket()->Binding()->PeerPort(), AConnection->Socket()->Binding()->Handle());

            DebugRequest(AConnection->Request());

            static auto OnReply = [](CObject *Sender) {
                auto LConnection = dynamic_cast<CHTTPServerConnection *> (Sender);

                DebugMessage("\n[%p] [%s:%d] [%d] ", LConnection, LConnection->Socket()->Binding()->PeerIP(),
                             LConnection->Socket()->Binding()->PeerPort(), LConnection->Socket()->Binding()->Handle());

                DebugReply(LConnection->Reply());
            };

            AConnection->OnReply(OnReply);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::DoGet(CHTTPServerConnection *AConnection) {

            auto LServer = dynamic_cast<CHTTPServer *> (AConnection->Server());
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            TCHAR szExt[PATH_MAX] = {0};

            // Decode url to path.
            CString LRequestPath;
            if (!LServer->URLDecode(LRequest->Uri, LRequestPath))
            {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            // Request path must be absolute and not contain "..".
            if (LRequestPath.empty() || LRequestPath.front() != '/' || LRequestPath.find("..") != CString::npos)
            {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            // If path ends in slash (i.e. is a directory) then add "index.html".
            if (LRequestPath.back() == '/')
            {
                LRequestPath += "index.html";
            }

            // Open the file to send back.
            CString LFullPath = LServer->DocRoot() + LRequestPath;
            if (!FileExists(LFullPath.c_str()))
            {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            LReply->Content.LoadFromFile(LFullPath.c_str());

            // Fill out the CReply to be sent to the client.
            AConnection->SendReply(CReply::ok, Mapping::ExtToType(ExtractFileExt(szExt, LRequestPath.c_str())));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::Execute(CHTTPServerConnection *AConnection) {
            int i = 0;

            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();
#ifdef _DEBUG
            DebugConnection(AConnection);
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

        void CWebServer::BeforeExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::AfterExecute(Pointer Data) {

        }
        //--------------------------------------------------------------------------------------------------------------

        bool CWebServer::CheckUserAgent(const CString &Value) {
            return true;
        }

    }
}
}