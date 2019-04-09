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

        typedef TList<TList<CStringList>> CQueryResult;
        //--------------------------------------------------------------------------------------------------------------

        class CJob: CCollectionItem {
        private:

            CString m_JobId;

            CString m_Result;

            CPQPollQuery *m_PollQuery;

        public:

            explicit CJob(CCollection *ACCollection);

            ~CJob() override = default;

            CString& JobId() { return m_JobId; };
            const CString& JobId() const { return m_JobId; };

            CString& Result() { return m_Result; }
            const CString& Result() const { return m_Result; }

            CPQPollQuery *PollQuery() { return m_PollQuery; };
            void PollQuery(CPQPollQuery *Value) { m_PollQuery = Value; };
        };
        //--------------------------------------------------------------------------------------------------------------

        class CJobManager: CCollection {
            typedef CCollection inherited;
        private:

            CJob *Get(int Index);
            void Set(int Index, CJob *Value);

        public:

            CJobManager(): CCollection(this) {

            }

            CJob *Add(CPQPollQuery *Query);

            CJob *FindJobById(const CString &Id);
            CJob *FindJobByQuery(CPQPollQuery *Query);

            CJob *Jobs(int Index) { return Get(Index); }
            void Jobs(int Index, CJob *Value) { Set(Index, Value); }

        };
        //--------------------------------------------------------------------------------------------------------------

        class CClient365: public CApostolModule {
        private:

            CString m_Session;

            CJobManager *m_Jobs;

            void InitResult(CPQPollQuery *APollQuery, CQueryResult& AResult);

            void ExceptionToJson(Delphi::Exception::Exception *AException, CString& Json);

            void ResultToJson(const CQueryResult& Result, CString& Json);
            void RowToJson(const CStringList& Row, CString& Json);

            void PQResultToJson(CPQResult *Result, CString& Json);
            void QueryToReply(CPQPollQuery *APollQuery, CReply *AReply);

        protected:

            void Post(CHTTPConnection *AConnection);
            void Get(CHTTPConnection *AConnection);

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override;
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override;

        public:

            CClient365();

            ~CClient365() override;

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
