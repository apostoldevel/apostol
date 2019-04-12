/*++

Programm name:

  Apostol

Module Name:

  Client365.cpp

Notices:

  Apostol Web Service (Client365)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

//----------------------------------------------------------------------------------------------------------------------

#include "Apostol.hpp"
#include "Client365.hpp"
//----------------------------------------------------------------------------------------------------------------------

#include <sstream>
#include <random>
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace Client365 {

        unsigned char random_char() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> dis(0, 255);
            return static_cast<unsigned char>(dis(gen));
        }
        //--------------------------------------------------------------------------------------------------------------

        CString generate_hex(const unsigned int len) {
            CString S = CString(len, ' ');

            for (auto i = 0; i < len / 2; i++) {
                auto rc = random_char();
                ByteToHexStr(S.Data() + i * 2 * sizeof(TCHAR), S.Size(), &rc, 1);
            }

            S[8] = '-';
            S[13] = '-';
            S[18] = '-';
            S[23] = '-';

            S[14] = '7';

            return S;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJob ------------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJob::CJob(CCollection *ACCollection) : CCollectionItem(ACCollection) {
            m_JobId = generate_hex(36);
            m_PollQuery = nullptr;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CJobManager -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::Get(int Index) {
            return (CJob *) inherited::GetItem(Index);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CJobManager::Set(int Index, CJob *Value) {
            inherited::SetItem(Index, (CCollectionItem *) Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::Add(CPQPollQuery *Query) {
            auto LJob = new CJob(this);
            LJob->PollQuery(Query);
            return LJob;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::FindJobById(const CString &Id) {
            CJob *LJob = nullptr;
            for (int I = 0; I < Count(); ++I) {
                LJob = Get(I);
                if (LJob->JobId() == Id)
                    break;
            }
            return LJob;
        }
        //--------------------------------------------------------------------------------------------------------------

        CJob *CJobManager::FindJobByQuery(CPQPollQuery *Query) {
            CJob *LJob = nullptr;
            for (int I = 0; I < Count(); ++I) {
                LJob = Get(I);
                if (LJob->PollQuery() == Query)
                    break;
            }
            return LJob;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CClient365 ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CClient365::CClient365() : CApostolModule() {
            m_Jobs = new CJobManager();
            UpdateCacheList();
        }
        //--------------------------------------------------------------------------------------------------------------

        CClient365::~CClient365() {
            delete m_Jobs;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::RowToJson(const CStringList &Row, CString &Json) {
            Json = "{";

            for (int I = 0; I < Row.Count(); ++I) {
                if (I > 0) {
                    Json += ", ";
                }

                const CString &Name = Row.Names(I);
                const CString &Value = Row.Values(Name);

                Json += "\"";
                Json += Name;
                Json += "\"";

                if (Name == _T("session")) {
                    Json += ": ";
                    if (Value == _T("<null>")) {
                        Json += _T("null");
                    } else {
                        Json += "\"";
                        Json += Value;
                        Json += "\"";
                    }
                } else if (Name == _T("result")) {
                    Json += ": ";
                    if (Value == _T("t")) {
                        Json += _T("true");
                    } else {
                        Json += _T("false");
                    }
                } else {
                    Json += ": \"";
                    Json += Value;
                    Json += "\"";
                }
            }

            Json += "}";
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::PQResultToJson(CPQResult *Result, CString &Json) {
            Json = "{";

            for (int I = 0; I < Result->nFields(); ++I) {
                if (I > 0) {
                    Json += ", ";
                }

                Json += "\"";
                Json += Result->fName(I);
                Json += "\"";

                if (SameText(Result->fName(I),_T("session"))) {
                    Json += ": ";
                    if (Result->GetIsNull(0, I)) {
                        Json += _T("null");
                    } else {
                        Json += "\"";
                        Json += Result->GetValue(0, I);
                        Json += "\"";
                    }
                } else if (SameText(Result->fName(I),_T("result"))) {
                    Json += ": ";
                    if (SameText(Result->GetValue(0, I), _T("t"))) {
                        Json += _T("true");
                    } else {
                        Json += _T("false");
                    }
                } else {
                    Json += ": \"";
                    Json += Result->GetValue(0, I);
                    Json += "\"";
                }
            }

            Json += "}";
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::ResultToJson(const CQueryResult &Result, CString &Json) {

            if ((Result.Count() > 0) && (Result.Count() <= 2)) {

                int Index = 0;

                const CString &VResult = Result[Index][0].Values("result");

                if (VResult.IsEmpty()) {
                    throw Delphi::Exception::EDBError(_T("Not found result in request!"));
                }

                if (Result.Count() == 2) {

                    if (VResult == "t") {

                        Json = "{\"result\": [";

                        Index++;

                        for (int Row = 0; Row < Result[Index].Count(); ++Row) {

                            const CString &VRun = Result[Index][Row].Values("run");

                            if (!VRun.IsEmpty()) {
                                if (Row != 0)
                                    Json += ", ";
                                Json += VRun;

                            } else
                                throw Delphi::Exception::EDBError(_T("Not found run in request!"));
                        }

                        Json += "]}";

                    } else {

                        const CString &VError = Result[Index][0].Values("error");
                        if (!VError.IsEmpty()) {
                            Log()->Postgres(LOG_EMERG, VError.c_str());
                        }

                        RowToJson(Result[Index][0], Json);
                    }

                } else {

                    const CString &VError = Result[Index][0].Values("error");
                    if (!VError.IsEmpty()) {
                        Log()->Postgres(LOG_EMERG, VError.c_str());
                    }

                    RowToJson(Result[Index][0], Json);
                }
            } else {
                throw Delphi::Exception::EDBError(_T("Invalid record count!"));
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::InitResult(CPQPollQuery *APollQuery, CQueryResult &AResult) {
            CPQResult *LResult = nullptr;
            CStringList LFields;

            for (int i = 0; i < APollQuery->ResultCount(); ++i) {
                LResult = APollQuery->Results(i);

                if (LResult->ExecStatus() == PGRES_TUPLES_OK || LResult->ExecStatus() == PGRES_SINGLE_TUPLE) {

                    LFields.Clear();
                    for (int I = 0; I < LResult->nFields(); ++I) {
                        LFields.Add(LResult->fName(I));
                    }

                    AResult.Add(TList<CStringList>());
                    for (int Row = 0; Row < LResult->nTuples(); ++Row) {
                        AResult[i].Add(CStringList());
                        for (int Col = 0; Col < LResult->nFields(); ++Col) {
                            if (LResult->GetIsNull(Row, Col)) {
                                AResult[i].Last().AddPair(LFields[Col].c_str(), "<null>");
                            } else {
                                if (LResult->fFormat(Col) == 0) {
                                    AResult[i].Last().AddPair(LFields[Col].c_str(), LResult->GetValue(Row, Col));
                                } else {
                                    AResult[i].Last().AddPair(LFields[Col].c_str(), "<binary>");
                                }
                            }
                        }
                    }
                } else {
                    throw Delphi::Exception::EDBError(LResult->GetErrorMessage());
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::QueryToJson(CPQPollQuery *Query, CString& Json, CString &Session) {

            CPQResult *Login = nullptr;
            CPQResult *Run = nullptr;

            clock_t start = clock();

            if ((Query->ResultCount() > 0) && (Query->ResultCount() <= 2)) {

                CString LoginResult;

                Login = Query->Results(0);
                if (Login->ExecStatus() == PGRES_TUPLES_OK) {

                    LoginResult = Login->GetValue(0, 1);

                    if (LoginResult.IsEmpty()) {
                        throw Delphi::Exception::EDBError(_T("Login Result Is Empty!"));
                    }

                    Session = Login->GetValue(0, 0);

                    if (Query->ResultCount() == 2) {
                        Run = Query->Results(1);

                        if (Run->ExecStatus() == PGRES_TUPLES_OK) {
                            if (LoginResult == "t") {

                                Json = "{\"result\": ";

                                if (Run->nTuples() > 0) {

                                    Json += "[";
                                    for (int Row = 0; Row < Run->nTuples(); ++Row) {
                                        for (int Col = 0; Col < Run->nFields(); ++Col) {
                                            if (Row != 0)
                                                Json += ", ";
                                            if (Run->GetIsNull(Row, Col)) {
                                                Json += "null";
                                            } else {
                                                Json += Run->GetValue(Row, Col);
                                            }
                                        }
                                    }
                                    Json += "]";

                                } else {
                                    Json += "{}";
                                }

                                Json += "}";

                            } else {
                                Log()->Error(LOG_EMERG, 0, Login->GetValue(0, 2));
                                PQResultToJson(Login, Json);
                            }
                        } else {
                            throw Delphi::Exception::EDBError(Run->GetErrorMessage());
                        }
                    } else {
                        Log()->Error(LOG_EMERG, 0, Login->GetValue(0, 2));
                        PQResultToJson(Login, Json);
                    }
                } else {
                    throw Delphi::Exception::EDBError(Login->GetErrorMessage());
                }
            } else {
                throw Delphi::Exception::EDBError(_T("Invalid record count!"));
            }

            log_debug1(LOG_DEBUG_CORE, Log(), 0, _T("Query parse runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::ExceptionToJson(Delphi::Exception::Exception *AException, CString &Json) {
            Json.Format(
                    _T("{\"error\": {\"errors\": [{\"domain\": \"%s\", \"reason\": \"%s\", \"message\": "
                       "\"%s\", \"locationType\": \"%s\", \"location\": \"%s\"}], \"code\": %u, "
                       "\"message\": \"%s\"}}"), _T("module"), _T("exception"),
                    AException->what(), _T("Query"), _T("PostgreSQL"), 500, _T("Internal Server Error"));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {
            auto LConnection = dynamic_cast<CHTTPConnection *> (APollQuery->PollConnection());

            CString Session;

            if (LConnection != nullptr) {

                auto LReply = LConnection->Reply();

                CReply::status_type LStatus = CReply::internal_server_error;

                try {
                    QueryToJson(APollQuery, LReply->Content, Session);

                    if (!Session.IsEmpty()) {
                        LReply->AddHeader(_T("Authenticate"), _T("SESSION="));
                        LReply->Headers.Last().Value += Session;
                    }

                    if (!LReply->CacheFile.IsEmpty()) {
                        clock_t start = clock();
                        LReply->Content.SaveToFile(LReply->CacheFile.c_str());
                        log_debug1(LOG_DEBUG_CORE, Log(), 0, _T("Save cache runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
                    }

                    LStatus = CReply::ok;

                } catch (Delphi::Exception::Exception &E) {
                    ExceptionToJson(&E, LReply->Content);
                    Log()->Error(LOG_EMERG, 0, E.what());
                }

                LConnection->SendReply(LStatus, nullptr, true);

            } else {

                auto LJob = m_Jobs->FindJobByQuery(APollQuery);

                if (LJob != nullptr) {
                    try {
                        QueryToJson(APollQuery, LJob->Result(), Session);
                        if (!LJob->CacheFile().IsEmpty()) {
                            clock_t start = clock();
                            LJob->Result().SaveToFile(LJob->CacheFile().c_str());
                            log_debug1(LOG_DEBUG_CORE, Log(), 0, _T("Save cache runtime: %.2f ms."), (double) ((clock() - start) / (double) CLOCKS_PER_SEC * 1000));
                        }

                    } catch (Delphi::Exception::Exception &E) {
                        ExceptionToJson(&E, LJob->Result());
                        Log()->Error(LOG_EMERG, 0, E.what());
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) {
            auto LConnection = dynamic_cast<CHTTPConnection *> (APollQuery->PollConnection());

            if (LConnection != nullptr) {
                auto LReply = LConnection->Reply();

                CReply::status_type LStatus = CReply::internal_server_error;

                ExceptionToJson(AException, LReply->Content);

                LConnection->SendReply(LStatus);
            }

            Log()->Error(LOG_EMERG, 0, AException->what());
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::UpdateCacheList() {
            CString LFile;
            LFile = Config()->Prefix() + _T("client365.cache");
            if (FileExists(LFile.c_str())) {
                m_CacheList.LoadFromFile(LFile.c_str());
                for (int I = m_CacheList.Count() - 1; I >= 0; --I) {
                    LFile = Config()->CachePrefix() + m_CacheList[I].SubString(1) + __T("/");
                    if (!ForceDirectories(LFile.c_str())) {
                        m_CacheList.Delete(I);
                    }
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::CheckCache(const CString &FileName) {
            if (!FileExists(FileName.c_str()))
                return false;

            time_t age = FileAge(FileName.c_str());
            if (age == -1)
                return false;

            time_t now = time(nullptr);

            return (now - age) <= 1 * 60; // 1 min
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::Get(CHTTPConnection *AConnection) {
            auto LRequest = AConnection->Request();

            CString JobId = LRequest->Uri.SubString(1);

            if (JobId.Length() != 36) {
                AConnection->SendStockReply(CReply::bad_request);
                return;
            }

            auto LJob = m_Jobs->FindJobById(JobId);

            if (LJob == nullptr) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            if (LJob->Result().IsEmpty()) {
                AConnection->SendStockReply(CReply::accepted);
                return;
            }

            auto LReply = AConnection->Reply();

            LReply->Content = LJob->Result();

            AConnection->SendReply(CReply::ok);

            delete LJob;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::Post(CHTTPConnection *AConnection) {
            auto LRequest = AConnection->Request();
            int LVersion = -1;

            CString LCacheFile;

            CStringList LUri;
            SplitColumns(LRequest->Uri.c_str(), LRequest->Uri.Size(), &LUri, '/');
            if (LUri.Count() < 3) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            if (LUri[1] == _T("v1")) {
                LVersion = 1;
            } else if (LUri[1] == _T("v2")) {
                LVersion = 2;
            }

            if (LUri[0] != _T("api") || (LVersion == -1) || LUri[2] != _T("json")) {
                AConnection->SendStockReply(CReply::not_found);
                return;
            }

            const CString &LContentType = LRequest->Headers.Values(_T("content-type"));
            if (!LContentType.IsEmpty() && LRequest->ContentLength == 0) {
                AConnection->SendStockReply(CReply::no_content);
                return;
            }

            CString LRoute;
            for (int I = 3; I < LUri.Count(); ++I) {
                LRoute.Append('/');
                LRoute.Append(LUri[I]);
            }

            CStringList LSQL;

            if (LUri[3] == _T("login")) {

                if (LRequest->Content.IsEmpty()) {
                    AConnection->SendStockReply(CReply::no_content);
                    return;
                }

                CJSON Login;

                Login << LRequest->Content;

                CString Host(Login["host"].AsSiring());
                if (Host.IsEmpty()) {
                    Host = AConnection->Socket()->Binding()->PeerIP();
                }

                const CString &Session = Login["session"].AsSiring();
                if (Session.IsEmpty()) {
                    LSQL.Add("SELECT * FROM apis.login('");

                    const CString &UserName = Login["username"].AsSiring();
                    const CString &Password = Login["password"].AsSiring();

                    if (UserName.IsEmpty() && Password.IsEmpty()) {
                        AConnection->SendStockReply(CReply::bad_request);
                        return;
                    }

                    LSQL.Last() += UserName;
                    LSQL.Last() += "', '" + Password;
                } else {
                    LSQL.Add("SELECT * FROM apis.slogin('");
                    LSQL.Last() += Session;
                }

                LSQL.Last() += "', '" + Host;

                const CString &Department = Login["department"].AsSiring();
                if (!Department.IsEmpty())
                    LSQL.Last() += "', '" + Department;

                const CString &WorkPlace = Login["workplace"].AsSiring();
                if (!WorkPlace.IsEmpty())
                    LSQL.Last() += "', '" + WorkPlace;

                LSQL.Last() += "');";

            } else {

                const CString &LAuthorization = LRequest->Headers.Values(_T("authorization"));
                const CString &LAuthenticate = LRequest->Headers.Values(_T("authenticate"));

                if (LAuthorization.IsEmpty() && LAuthenticate.IsEmpty()) {
                    AConnection->SendStockReply(CReply::unauthorized);
                    return;
                }

                CString LSession;

                if (!LAuthenticate.IsEmpty()) {
                    size_t Pos = LAuthenticate.Find('=');
                    if (Pos == CString::npos) {
                        AConnection->SendStockReply(CReply::bad_request);
                        return;
                    }

                    LSession = LAuthenticate.SubString(Pos + 1);
                }

                if (LSession.Length() != 40) {
                    AConnection->SendStockReply(CReply::bad_request);
                    return;
                }

                //int Index = m_CacheList.IndexOf(LRoute);
                //if (Index != -1)
                {

                    LCacheFile = Config()->CachePrefix();
                    LCacheFile += LRoute.SubString(1);
                    LCacheFile += _T("/");

                    if (!DirectoryExists(LCacheFile.c_str())) {
                        if (!ForceDirectories(LCacheFile.c_str())) {
                            AConnection->SendStockReply(CReply::internal_server_error);
                            return;
                        }
                    }

                    LCacheFile += LSession;
                    if (!LRequest->Content.IsEmpty()) {
                        std::hash<std::string> ContentHash;
                        std::string Content(LRequest->Content.c_str());
                        LCacheFile.Append('.');
                        LCacheFile << (size_t) ContentHash(Content);
                    }
                }

                if (!LCacheFile.IsEmpty()) {
                    auto LReply = AConnection->Reply();
                    LReply->CacheFile = LCacheFile;
                    if (CheckCache(LReply->CacheFile)) {
                        LReply->Content.LoadFromFile(LReply->CacheFile);
                        AConnection->SendReply(CReply::ok);
                        return;
                    }
                }

                LSQL.Add("SELECT * FROM apis.slogin('" + LSession + "');");
                LSQL.Add("SELECT * FROM api.run('" + LRoute);

                if (LRequest->Content == "{}" || LRequest->Content == "[]")
                    LRequest->Content.Clear();

                if (!LRequest->Content.IsEmpty()) {
                    LSQL.Last() += "', '" + LRequest->Content;
                }

                LSQL.Last() += "');";
            }

            QueryStart(AConnection, LSQL, LCacheFile);
        }
        //--------------------------------------------------------------------------------------------------------------

        bool CClient365::QueryStart(CHTTPConnection *AConnection, const CStringList& ASQL, const CString& ACacheFile) {
            auto LQuery = GetQuery(AConnection);

            if (LQuery == nullptr) {
                AConnection->SendStockReply(CReply::internal_server_error);
                return false;
            }

            LQuery->SQL() = ASQL;

            if (LQuery->QueryStart() != POLL_QUERY_START_ERROR) {
                if (m_Version == 1) {
                    auto LJob = m_Jobs->Add(LQuery);
                    auto LReply = AConnection->Reply();

                    LJob->CacheFile() = ACacheFile;

                    LReply->Content = "{\"jobid\":" "\"" + LJob->JobId() + "\"}";

                    AConnection->SendReply(CReply::accepted);
                } else {
                    // Wait query result...
                    AConnection->CloseConnection(false);
                }

                return true;
            } else {
                delete LQuery;
            }

            return false;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CClient365::Execute(CHTTPConnection *AConnection) {
            auto LRequest = AConnection->Request();
            auto LReply = AConnection->Reply();

            LReply->Clear();
            LReply->ContentType = CReply::json;

            if (LRequest->Method == _T("POST")) {
                Post(AConnection);
                return;
            }

            if (LRequest->Method == _T("GET")) {
                Get(AConnection);
                return;
            }

            AConnection->SendStockReply(CReply::not_implemented);
        }

    }
}
}