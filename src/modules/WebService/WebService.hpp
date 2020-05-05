/*++

Program name:

  Apostol

Module Name:

  WebService.hpp

Notices:

  Module WebService

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_WEBSERVICE_HPP
#define APOSTOL_WEBSERVICE_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace WebService {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWebService -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CWebService: public CApostolModule {
        private:

            int m_Version;

            CStringPairs m_Roots;

            void InitRoots(const CSites &Sites);
            const CString& GetRoot(const CString &Host) const;

            static void ExceptionToJson(int ErrorCode, const std::exception &AException, CString& Json);

            static bool CheckAuthorization(CHTTPServerConnection *AConnection, CAuthorization &Authorization);
            static bool CheckCookies(CHTTPServerConnection *AConnection, CAuthorization &Authorization);

            static int CheckAuthorizedData(CHTTPServerConnection *AConnection, const CLocation &Location,
                                           CAuthorization &Authorization);

            int AuthorizedRequest(CHTTPServerConnection *AConnection, const CLocation &Location,
                                  const CAuthorization &Authorization);

        protected:

            void DoAPI(CHTTPServerConnection *AConnection);

            void DoGet(CHTTPServerConnection *AConnection);
            void DoPost(CHTTPServerConnection *AConnection);

        public:

            explicit CWebService(CModuleManager *AManager);

            ~CWebService() override = default;

            static class CWebService *CreateModule(CModuleManager *AManager) {
                return new CWebService(AManager);
            }

            void InitMethods() override;

            void BeforeExecute(Pointer Data) override;
            void AfterExecute(Pointer Data) override;

            void Heartbeat() override;
            void Execute(CHTTPServerConnection *AConnection) override;

            bool CheckUserAgent(const CString& Value) override;

            static void Redirect(CHTTPServerConnection *AConnection, const CString& Location, bool SendNow = false);

            void SendResource(CHTTPServerConnection *AConnection, const CString &Path, bool SendNow = false);

        };
    }
}

using namespace Apostol::WebService;
}
#endif //APOSTOL_WEBSERVICE_HPP
