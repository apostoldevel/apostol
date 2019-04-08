/*++

Programm name:

  Apostol

Module Name:

  Modules.cpp

Notices:

  Apostol Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Apostol.hpp"
#include "Modules.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Modules {

        CPQPollQuery *CApostolModule::GetQuery(CPollConnection *AConnection) {
            CPQPollQuery *LQuery = nullptr;

            if (Application::Application->PQServer()->Active()) {
                LQuery = Application::Application->PQServer()->GetQuery();

                LQuery->OnPollExecuted(std::bind(&CApostolModule::DoPostgresQueryExecuted, this, _1));
                LQuery->OnException(std::bind(&CApostolModule::DoPostgresQueryException, this, _1, _2));

                LQuery->OnSendQuery(std::bind(&CApplication::DoPQSendQuery, Application::Application, _1));
                LQuery->OnResultStatus(std::bind(&CApplication::DoPQResultStatus, Application::Application, _1));
                LQuery->OnResult(std::bind(&CApplication::DoPQResult, Application::Application, _1, _2));

                LQuery->PollConnection(AConnection);
            }

            return LQuery;
        }
    }
}
}
