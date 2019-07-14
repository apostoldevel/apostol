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

            CString m_CacheFile;

            CPQPollQuery *m_PollQuery;

        public:

            explicit CJob(CCollection *ACCollection);

            ~CJob() override = default;

            CString& JobId() { return m_JobId; };
            const CString& JobId() const { return m_JobId; };

            CString& CacheFile() { return m_CacheFile; };
            const CString& CacheFile() const { return m_CacheFile; };

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

            int m_Version;

            CJobManager *m_Jobs;

            CStringList m_CacheList;

            void InitResult(CPQPollQuery *APollQuery, CQueryResult& AResult);

            void ExceptionToJson(Delphi::Exception::Exception *AException, CString& Json);

            void ResultToJson(const CQueryResult& Result, CString& Json);
            void RowToJson(const CStringList& Row, CString& Json);

            void PQResultToJson(CPQResult *Result, CString& Json);
            void QueryToJson(CPQPollQuery *Query, CString& Json, CString &Session);

            bool QueryStart(CHTTPConnection *AConnection, const CStringList& ASQL, const CString& ACacheFile);

        protected:

            void Post(CHTTPConnection *AConnection);
            void Get(CHTTPConnection *AConnection);

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override;
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override;

        public:

            explicit CClient365(CModuleManager *AManager);

            ~CClient365() override;

            static class CClient365 *CreateModule(CModuleManager *AManager) {
                return new CClient365(AManager);
            }

            void Execute(CHTTPConnection *AConnection) override;

            bool CheckUrerArent(const CString& Value) override;

            bool CheckCache(const CString &FileName);

            void UpdateCacheList();

            CStringList &CacheList() { return m_CacheList; };
            const CStringList &CacheList() const { return m_CacheList; };

        };

    }
}

using namespace Apostol::Client365;
}
#endif //APOSTOL_CLIENT365_HPP
