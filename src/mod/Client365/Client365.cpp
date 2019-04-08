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
            CQueryResult LResult;

            if (LConnection != nullptr) {

                auto LReply = LConnection->Reply();

                CReply::status_type LStatus = CReply::internal_server_error;

                try {
                    InitResult(APollQuery, LResult);

                    const CString &Session = LResult[0][0].Values("session");

                    if (!Session.IsEmpty()) {
                        LReply->AddHeader(_T("Authenticate"), _T("SESSION="));
                        LReply->Headers.Last().Value += Session;
                    }

                    LStatus = CReply::ok;

                    ResultToJson(LResult, LReply->Content);

                    Log()->Debug(0, LReply->Content.c_str());

                } catch (Delphi::Exception::Exception &E) {
                    ExceptionToJson(&E, LReply->Content);
                    Log()->Error(LOG_EMERG, 0, E.what());
                }

                LConnection->SendReply(LStatus, nullptr, true);

            } else {

                auto LJob = m_Jobs->FindJobByQuery(APollQuery);

                if (LJob != nullptr) {
                    try {
                        InitResult(APollQuery, LResult);

                        ResultToJson(LResult, LJob->Result());

                        Log()->Debug(0, LJob->Result().c_str());

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
                LRoute += '/' + LUri[I];
            }

            auto LQuery = GetQuery(AConnection);
            if (LQuery == nullptr) {
                AConnection->SendStockReply(CReply::internal_server_error);
                return;
            }

            if (LUri[3] == _T("login")) {

                if (LRequest->Content.IsEmpty()) {
                    AConnection->SendStockReply(CReply::no_content);
                    return;
                }

                CJSON Login;

                Login << LRequest->Content;

                const CString &UserName = Login["username"].AsSiring();
                const CString &Password = Login["password"].AsSiring();

                if (UserName.IsEmpty() && Password.IsEmpty()) {
                    AConnection->SendStockReply(CReply::bad_request);
                    return;
                }

                CString Host(Login["host"].AsSiring());
                if (Host.IsEmpty()) {
                    Host = AConnection->Socket()->Binding()->PeerIP();
                }

                LQuery->SQL().Add(CString());

                LQuery->SQL().Last() = "SELECT * FROM apis.login('";
                LQuery->SQL().Last() += UserName;
                LQuery->SQL().Last() += "', '" + Password;
                LQuery->SQL().Last() += "', '" + Host;
                LQuery->SQL().Last() += "');";

            } else {

                const CString &LAuthorization = LRequest->Headers.Values(_T("authorization"));
                const CString &LAuthenticate = LRequest->Headers.Values(_T("authenticate"));

                if (LAuthorization.IsEmpty() && LAuthenticate.IsEmpty()) {
                    AConnection->SendStockReply(CReply::unauthorized);
                    return;
                }

                if (!LAuthenticate.IsEmpty()) {
                    size_t Pos = LAuthenticate.Find('=');
                    if (Pos == CString::npos) {
                        AConnection->SendStockReply(CReply::bad_request);
                        return;
                    }

                    m_Session = LAuthenticate.SubString(Pos + 1);
                }

                if (m_Session.Length() != 40) {
                    AConnection->SendStockReply(CReply::bad_request);
                    return;
                }

                LQuery->SQL().Add("SELECT * FROM apis.slogin('" + m_Session + "');");
                LQuery->SQL().Add("SELECT * FROM api.run('" + LRoute);

                if (!LRequest->Content.IsEmpty()) {
                    LQuery->SQL().back() += "', '" + LRequest->Content;
                }

                LQuery->SQL().back() += "');";
            }

            if (LQuery->QueryStart() != POLL_QUERY_START_ERROR) {

                if (LVersion == 1) {
                    auto LJob = m_Jobs->Add(LQuery);
                    auto LReply = AConnection->Reply();

                    LReply->Content = "{\"jobid\":" "\"" + LJob->JobId() + "\"}";

                    AConnection->SendReply(CReply::accepted);
                } else {
                    // Wait query result...
                    AConnection->CloseConnection(false);
                }
            } else {
                delete LQuery;
            }
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