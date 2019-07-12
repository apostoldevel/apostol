/*++

Programm name:

  Apostol

Module Name:

  WebServer.cpp

Notices:

  Apostol Web Service (WebServer)

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

        CWebServer::CWebServer(): CApostolModule() {
            m_Headers = new CStringList(true);
            InitHeaders();
        }
        //--------------------------------------------------------------------------------------------------------------

        CWebServer::~CWebServer() {
            delete m_Headers;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::InitHeaders() {
            m_Headers->AddObject(_T("GET"), (CObject *) new CHeaderHandler(true, std::bind(&CWebServer::DoGet, this, _1)));
            m_Headers->AddObject(_T("OPTIONS"), (CObject *) new CHeaderHandler(true, std::bind(&CWebServer::DoOptions, this, _1)));
            m_Headers->AddObject(_T("POST"), (CObject *) new CHeaderHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("PUT"), (CObject *) new CHeaderHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("DELETE"), (CObject *) new CHeaderHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("TRACE"), (CObject *) new CHeaderHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("HEAD"), (CObject *) new CHeaderHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("PATCH"), (CObject *) new CHeaderHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
            m_Headers->AddObject(_T("CONNECT"), (CObject *) new CHeaderHandler(false, std::bind(&CWebServer::MethodNotAllowed, this, _1)));
        }
        //--------------------------------------------------------------------------------------------------------------

        const CString &CWebServer::GetAllowedMethods() {
            if (m_AllowedMethods.IsEmpty()) {
                if (m_Headers->Count() > 0) {
                    CHeaderHandler *Handler;
                    for (int i = 0; i < m_Headers->Count(); ++i) {
                        Handler = (CHeaderHandler *) m_Headers->Objects(i);
                        if (Handler->Allow()) {
                            if (m_AllowedMethods.IsEmpty())
                                m_AllowedMethods = m_Headers->Strings(i);
                            else
                                m_AllowedMethods += _T(", ") + m_Headers->Strings(i);
                        }
                    }
                }
            }
            return m_AllowedMethods;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::MethodNotAllowed(CHTTPConnection *AConnection) {
            auto LReply = AConnection->Reply();

            CReply::GetStockReply(LReply, CReply::not_allowed);

            if (!AllowedMethods().IsEmpty())
                LReply->AddHeader(_T("Allow"), AllowedMethods().c_str());

            AConnection->SendReply();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::DoOptions(CHTTPConnection *AConnection) {

            auto LReply = AConnection->Reply();
#ifdef _DEBUG
            auto LRequest = AConnection->Request();
            if (LRequest->Uri == _T("/quit"))
                Application::Application->SignalProcess()->Quit();
#endif
            CReply::GetStockReply(LReply, CReply::ok);

            if (!AllowedMethods().IsEmpty())
                LReply->AddHeader(_T("Allow"), AllowedMethods().c_str());

            AConnection->SendReply();
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWebServer::DoGet(CHTTPConnection *AConnection) {

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

        void CWebServer::Execute(CHTTPConnection *AConnection) {

            int i = 0;
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->Clear();
            LReply->ContentType = CReply::html;

            CHeaderHandler *Handler;
            for (i = 0; i < m_Headers->Count(); ++i) {
                Handler = (CHeaderHandler *) m_Headers->Objects(i);
                if (Handler->Allow()) {
                    const CString& Method = m_Headers->Strings(i);
                    if (Method == LRequest->Method) {
                        Handler->Handler(AConnection);
                        break;
                    }
                }
            }

            if (i == m_Headers->Count()) {
                AConnection->SendStockReply(CReply::not_implemented);
            }
        }

    }
}
}