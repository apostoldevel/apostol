/*++

Programm name:

  Apostol

Module Name:

  Client365.hpp

Notices:

  Apostol Web Service (Client365)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_CLIENT365_HPP
#define APOSTOL_CLIENT365_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Client365 {

        class CClient365: public CApostolModule {
        private:

            CString m_Session;

        protected:

            void Post(CHTTPConnection *AConnection);
            void Get(CHTTPConnection *AConnection);

            void RowToJson(const CStringList& Row, CString& Json);

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override;
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override;

        public:

            CClient365();

            ~CClient365() override = default;

            static class CClient365 *CreateModule() {
                return new CClient365();
            }

            void Execute(CHTTPConnection *AConnection) override;

            const CString &Session() const { return m_Session; };

        };

    }
}

using namespace Apostol::Client365;
}
#endif //APOSTOL_CLIENT365_HPP
