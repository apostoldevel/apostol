/*++

Library name:

  apostol-core

Module Name:

  Module.cpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "Module.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Module {

        CPQPollQuery *CApostolModule::GetQuery(CPollConnection *AConnection) {
            CPQPollQuery *LQuery = Application::Application->GetQuery(AConnection);

            if (Assigned(LQuery)) {
                LQuery->OnPollExecuted(std::bind(&CApostolModule::DoPostgresQueryExecuted, this, _1));
                LQuery->OnException(std::bind(&CApostolModule::DoPostgresQueryException, this, _1, _2));
            }

            return LQuery;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CApostolModule::CApostolModule(): CApostolModule((CModuleManager *) Application::Application) {

        }
        //--------------------------------------------------------------------------------------------------------------

        CApostolModule::CApostolModule(CModuleManager *AManager): CCollectionItem(AManager), CGlobalComponent() {

        }
        //--------------------------------------------------------------------------------------------------------------

        bool CModuleManager::ExecuteModule(CHTTPServerConnection *AConnection) {

            CApostolModule *LModule = nullptr;

            auto LRequest = AConnection->Request();

            const CString &UserAgent = LRequest->Headers.Values(_T("user-agent"));

            int Index = 0;
            while (Index < ModuleCount() && !Modules(Index)->CheckUrerArent(UserAgent))
                Index++;

            if (Index == ModuleCount()) {
                AConnection->SendStockReply(CReply::forbidden);
                return false;
            }

            LModule = Modules(Index);

            DoBeforeExecuteModule(LModule);
            try {
                LModule->Execute(AConnection);
            } catch (...) {
                AConnection->SendStockReply(CReply::internal_server_error);
                DoAfterExecuteModule(LModule);
                throw;
            }
            DoAfterExecuteModule(LModule);

            return true;
        }
    }
}
}
