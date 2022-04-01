/*++

Program name:

  Apostol Web Service

Module Name:

  PQServer.hpp

Notices:

  Module: Postgres Query Server

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_PQSERVER_HPP
#define APOSTOL_PQSERVER_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Workers {

        //--------------------------------------------------------------------------------------------------------------

        //-- CPQServer -------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPQServer: public CApostolModule {
        private:

            static CJSON ParamsToJson(const CStringList &Params);
            static CJSON HeadersToJson(const CHeaders &Headers);

            void InitMethods() override;

            static void QueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E);

        protected:

            void DoGet(CHTTPServerConnection *AConnection) override;
            void DoPost(CHTTPServerConnection *AConnection);

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override;
            void DoPostgresQueryException(CPQPollQuery *APollQuery, const Delphi::Exception::Exception &E) override;

        public:

            explicit CPQServer(CModuleProcess *AProcess);

            ~CPQServer() override = default;

            static class CPQServer *CreateModule(CModuleProcess *AProcess) {
                return new CPQServer(AProcess);
            }

            void PQGet(CHTTPServerConnection *AConnection, const CString &Path);
            void PQPost(CHTTPServerConnection *AConnection, const CString &Path, const CString &Body);

            void Heartbeat() override;

            bool Enabled() override;

            bool CheckLocation(const CLocation &Location) override;

        };
    }
}

using namespace Apostol::Workers;
}
#endif //APOSTOL_PQSERVER_HPP
