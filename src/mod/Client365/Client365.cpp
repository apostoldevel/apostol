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

namespace Apostol::Client365 {

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

    CClient365::CClient365() : CApostolModule() {

    }
    //--------------------------------------------------------------------------------------------------------------

    void CClient365::RowToJson(const CStringList& Row, CString& Json) {
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

    void CClient365::DoPostgresQueryExecuted(CPQPollQuery *APollQuery) {
        auto LConnection = dynamic_cast<CHTTPConnection *> (APollQuery->PollConnection());
        if (LConnection != nullptr) {
            auto LReply = LConnection->Reply();
            CPQResult *LResult = nullptr;

            CReply::status_type LStatus = CReply::internal_server_error;

            LReply->Clear();

            try {
                TList<TList<CStringList>> QResult;

                for (int i = 0; i < APollQuery->ResultCount(); ++i) {
                    LResult = APollQuery->Results(i);

                    if (LResult->ExecStatus() == PGRES_TUPLES_OK || LResult->ExecStatus() == PGRES_SINGLE_TUPLE) {

                        CStringList Fields;
                        for (int I = 0; I < LResult->nFields(); ++I) {
                            Fields.Add(LResult->fName(I));
                        }

                        QResult.Add(TList<CStringList>());
                        for (int Row = 0; Row < LResult->nTuples(); ++Row) {
                            QResult[i].Add(CStringList());
                            for (int Col = 0; Col < LResult->nFields(); ++Col) {
                                if (LResult->GetIsNull(Row, Col)) {
                                    QResult[i].Last().AddPair(Fields[Col].c_str(), "<null>");
                                } else {
                                    if (LResult->fFormat(Col) == 0) {
                                        QResult[i].Last().AddPair(Fields[Col].c_str(), LResult->GetValue(Row, Col));
                                    } else {
                                        QResult[i].Last().AddPair(Fields[Col].c_str(), "<binary>");
                                    }
                                }
                            }
                        }
                    } else {
                        throw Delphi::Exception::EDBError(LResult->GetErrorMessage());
                    }
                }

                if ((QResult.Count() > 0) && (QResult.Count() <= 2)) {

                    int Index = 0;

                    const CString &VResult = QResult[Index][0].Values("result");

                    if (VResult.IsEmpty()) {
                        throw Delphi::Exception::EDBError(_T("Not found result in request!"));
                    }

                    if (QResult.Count() == 2) {
                        if (VResult == "t") {

                            LStatus = CReply::ok;

                            LReply->Content = "{\"result\": [";

                            Index++;

                            for (int Row = 0; Row < QResult[Index].Count(); ++Row) {

                                const CString &VRun = QResult[Index][Row].Values("run");

                                if (!VRun.IsEmpty()) {
                                    if (Row != 0)
                                        LReply->Content += ", ";
                                    LReply->Content += VRun;

                                } else
                                    throw Delphi::Exception::EDBError(_T("Not found run in request!"));
                            }

                            LReply->Content += "]}";

                        } else {

                            const CString &VError = QResult[Index][0].Values("error");
                            if (!VError.IsEmpty()) {
                                Log()->Postgres(LOG_EMERG, VError.c_str());
                            }

                            LStatus = CReply::ok;

                            RowToJson(QResult[Index][0], LReply->Content);
                        }

                        Log()->Postgres(LOG_NOTICE, _T("Client365 result count: %d"), QResult[Index].Count());

                    } else {

                        const CString &VError = QResult[Index][0].Values("error");
                        if (!VError.IsEmpty()) {
                            Log()->Postgres(LOG_EMERG, VError.c_str());
                        }

                        LStatus = CReply::ok;

                        RowToJson(QResult[Index][0], LReply->Content);
                    }
                } else {
                    throw Delphi::Exception::EDBError(_T("Invalid record count!"));
                }

            } catch (std::exception &e) {
                LReply->Content.Clear();
                LReply->Content.Format(
                        _T("{\"error\": {\"errors\": [{\"domain\": \"%s\", \"reason\": \"%s\", \"message\": "
                           "\"%s\", \"locationType\": \"%s\", \"location\": \"%s\"}], \"code\": %u, "
                           "\"message\": \"%s\"}}"), _T("module"), _T("exception"),
                        e.what(), _T("Query"), _T("PostgreSQL"), LStatus, _T("Internal Server Error"));
            }

            CReply::SendReply(LConnection, LStatus, CReply::json, nullptr, true);
        }
    }
    //--------------------------------------------------------------------------------------------------------------

    void CClient365::DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) {
        auto LConnection = dynamic_cast<CHTTPConnection *> (APollQuery->PollConnection());
        if (LConnection != nullptr) {
            auto LReply = LConnection->Reply();

            CReply::status_type LStatus = CReply::internal_server_error;

            LReply->Content.Clear();
            LReply->Content.Format(
                    _T("{\"error\": {\"errors\": [{\"domain\": \"%s\", \"reason\": \"%s\", \"message\": "
                       "\"%s\", \"locationType\": \"%s\", \"location\": \"%s\"}], \"code\": %u, "
                       "\"message\": \"%s\"}}"), _T("module"), _T("exception"),
                    AException->what(), _T("Query"), _T("PostgreSQL"), LStatus, _T("Internal Server Error"));

            Log()->Postgres(LOG_EMERG, LReply->Content.c_str());

            CReply::SendReply(LConnection, LStatus, CReply::json);
        }
    }
    //--------------------------------------------------------------------------------------------------------------

    void CClient365::Get(CHTTPConnection *AConnection) {

    }
    //--------------------------------------------------------------------------------------------------------------

    void CClient365::Post(CHTTPConnection *AConnection) {
        auto LRequest = AConnection->Request();
        int LVersion = -1;

        CStringList LUri;
        SplitColumns(LRequest->Uri.c_str(), LRequest->Uri.Size(), &LUri, '/');
        if (LUri.Count() < 3) {
            CReply::SendStockReply(AConnection, CReply::not_found, CReply::json);
            return;
        }

        if (LUri[1] == _T("v1")) {
            LVersion = 1;
        } else if (LUri[1] == _T("v2")) {
            LVersion = 2;
        }

        if (LUri[0] != _T("api") || (LVersion == -1) || LUri[2] != _T("json")) {
            CReply::SendStockReply(AConnection, CReply::not_found, CReply::json);
            return;
        }

        const CString &LContentType = LRequest->Headers.Values(_T("content-type"));
        if (!LContentType.IsEmpty() && LRequest->ContentLength == 0) {
            CReply::SendStockReply(AConnection, CReply::bad_request, CReply::json);
            return;
        }

        const CString &Value = LRequest->Headers.Values(_T("connection"));
        if (!Value.IsEmpty()) {
            AConnection->CloseConnection(true);
            if (Value == _T("keep-alive"))
                AConnection->CloseConnection(false);
        }

        CString LRoute;
        for (int I = 3; I < LUri.Count(); ++I) {
            LRoute += '/' + LUri[I];
        }

        auto LQuery = GetQuery(AConnection);
        if (LQuery == nullptr) {
            CReply::SendStockReply(AConnection, CReply::internal_server_error, CReply::json);
            return;
        }

        if (LUri[3] == _T("login")) {

            if (LRequest->Content.IsEmpty()) {
                CReply::SendStockReply(AConnection, CReply::bad_request, CReply::json);
                return;
            }

            CJSON Login;

            Login << LRequest->Content;

            const CString &UserName = Login["username"].AsSiring();
            const CString &Password = Login["password"].AsSiring();

            if (UserName.IsEmpty() && Password.IsEmpty()) {
                CReply::SendStockReply(AConnection, CReply::bad_request, CReply::json);
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
                CReply::SendStockReply(AConnection, CReply::unauthorized, CReply::json);
                return;
            }

            if (!LAuthenticate.IsEmpty()) {
                size_t Pos = LAuthenticate.Find('=');
                if (Pos == CString::npos) {
                    CReply::SendStockReply(AConnection, CReply::bad_request, CReply::json);
                    return;
                }

                m_Session = LAuthenticate.SubString(Pos + 1);
            }

            if (m_Session.Length() != 40) {
                CReply::SendStockReply(AConnection, CReply::bad_request, CReply::json);
                return;
            }

            LQuery->SQL().Add("SELECT * FROM apis.slogin('" + m_Session + "');");
            LQuery->SQL().Add("SELECT * FROM api.run('" + LRoute);

            if (!LRequest->Content.IsEmpty()) {
                LQuery->SQL().back() += "', '" + LRequest->Content;
            }

            LQuery->SQL().back() += "');";
        }

        LQuery->JobId() = generate_hex(36);

        if (LQuery->QueryStart() != 0xFFFFFFFF) {
            if (LVersion == 1) {
                auto LReply = AConnection->Reply();

                LReply->Content = "{\"jobid\":" "\"" + LQuery->JobId() + "\"}";

                CReply::SendReply(AConnection, CReply::ok, CReply::json, nullptr, true);
            }
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

        CReply::SendStockReply(AConnection, CReply::not_implemented);
    }
}
}