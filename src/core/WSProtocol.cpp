/*++

Program name:

  apostol

Module Name:

  WSProtocol.cpp

Notices:

  Apostol core (WebSocket Protocol)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Core.hpp"
#include "WSProtocol.hpp"
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace WSProtocol {

        //--------------------------------------------------------------------------------------------------------------

        //-- CWSProtocol -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        // t: MessageTypeId
        // u: UniqueId
        // a: Action
        // c: ErrorCode
        // m: ErrorMessage
        // p: Payload

        bool CWSProtocol::Request(const CString &String, CWSMessage &Message) {
            bool Result = false;

            if (String.Size() > 0) {
                CJSON Json(String);

                const auto type = Json["t"].AsInteger();

                switch (type) {
                    case 0:
                        Message.MessageTypeId = mtOpen;
                        break;
                    case 1:
                        Message.MessageTypeId = mtClose;
                        break;
                    case 2:
                        Message.MessageTypeId = mtCall;
                        break;
                    case 3:
                        Message.MessageTypeId = mtCallResult;
                        break;
                    case 4:
                        Message.MessageTypeId = mtCallError;
                        break;
                    default:
                        throw Delphi::Exception::Exception("Invalid \"MessageTypeId\" value.");
                }

                const auto& errorCode = Json["c"];

                Message.UniqueId     = Json["u"].AsString();
                Message.Action       = Json["a"].AsString();
                Message.ErrorCode    = errorCode.IsEmpty() ? -1 : Json["c"].AsInteger();
                Message.ErrorMessage = Json["m"].AsString();

                const auto& payload  = Json["p"];
                if (payload.ValueType() == jvtObject)
                    Message.Payload  = payload.ToString();
            }

            return Result;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWSProtocol::Response(const CWSMessage &Message, CString &String) {

            const auto& Payload = Message.Payload.ToString();
            const size_t Size = Message.Size() + Payload.Size();

            String.MaxFormatSize(64 + Size);

            switch (Message.MessageTypeId) {
                case mtOpen:
                    String.Format(R"({"t":0,"u":"%s","p":%s})",
                                  Message.UniqueId.c_str(),
                                  Payload.IsEmpty() ? "{}" : Payload.c_str()
                    );
                    break;

                case mtClose:
                    String.Format(R"({"t":1,"u":"%s","p":%s})",
                                  Message.UniqueId.c_str(),
                                  Payload.IsEmpty() ? "{}" : Payload.c_str()
                    );
                    break;

                case mtCall:
                    String.Format(R"({"t":2,"u":"%s","a":"%s","p":%s})",
                                  Message.UniqueId.c_str(),
                                  Message.Action.c_str(),
                                  Payload.IsEmpty() ? "{}" : Payload.c_str()
                    );
                    break;

                case mtCallResult:
                    String.Format(R"({"t":3,"u":"%s","p":%s})",
                                  Message.UniqueId.c_str(),
                                  Payload.IsEmpty() ? "{}" : Payload.c_str()
                    );
                    break;

                case mtCallError:
                    String.Format(R"({"t":4,"u":"%s","c":%d,"m":"%s"})",
                                  Message.UniqueId.c_str(),
                                  Message.ErrorCode,
                                  Message.ErrorMessage.c_str()
                    );
                    break;
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWSProtocol::PrepareResponse(const CWSMessage &Request, CWSMessage &Response) {
            Response.MessageTypeId = mtCallResult;
            Response.UniqueId = Request.UniqueId;
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWSProtocol::Call(const CString &UniqueId, const CString &Action, const CJSON &Payload, CString &Result) {
            CWSMessage Message;
            Message.MessageTypeId = mtCall;
            Message.UniqueId = UniqueId;
            Message.Action = Action;
            Message.Payload = Payload;
            Response(Message, Result);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWSProtocol::CallResult(const CString &UniqueId, const CJSON &Payload, CString &Result) {
            CWSMessage Message;
            Message.MessageTypeId = mtCallResult;
            Message.UniqueId = UniqueId;
            Message.Payload = Payload;
            Response(Message, Result);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CWSProtocol::CallError(const CString &UniqueId, int ErrorCode, const CString &ErrorMessage,
                const CJSON &Payload, CString &Result) {

            CWSMessage Message;
            Message.MessageTypeId = mtCallError;
            Message.UniqueId = UniqueId;
            Message.ErrorCode = ErrorCode;
            Message.ErrorMessage = ErrorMessage;
            Message.Payload = Payload;
            Response(Message, Result);
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CMessageHandler -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CMessageHandler::CMessageHandler(CMessageManager *AManager, COnMessageHandlerEvent &&Handler) :
                CCollectionItem(AManager), m_Handler(Handler) {
            m_UniqueId = GetUID(APOSTOL_MODULE_UID_LENGTH);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMessageHandler::Handler(CHTTPServerConnection *AConnection) {
            if (m_Handler != nullptr) {
                m_Handler(this, AConnection);
            }
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CMessageManager -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CMessageHandler *CMessageManager::Get(int Index) {
            return dynamic_cast<CMessageHandler *> (inherited::GetItem(Index));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CMessageManager::Set(int Index, CMessageHandler *Value) {
            inherited::SetItem(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        CMessageHandler *CMessageManager::Add(COnMessageHandlerEvent &&Handler, const CString &Action,
                const CJSON &Payload) {
            
            auto LHandler = new CMessageHandler(this, static_cast<COnMessageHandlerEvent &&>(Handler));
            auto LConnection = m_pSession->Connection();

            LHandler->Action() = Action;

            CString LResult;

            CWSProtocol::Call(LHandler->UniqueId(), Action, Payload, LResult);

            auto LWSReply = LConnection->WSReply();

            LWSReply->Clear();
            LWSReply->SetPayload(LResult);

            LConnection->SendWebSocket(true);

            return LHandler;
        }
        //--------------------------------------------------------------------------------------------------------------

        CMessageHandler *CMessageManager::FindMessageById(const CString &Value) {
            CMessageHandler *Handler = nullptr;

            for (int I = 0; I < Count(); ++I) {
                Handler = Get(I);
                if (Handler->UniqueId() == Value)
                    return Handler;
            }

            return nullptr;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSession --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSession::CSession(CHTTPServerConnection *AConnection, CSessionManager *AManager) : CCollectionItem(AManager) {
            m_pConnection = AConnection;
            m_pMessages = new CMessageManager(this);
            AddToConnection(AConnection);
        }
        //--------------------------------------------------------------------------------------------------------------

        CSession::~CSession() {
            DeleteFromConnection(m_pConnection);
            m_pConnection = nullptr;
            FreeAndNil(m_pMessages);
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSession::AddToConnection(CHTTPServerConnection *AConnection) {
            if (Assigned(AConnection)) {
                int Index = AConnection->Data().IndexOfName("session");
                if (Index == -1) {
                    AConnection->Data().AddObject("session", this);
                } else {
                    delete AConnection->Data().Objects(Index);
                    AConnection->Data().Objects(Index, this);
                }
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSession::DeleteFromConnection(CHTTPServerConnection *AConnection) {
            if (Assigned(AConnection)) {
                int Index = AConnection->Data().IndexOfObject(this);
                if (Index == -1)
                    AConnection->Data().Delete(Index);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSession::SwitchConnection(CHTTPServerConnection *AConnection) {
            if (m_pConnection != AConnection) {
                DeleteFromConnection(m_pConnection);
                m_pConnection->Disconnect();
                m_pConnection = AConnection;
                AddToConnection(m_pConnection);
            }
        }
        //--------------------------------------------------------------------------------------------------------------

        CSession *CSession::FindOfConnection(CHTTPServerConnection *AConnection) {
            int Index = AConnection->Data().IndexOfName("session");
            if (Index == -1)
                throw Delphi::Exception::ExceptionFrm("Not found session in connection");

            auto Object = AConnection->Data().Objects(Index);
            if (Object == nullptr)
                throw Delphi::Exception::ExceptionFrm("Object in connection data is null");

            auto Session = dynamic_cast<CSession *> (Object);
            if (Session == nullptr)
                throw Delphi::Exception::Exception("Session is null");

            return Session;
        }

        //--------------------------------------------------------------------------------------------------------------

        //-- CSessionManager -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        CSession *CSessionManager::Get(int Index) {
            return dynamic_cast<CSession *> (inherited::GetItem(Index));
        }
        //--------------------------------------------------------------------------------------------------------------

        void CSessionManager::Set(int Index, CSession *Value) {
            inherited::SetItem(Index, Value);
        }
        //--------------------------------------------------------------------------------------------------------------

        CSession *CSessionManager::Add(CHTTPServerConnection *AConnection) {
            return new CSession(AConnection, this);
        }
        //--------------------------------------------------------------------------------------------------------------

        CSession *CSessionManager::FindByIP(const CString &Value) {
            CSession *Session;
            for (int I = 0; I < Count(); ++I) {
                Session = Get(I);
                if (Session->IP() == Value)
                    return Session;
            }

            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CSession *CSessionManager::FindByIdentity(const CString &Value) {
            CSession *Session;
            for (int I = 0; I < Count(); ++I) {
                Session = Get(I);
                if (Session->Identity() == Value)
                    return Session;
            }

            return nullptr;
        }
        //--------------------------------------------------------------------------------------------------------------

        CSession *CSessionManager::FindByConnection(CHTTPServerConnection *Value) {
            CSession *Session;
            for (int I = 0; I < Count(); ++I) {
                Session = Get(I);
                if (Session->Connection() == Value)
                    return Session;
            }

            return nullptr;
        }
        
    }
}
}
