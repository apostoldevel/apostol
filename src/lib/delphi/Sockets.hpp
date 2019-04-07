/*++

Library name:

  libdelphi

Module Name:

  Sockets.hpp

Notices:

  Delphi classes for C++

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef DELPHI_SOCKETS_HPP
#define DELPHI_SOCKETS_HPP
//----------------------------------------------------------------------------------------------------------------------

#include <sys/socket.h>
#include <sys/epoll.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
//----------------------------------------------------------------------------------------------------------------------

#pragma once
//----------------------------------------------------------------------------------------------------------------------

#ifndef LF
#define LF 10
#endif

#ifndef CR
#define CR 13
#endif

#define EOLW L"\r\n\0"
#define EOLA "\r\n\0"

#ifdef  _UNICODE
#    define EOL    EOLW
#else
#    define EOL         EOLA
#endif
//----------------------------------------------------------------------------------------------------------------------

#define NI_MAXHOST      1025
#define NI_MAXIP        25
//----------------------------------------------------------------------------------------------------------------------

#define TimeoutDefault  (-1)
#define TimeoutInfinite (-2)
//----------------------------------------------------------------------------------------------------------------------

#ifndef SOCKET_ERROR
#define SOCKET_ERROR    (-1)
#endif
//----------------------------------------------------------------------------------------------------------------------

#ifndef INVALID_SOCKET
#define INVALID_SOCKET  (-1)
#endif
//----------------------------------------------------------------------------------------------------------------------

typedef struct sockaddr SOCKADDR, *LPSOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
//----------------------------------------------------------------------------------------------------------------------

typedef struct servent SERVENT, *LPSERVENT;
//----------------------------------------------------------------------------------------------------------------------

namespace Delphi {

    namespace Socket {

        typedef SOCKET CSocket;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCommand;

        class LIB_DELPHI CTCPServer;

        class LIB_DELPHI CPeerThread;

        class LIB_DELPHI CSocketThread;

        class LIB_DELPHI CTCPConnection;

        class LIB_DELPHI CTCPServerConnection;

        class LIB_DELPHI CListenerThread;
        //--------------------------------------------------------------------------------------------------------------

        enum COverlappedType {
            otRecv, otSend
        };
        //--------------------------------------------------------------------------------------------------------------

        enum CErrorGroup {
            egSystem, egSocket
        };

        class LIB_DELPHI CStack;
        //--------------------------------------------------------------------------------------------------------------

        extern LIB_DELPHI CStack *GStack;
        //--------------------------------------------------------------------------------------------------------------

        LIB_DELPHI char *inet_ntoa2(in_addr_t ulAddr);

        LIB_DELPHI bool IsCurrentThread(CThread *AThread);

        LIB_DELPHI void SplitColumns(LPCTSTR AData, size_t ASize, CStringList *AStrings, char ADelim);

        //--------------------------------------------------------------------------------------------------------------

        //-- CStack ----------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CStack {
        private:

            int m_LastError;

        public:

            CStack();

            virtual ~CStack() = default;

            inline static class CStack *CreateSocket() { return GStack = new CStack; };

            inline void DeleteSocket() { delete GStack; };

            CSocket CreateSocketHandle(int ASocketType, int AProtocol = IPPROTO_IP, int AFlag = 0);

            virtual bool CheckForSocketError(ssize_t AResult, CErrorGroup AErrGroup);

            virtual bool CheckForSocketError(ssize_t AResult, int const AIgnore[], int ACount, CErrorGroup AErrGroup);

            void RaiseSocketError(int AErr);

            virtual CSocket Accept(CSocket ASocket, char *VIP, size_t ASize, unsigned short *VPort, int AFlags);

            virtual int Bind(CSocket ASocket, sa_family_t AFamily, LPCSTR AIP, unsigned short APort);

            virtual int CloseSocket(CSocket ASocket);

            virtual int Connect(CSocket ASocket, sa_family_t AFamily, LPCSTR AIP, unsigned short APort);

            virtual int Listen(CSocket ASocket, int ABackLog);

            virtual uint16_t HToNS(uint16_t AHostShort);

            virtual uint16_t NToHS(uint16_t ANetShort);

            virtual uint32_t HToNL(uint32_t AHostLong);

            virtual uint32_t NToHL(uint32_t ANetLong);

            virtual ssize_t Recv(CSocket ASocket, void *ABuffer, size_t ABufferLength, int AFlags);

            virtual ssize_t RecvFrom(CSocket ASocket, void *ABuffer, size_t ABufferLength, int AFlags, char *VIP,
                    size_t ASize, unsigned short *VPort, LPSOCKADDR AFrom, socklen_t *AFromLen);

            virtual CSocket Select(CList *ARead, CList *AWrite, CList *AErrors, int ATimeout);

            virtual ssize_t Send(CSocket ASocket, void *ABuffer, size_t ABufferLength, int AFlags);

            virtual ssize_t SendTo(CSocket ASocket, void *ABuffer, size_t ABufferLength, int AFlags, LPCSTR AIP,
                    unsigned short APort);

            virtual int SetSockOpt(CSocket ASocket, int ALevel, int AOptName, const void *AOptVal, socklen_t AOptLen);

            virtual CSocket Socket(int ADomain, int AType, int AProtocol, int AFlag);

            virtual int Shutdown(CSocket ASocket, int AHow);

            bool GetHostName(char *AName, size_t ASize);

            virtual int GetLastError();

            virtual void GetPeerName(CSocket ASocket, sa_family_t *AFamily, char *AIP, size_t ASize,
                    unsigned short *APort);

            virtual void GetSockName(CSocket ASocket, sa_family_t *AFamily, char *AIP, size_t ASize,
                    unsigned short *APort);

            virtual void GetHostByAddr(SOCKADDR_IN *AAddr, char *VHost, size_t ASize);

            virtual void GetHostByName(char *AName, char *VHost, size_t ASize);

            virtual void GetIPByName(char *AName, char *VIP, size_t ASize);

            virtual void GetServByPort(SOCKADDR_IN *AName, char *VPort, size_t ASize);

            virtual int GetSockOpt(CSocket ASocket, int Alevel, int AOptname, void *AOptval, socklen_t *AOptlen);

            virtual void SetNonBloking(CSocket ASocket);

            int LastError() { return m_LastError; }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CSocketComponent ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        enum CWorkMode {
            wmRead, wmWrite
        };
        //--------------------------------------------------------------------------------------------------------------

        typedef struct tagWORK_INFO {
            ssize_t Current;
            ssize_t Max;
            int Level;
        } CWorkInfo;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSocketComponent: public CPersistent {
        protected:

            CWorkInfo m_WorkInfos[sizeof(CWorkInfo)];

        public:

            CSocketComponent();

            ~CSocketComponent() override;

            void BeginWork(CWorkMode AWorkMode, ssize_t ASize = 0); // ASize = 0

            void DoWork(CWorkMode AWorkMode, ssize_t ACount);

            void EndWork(CWorkMode AWorkMode);

        }; // CSocketComponent

        //--------------------------------------------------------------------------------------------------------------

        //-- CSimpleBuffer ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        #define GRecvBufferSizeDefault  (64 * 1024)
        #define GSendBufferSizeDefault  (64 * 1024)
        #define MaxLineLengthDefault    (32 * 1024)
        #define InBufCacheSizeDefault   (32 * 1024) //CManagedBuffer.PackReadedSize
        //--------------------------------------------------------------------------------------------------------------

        enum CMaxLineAction {
            maException, maSplit
        };
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSimpleBuffer : public CMemoryStream {
            typedef CMemoryStream inherited;

        public:

            CSimpleBuffer();

            virtual size_t Extract(void *ABuffer, size_t AByteCount);

            virtual void Remove(size_t AByteCount);

        }; // CSimpleBuffer

        //--------------------------------------------------------------------------------------------------------------

        //-- CManagedBuffer --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CManagedBuffer: public CSimpleBuffer {
            typedef CSimpleBuffer inherited;

        protected:

            size_t m_PackReadedSize;
            size_t m_ReadedSize;

            void SetPackReadedSize(size_t Value);

        public:

            CManagedBuffer();

            void Clear() override;

            size_t Extract(void *ABuffer, size_t AByteCount) override;

            Pointer Memory() override;

            void PackBuffer();

            void Remove(size_t AByteCount) override;

            size_t Seek(size_t Offset, unsigned short Origin);

            size_t PackReadedSize() { return m_PackReadedSize; }

            void PackReadedSize(size_t Value) { SetPackReadedSize(Value); }

        }; // CManagedBuffer

        //--------------------------------------------------------------------------------------------------------------

        //-- CSocketHandle ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSocketHandle: public CSocketComponent, public CCollectionItem {
            typedef CCollectionItem inherited;

        private:

            CSocket m_Handle;

            int m_SocketType;

            bool m_HandleAllocated;
            bool m_Nonblocking;

            char m_IP[NI_MAXIP];
            char m_PeerIP[NI_MAXIP];

            unsigned short m_Port;
            unsigned short m_PeerPort;

            unsigned short m_ClientPortMin;
            unsigned short m_ClientPortMax;

            bool TryBind();

            bool BindPortReserved();

        protected:

            SOCKADDR_IN m_From;
            socklen_t m_FromLen;

            virtual void SetDefaultPort(unsigned short Value);

            void SetIP(LPCSTR Value);

            void SetPeerIP(LPCSTR Value);

            int GetLastError();

        public:

            explicit CSocketHandle(CCollection *ACollection);

            ~CSocketHandle() override;

            bool Accept(CSocket ASocket, int AFlag);

            void AllocateSocket(int ASocketType, int AProtocol, int AFlag);

            void Bind();

            void CloseSocket(bool AResetLocal = true);

            bool GetHostName(char *AName, size_t ASize);

            void GetHostByName(char *AName, char *VHost, size_t ASize);

            bool GetHostIP(char *AIP, size_t ASize);

            int Connect(sa_family_t AFamily);

            void GetSockOpt(int ALevel, int AOptName, void *AOptVal, socklen_t AOptLen);

            void Listen(int anQueueCount);

            ssize_t Recv(void *ABuffer, size_t ALength);

            ssize_t RecvFrom(void *ABuffer, size_t ALength);

            void Reset(bool AResetLocal = true);

            ssize_t Send(void *ABuffer, size_t ABufferSize);

            ssize_t SendTo(LPCSTR AIP, unsigned short APort, void *ABuffer, size_t ABufferSize);

            void SetPeer(LPCSTR asIP, unsigned short anPort);

            void SetSockOpt(int ALevel, int AOptName, const void *AOptVal, socklen_t AOptLen);

            bool Select(CSocket ASocket, int ATimeOut);

            void UpdateBindingLocal();

            void UpdateBindingPeer();

            void UpdateFromPeer();

            int SocketType() { return m_SocketType; }

            bool HandleAllocated() { return m_HandleAllocated; }

            bool Nonblocking() { return m_Nonblocking; }

            CSocket Handle() { return m_Handle; }

            char *IP() { return m_IP; }
            void IP(LPCSTR Value) { SetIP(Value); }

            unsigned short Port() { return m_Port; }
            void Port(unsigned short Value) { m_Port = Value; }

            char *PeerIP() { return m_PeerIP; }
            void PeerIP(LPCSTR Value) { SetPeerIP(Value); }

            unsigned short PeerPort() { return m_PeerPort; }

            unsigned short ClientPortMin() { return m_ClientPortMin; }
            void ClientPortMin(unsigned short Value) { m_ClientPortMin = Value; }

            unsigned short ClientPortMax() { return m_ClientPortMax; }
            void ClientPortMax(unsigned short Value) { m_ClientPortMax = Value; }

            int LastError() { return GetLastError(); }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CSocketHandles --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSocketHandles : public CCollection {
            typedef CCollection inherited;

        protected:

            unsigned short m_DefaultPort;

            CSocketHandle *GetItem(int Index) override;
            void SetItem(int Index, CSocketHandle *Value);

        public:

            CSocketHandles();

            CSocketHandle *Add();

            CSocketHandle *Insert(int Index);

            CSocketHandle *BindingByHandle(CSocket AHandle);

            CSocketHandle *Handles(int Index) { return GetItem(Index); }
            void Handles(int Index, CSocketHandle *Value) { SetItem(Index, Value); }

            unsigned short DefaultPort() { return m_DefaultPort; }
            void DefaultPort(unsigned short Value) { m_DefaultPort = Value; }

            CSocketHandle *operator[] (int Index) override { return Handles(Index); };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CIOHandler ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CIOHandlerSocket;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CIOHandler : public CSocketComponent {
            typedef CSocketComponent inherited;

        public:

            CIOHandler() : CSocketComponent() {};

            ~CIOHandler() override { Close(); };

            virtual void AfterAccept() {};

            virtual void Open() {};

            virtual void Close() {};

            virtual bool Connected() abstract;

            virtual ssize_t Recv(void *ABuf, size_t ALen) abstract;

            virtual ssize_t Send(void *ABuf, size_t ALen) abstract;

        }; // CIOHandler

        //--------------------------------------------------------------------------------------------------------------

        //-- CIOHandlerSocket ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CIOHandlerSocket: public CIOHandler {
            typedef CIOHandler inherited;

        private:

            CSocketHandle *m_Binding;

        public:

            CIOHandlerSocket();

            ~CIOHandlerSocket() override;

            void Open() override;

            void Close() override;

            bool Connected() override;

            ssize_t Recv(void *ABuf, size_t ALen) override;

            ssize_t Send(void *ABuf, size_t ALen) override;

            CSocketHandle *Binding() { return m_Binding; }

        }; // CIOHandlerSocket

        //--------------------------------------------------------------------------------------------------------------

        //-- CPollConnection -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CPollEventHandler;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CPollConnection: public CSocketComponent {
        private:

            CPollEventHandler *m_EventHandler;

            CPollConnection *m_PollConnection;

        protected:

            void ClosePoll();

            void SetEventHandler(CPollEventHandler *AValue);

        public:

            explicit CPollConnection(CPollConnection *APollConnection);

            CPollConnection *PollConnection() { return m_PollConnection; };

            CPollEventHandler *EventHandler() { return m_EventHandler; }

            void EventHandler(CPollEventHandler *Value) { SetEventHandler(Value); }

            virtual void Disconnect() abstract;

        }; // CPollConnection

        //--------------------------------------------------------------------------------------------------------------

        //-- CTCPConnection --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CTCPServer;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CTCPConnection: public CPollConnection {
            friend CListenerThread;

        private:

            CIOHandler *m_IOHandler;
            CIOHandlerSocket *m_Socket;

            CSimpleBuffer *m_WriteBuffer;
            CSimpleBuffer *m_RecvBuffer;

            CManagedBuffer *m_InputBuffer;
            CSimpleBuffer *m_OutputBuffer;

            bool m_ReadLnSplit;
            bool m_ReadLnTimedOut;
            bool m_ClosedGracefully;
            bool m_OEM;

            size_t m_SendBufferSize;
            size_t m_RecvBufferSize;

            int m_WriteBufferThreshhold;

            size_t m_MaxLineLength;
            int m_ReadTimeOut;

            CMaxLineAction m_MaxLineAction;
            CNotifyEvent m_OnDisconnected;

        protected:

            void SetIOHandler(CIOHandler *AValue);

            void DoDisconnected();

        public:

            CTCPConnection();

            ~CTCPConnection() override;

            bool CheckForDisconnect(bool ARaiseExceptionIfDisconnected = true);

            CIOHandlerSocket *Socket() { return m_Socket; }

            CIOHandler *IOHandler() { return m_IOHandler; }
            void IOHandler(CIOHandler *Value) { SetIOHandler(Value); }

            bool ClosedGracefully() { return m_ClosedGracefully; }

            bool OEM() { return m_OEM; }
            void OEM(bool Value) { m_OEM = Value; }

            virtual bool Connected();

            void Disconnect() override;

            void DisconnectSocket();

            void OpenWriteBuffer(int AThreshhold = -1);

            void CloseWriteBuffer();

            void FlushWriteBuffer(ssize_t AByteCount = -1);

            void ClearWriteBuffer();

            void FreeWriteBuffer();

            void FlushOutputBuffer(ssize_t AByteCount = -1);

            void CheckWriteResult(ssize_t AByteCount);

            void WriteBuffer(void *ABuffer, size_t AByteCount, bool AWriteNow = false);

            ssize_t WriteBufferAsync(void *ABuffer, size_t AByteCount);

            bool WriteAsync(ssize_t AByteCount = -1);

            void WriteInteger(int AValue, bool AConvert = true);

            virtual void WriteLn(LPCSTR Format, ...);

            virtual void Write(LPCSTR Format, ...);

            void WriteStream(CStream *AStream, bool AAll = true, bool AWriteByteCount = false, size_t ASize = 0);

            void WriteStrings(CStrings *AValue, bool AWriteLinesCount = false);

            virtual ssize_t CheckReadStack(ssize_t AByteCount);

            ssize_t ReadFromStack(bool ARaiseExceptionIfDisconnected = true, bool ARaiseExceptionOnTimeout = true);

            ssize_t ReadAsync(bool ARaiseExceptionIfDisconnected = true);

            void ReadBuffer(void *ABuffer, size_t AByteCount);

            int ReadInteger(bool AConvert = true);

            size_t ReadLn(char *AStr, char ATerminator = LF, int ATimeout = TimeoutDefault, int AMaxLineLength = -1);

            size_t ReadLnWait(char *AStr, size_t ABytes, int AFailCount = MaxInt);

            void ReadString(char *AStr, size_t ABytes);

            void ReadStrings(CStrings *AValue, int AReadLinesCount = -1);

            bool ReadLnTimedOut() { return m_ReadLnTimedOut; }

            int ReadTimeOut() { return m_ReadTimeOut; }
            void ReadTimeOut(int Value) { m_ReadTimeOut = Value; }

            size_t RecvBufferSize() { return m_RecvBufferSize; }
            void RecvBufferSize(size_t Value) { m_RecvBufferSize = Value; }

            size_t MaxLineLength() { return m_MaxLineLength; }
            void MaxLineLength(size_t Value) { m_MaxLineLength = Value; }

            CMaxLineAction MaxLineAction() { return m_MaxLineAction; }
            void MaxLineAction(CMaxLineAction Value) { m_MaxLineAction = Value; }

            CManagedBuffer *InputBuffer() { return m_InputBuffer; }
            CSimpleBuffer *OutputBuffer() { return m_OutputBuffer; }

            const CNotifyEvent& OnDisconnected() { return m_OnDisconnected; }
            void OnDisconnected(CNotifyEvent && Value) { m_OnDisconnected = Value; }

        }; // CTCPConnection

        //--------------------------------------------------------------------------------------------------------------

        //-- CSocketServer ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSocketServer: public CPersistent {
        private:

            unsigned short GetDefaultPort();
            virtual void SetDefaultPort(unsigned short Value);

        protected:

            CString m_ServerName;

            CString m_AllowedMethods;

            CSocketHandles *m_Bindings;

        public:

            CSocketServer();

            ~CSocketServer() override;

            unsigned short DefaultPort() { return GetDefaultPort(); }
            void DefaultPort(unsigned short Value) { SetDefaultPort(Value); }

            CString& ServerName() { return m_ServerName; };
            const CString& ServerName() const { return m_ServerName; };

            CString& AllowedMethods() { return m_AllowedMethods; };
            const CString& AllowedMethods() const { return m_AllowedMethods; };

            CSocketHandles *Bindings() { return m_Bindings; }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CEventSocketServer ----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CSocketServer *AServer, CTCPServerConnection *AConnection, LPCTSTR AFormat, va_list args)> COnSocketServerDebugEvent;

        typedef std::function<void (CTCPServerConnection *AConnection)> COnSocketServerExecuteEvent;

        typedef std::function<void (CTCPServerConnection *AConnection)> COnSocketServerConnectionEvent;

        typedef std::function<void (CSocketServer *AServer, Exception::Exception *AException)> COnSocketServerListenExceptionEvent;

        typedef std::function<void (CTCPServerConnection *AConnection, Exception::Exception *AException)> COnSocketServerExceptionEvent;

        typedef std::function<void (CSocketServer *AServer, LPCTSTR AData, CTCPServerConnection *AConnection)> COnSocketServerBeforeCommandHandlerEvent;

        typedef std::function<void (CSocketServer *AServer, LPCTSTR AData, CTCPServerConnection *AConnection)> COnSocketServerNoCommandHandlerEvent;

        typedef std::function<void (CSocketServer *AServer, CTCPServerConnection *AConnection)> COnSocketServerAfterCommandHandlerEvent;

        typedef std::function<void (CCommand *ACommand)> COnSocketServerCommandEvent;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CEventSocketServer: public CSocketServer {
            typedef CSocketServer inherited;

            friend CPeerThread;

        protected:

            COnSocketServerDebugEvent m_OnDebug;
            COnSocketServerConnectionEvent m_OnAccessLog;

            COnSocketServerExecuteEvent m_OnExecute;

            CNotifyEvent m_OnConnected;
            CNotifyEvent m_OnDisconnected;

            COnSocketServerExceptionEvent m_OnException;
            COnSocketServerListenExceptionEvent m_OnListenException;

            COnSocketServerBeforeCommandHandlerEvent m_OnBeforeCommandHandler;
            COnSocketServerAfterCommandHandlerEvent m_OnAfterCommandHandler;
            COnSocketServerNoCommandHandlerEvent m_OnNoCommandHandler;

            virtual void DoDebug(CTCPServerConnection *AConnection, LPCTSTR AFormat, ...);

            virtual bool DoExecute(CTCPServerConnection *AConnection) abstract;

            virtual void DoAccessLog(CTCPServerConnection *AConnection);

            virtual void DoConnected(CObject *Sender);
            virtual void DoDisconnected(CObject *Sender);

            virtual void DoException(CTCPServerConnection *AConnection, Exception::Exception *AException);
            virtual void DoListenException(Exception::Exception *AException);

            virtual void DoBeforeCommandHandler(CTCPServerConnection *AConnection, LPCTSTR ALine);
            virtual void DoAfterCommandHandler(CTCPServerConnection *AConnection);
            virtual void DoNoCommandHandler(LPCTSTR AData, CTCPServerConnection *AConnection);

        public:

            CEventSocketServer();

            const COnSocketServerDebugEvent &OnDebug() { return m_OnDebug; }
            void OnDebug(COnSocketServerDebugEvent && Value) { m_OnDebug = Value; }

            const COnSocketServerExecuteEvent &OnExecute() { return m_OnExecute; }
            void OnExecute(COnSocketServerExecuteEvent && Value) { m_OnExecute = Value; }

            const COnSocketServerConnectionEvent &OnAccessLog() { return m_OnAccessLog; }
            void OnAccessLog(COnSocketServerConnectionEvent && Value) { m_OnAccessLog = Value; }

            const CNotifyEvent &OnConnected() { return m_OnConnected; }
            void OnConnected(CNotifyEvent && Value) { m_OnConnected = Value; }

            const CNotifyEvent &OnDisconnected() { return m_OnDisconnected; }
            void OnDisconnected(CNotifyEvent && Value) { m_OnDisconnected = Value; }

            const COnSocketServerExceptionEvent &OnException() { return m_OnException; }
            void OnException(COnSocketServerExceptionEvent && Value) { m_OnException = Value; }

            const COnSocketServerListenExceptionEvent OnListenException() { return m_OnListenException; }
            void OnListenException(COnSocketServerListenExceptionEvent && Value) { m_OnListenException = Value; }

            COnSocketServerBeforeCommandHandlerEvent &OnBeforeCommandHandler() { return m_OnBeforeCommandHandler; }
            void OnBeforeCommandHandler(COnSocketServerBeforeCommandHandlerEvent && Value) { m_OnBeforeCommandHandler = Value; }

            COnSocketServerAfterCommandHandlerEvent &OnAfterCommandHandler() { return m_OnAfterCommandHandler; }
            void OnAfterCommandHandler(COnSocketServerAfterCommandHandlerEvent && Value) { m_OnAfterCommandHandler = Value; }

            const COnSocketServerNoCommandHandlerEvent &OnNoCommandHandler() { return m_OnNoCommandHandler; }
            void OnNoCommandHandler(COnSocketServerNoCommandHandlerEvent && Value) { m_OnNoCommandHandler = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CTCPServerConnection --------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CTCPServerConnection: public CTCPConnection {
            typedef CTCPConnection inherited;

        private:

            CEventSocketServer *m_Server;

        public:

            explicit CTCPServerConnection(CEventSocketServer *AServer);

            ~CTCPServerConnection() override;

            virtual CEventSocketServer *Server() { return m_Server; }

        }; // CTCPServerConnection

        //--------------------------------------------------------------------------------------------------------------

        //-- CServerIOHandler ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CServerIOHandler : public CSocketComponent {
            typedef CSocketComponent inherited;

        public:

            CServerIOHandler() : CSocketComponent() {};

            ~CServerIOHandler() override = default;;

            CIOHandler *Accept(CSocket ASocket, int AFlags);

        }; // CServerIOHandler

        //--------------------------------------------------------------------------------------------------------------

        //-- CUDPServer ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CUDPServer : public CSocketServer {
            typedef CSocketServer inherited;

        private:

            bool m_Active;

        protected:

            void SetActive(bool AValue);

        public:

            CUDPServer();

            ~CUDPServer() override;

            bool Active() { return m_Active; }
            void Active(bool Value) { SetActive(Value); }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CUDPClient ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CUDPClient : public CSocketComponent {
            typedef CSocketHandle inherited;

        private:

            CSocketHandle *m_Socket;
            bool m_Active;

        protected:

            virtual void SetActive(bool AValue);

        public:

            CUDPClient();

            ~CUDPClient() override;

            bool Active() { return m_Active; }
            void Active(bool Value) { SetActive(Value); }

            CSocketHandle *Socket() { return m_Socket; }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CSocketThread ---------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        enum CThreadStopMode {
            smTerminate, smSuspend
        };
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CSocketThread *AThread)> COnSocketThreadNotifyEvent;
        typedef std::function<void (CSocketThread *AThread, Exception::Exception *AException)> COnSocketThreadExceptionEvent;
        typedef std::function<void (CSocketThread *AThread, Pointer AData)> COnSocketThreadSynchronizeEvent;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CSocketThread : public CThread {
            typedef CThread inherited;

        protected:

            pthread_mutex_t m_Lock;
            CThreadStopMode m_StopMode;
            bool m_Stopped;

            COnSocketThreadExceptionEvent m_OnException;
            COnSocketThreadNotifyEvent m_OnStopped;

            virtual void AfterRun() abstract;

            virtual void AfterExecute() abstract;

            virtual void BeforeExecute() abstract;

            virtual void BeforeRun() abstract;

            virtual void Cleanup() {};

            virtual void DoException(Exception::Exception *AException);

            virtual void DoStopped();

            void Execute() override;

            virtual void ExecuteTry();

            bool GetStopped();

            virtual void Run() abstract;

        public:

            explicit CSocketThread(bool ACreateSuspended = true);

            ~CSocketThread() override;

            virtual void Start();

            virtual void Stop();

            void Terminate() override;

            virtual void TerminateAndWaitFor();

            CThreadStopMode StopMode() { return m_StopMode; }
            void StopMode(CThreadStopMode Value) { m_StopMode = Value; }

            bool Stopped() { return GetStopped(); }

            const COnSocketThreadExceptionEvent &OnException() { return m_OnException; }
            void OnException(COnSocketThreadExceptionEvent && Value) { m_OnException = Value; }

            const COnSocketThreadNotifyEvent OnStopped() { return m_OnStopped; }
            void OnStopped(COnSocketThreadNotifyEvent && Value) { m_OnStopped = Value; }

        }; // CSocketThread

        //--------------------------------------------------------------------------------------------------------------

        //-- CPeerThread -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CPeerThread : public CSocketThread {
            typedef CSocketThread inherited;

            friend CListenerThread;

        protected:

            CTCPServerConnection *m_Connection;

            void AfterRun() override;

            void AfterExecute() override {};

            void BeforeRun() override;

            void BeforeExecute() override {};

            void Cleanup() override;

            void Run() override;

        public:

            CTCPServerConnection *Connection() { return m_Connection; }

        }; // CPeerThread

        //--------------------------------------------------------------------------------------------------------------

        //-- CListenerThread -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CListenerThread : public CThread {
            typedef CThread inherited;

        private:

            CTCPServer *m_Server;
            CSocketHandle *m_Binding;

        protected:

            void Execute() override;

        public:

            CListenerThread(CTCPServer *AServer, CSocketHandle *ABinding);

            CTCPServer *Server() { return m_Server; }

            CSocketHandle *Binding() { return m_Binding; }

        }; // CListenerThread

        //--------------------------------------------------------------------------------------------------------------

        //-- CThreadMgr ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CThreadMgr {
        protected:

            CThreadList *m_ActiveThreads;
            CThreadPriority m_ThreadPriority;

        public:

            CThreadMgr();

            ~CThreadMgr();

            virtual CPeerThread *GetThread();

            virtual void ReleaseThread(CPeerThread *AThread) abstract;

            virtual void TerminateThreads();

            CThreadList *ActiveThreads() { return m_ActiveThreads; }

            CThreadPriority ThreadPriority() { return m_ThreadPriority; }
            void ThreadPriority(CThreadPriority Value) { m_ThreadPriority = Value; }

        }; // CThreadMgr

        //--------------------------------------------------------------------------------------------------------------

        //-- CThreadMgrDefault -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CThreadMgrDefault : public CThreadMgr {
            typedef CThreadMgr inherited;

        public:

            CPeerThread *GetThread() override {
              CPeerThread *Result = inherited::GetThread();
              ActiveThreads()->Add(Result);
              return Result;
            };

            void ReleaseThread(CPeerThread *AThread) override {
              if (!IsCurrentThread(AThread)) {
                if (!AThread->Suspended())
                  AThread->TerminateAndWaitFor();
                FreeAndNil(AThread);
              } else {
                AThread->FreeOnTerminate(true);
                AThread->Terminate();
              };

              ActiveThreads()->Remove(AThread);
            };

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CThreadSafeList -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CThreadSafeList : public CThreadList {
            typedef CThreadList inherited;

        public:

            bool IsCountLessThan(int AValue) {
              int Count = 0;

              CList *LList = LockList();
              try {
                Count = LList->Count();
              } catch (...) {
              }
              UnlockList();

              return (Count < AValue);
            };
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCommandHandler -------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCommandHandler : public CCollectionItem {
            typedef CCollectionItem inherited;

            friend CCommand;

        protected:

            char m_CmdDelimiter;

            LPCTSTR m_Command;
            Pointer m_Data;

            bool m_Disconnect;
            bool m_Enabled;

            char m_ParamDelimiter;
            bool m_ParseParams;

            int m_ReplyExceptionCode;

            COnSocketServerCommandEvent m_OnCommand;

        public:

            explicit CCommandHandler(CCollection *ACollection);

            ~CCommandHandler() override = default;

            virtual bool Check(LPCTSTR AData, size_t ALen, CTCPServerConnection *AConnection);

            Pointer Data() { return m_Data; }
            void Data(Pointer Value) { m_Data = Value; }

            char CmdDelimiter() { return m_CmdDelimiter; }
            void CmdDelimiter(char Value) { m_CmdDelimiter = Value; }

            LPCTSTR Command() { return m_Command; }
            void Command(LPCTSTR Value) { m_Command = Value; }

            bool Disconnect() { return m_Disconnect; }
            void Disconnect(bool Value) { m_Disconnect = Value; }

            bool Enabled() { return m_Enabled; }
            void Enabled(bool Value) { m_Enabled = Value; }

            char ParamDelimiter() { return m_ParamDelimiter; }
            void ParamDelimiter(char Value) { m_ParamDelimiter = Value; }

            bool ParseParams() { return m_ParseParams; }
            void ParseParams(bool Value) { m_ParseParams = Value; }

            int ReplyExceptionCode() { return m_ReplyExceptionCode; }
            void ReplyExceptionCode(int Value) { m_ReplyExceptionCode = Value; }

            const COnSocketServerCommandEvent &OnCommand() { return m_OnCommand; }
            void OnCommand(COnSocketServerCommandEvent && Value) { m_OnCommand = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CCommandHandlers ------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCommandHandlers : public CCollection {
            typedef CCollection inherited;
        private:

            CSocketServer *m_Server;

            bool m_EnabledDefault;
            bool m_ParseParamsDefault;

        protected:

            CCommandHandler *GetItem(int AIndex) override;

            void SetItem(int AIndex, const CCommandHandler *AValue);

        public:

            explicit CCommandHandlers(CSocketServer *AServer);

            CSocketServer *Server() { return m_Server; }

            CCommandHandler *Add();

            bool EnabledDefault() { return m_EnabledDefault; }
            void EnabledDefault(bool Value) { m_EnabledDefault = Value; }

            bool ParseParamsDefault() { return m_ParseParamsDefault; }
            void ParseParamsDefault(bool Value) { m_ParseParamsDefault = Value; }

            CCommandHandler *Commands(int Index) { return GetItem(Index); }
            void Commands(int Index, CCommandHandler *Value) { SetItem(Index, Value); }

            CCommandHandler *operator[] (int Index) override { return Commands(Index); };

        }; // CCommandHandlers

        //--------------------------------------------------------------------------------------------------------------

        //-- CCommand --------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CCommand: public CObject {
            friend CCommandHandler;

        protected:

            CTCPServerConnection *m_Connection;
            CCommandHandler *m_CommandHandler;
            CStringList *m_Params;
            bool m_PerformReply;
            LPCTSTR m_RawLine;
            LPCTSTR m_UnparsedParams;

            virtual void DoCommand();

        public:

            explicit CCommand(CCommandHandler *ACommandHandler);

            ~CCommand() override;

            CTCPServerConnection *Connection() { return m_Connection; }

            CCommandHandler *CommandHandler() { return m_CommandHandler; }

            bool PerformReply() { return m_PerformReply; }
            void PerformReply(bool Value) { m_PerformReply = Value; }

            CStringList *Params() { return m_Params; }

            LPCTSTR RawLine() { return m_RawLine; }

            LPCTSTR UnparsedParams() { return m_UnparsedParams; }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CTCPServer ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CTCPServer : public CEventSocketServer {
            typedef CEventSocketServer inherited;

            friend CPeerThread;
            friend CListenerThread;

        private:

            CThreadMgr *m_ThreadMgr;
            CServerIOHandler *m_IOHandler;
            CThreadSafeList *m_Threads;
            CThreadList *m_ListenerThreads;
            CCommandHandlers *m_CommandHandlers;

            bool m_Active;

        protected:

            CThreadMgr *GetThreadMgr();

            void SetActive(bool AValue);

            virtual void SetIOHandler(CServerIOHandler *Value);

            virtual void InitializeCommandHandlers() {};

            void TerminateAllThreads();

            void TerminateListenerThreads();

            bool DoExecute(CTCPServerConnection *AConnection) override;

        public:

            explicit CTCPServer();

            ~CTCPServer() override;

            bool Active() { return m_Active; }
            void Active(bool Value) { SetActive(Value); }

            int IndexOfThreadId(DWORD dwThreadId);

            CPeerThread *FindThread(DWORD dwThreadId);

            CThreadSafeList *Threads() { return m_Threads; }

            CServerIOHandler *IOHandler() { return m_IOHandler; }
            void IOHandler(CServerIOHandler *Value) { SetIOHandler(Value); }

            CThreadMgr *ThreadMgr() { return GetThreadMgr(); }

            CCommandHandlers *CommandHandlers() { return m_CommandHandlers; }
            void CommandHandlers(CCommandHandlers *Value) { m_CommandHandlers = Value; }

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPollStack ------------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPollEventHandler;
        //--------------------------------------------------------------------------------------------------------------

        typedef int CSocketPoll;
        typedef struct epoll_event CPollEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CPollStack: public CObject {
        private:

            CSocketPoll m_Handle;

            CPollEvent *m_EventList;

            size_t m_EventSize;

            int m_TimeOut;

        protected:

            void Ctl(int AOption, CPollEventHandler *AEventHandler);

            CPollEvent *GetEvent(int Index);

        public:

            explicit CPollStack(size_t AEventSize = 512);

            ~CPollStack() override;

            void Assign(CPollStack *Source);

            CSocketPoll Create(int AFlags = 0);

            void Close();

            void Add(CPollEventHandler *AEventHandler);

            void Mod(CPollEventHandler *AEventHandler);

            void Del(CPollEventHandler *AEventHandler);

            int TimeOut() { return m_TimeOut; };
            void TimeOut(int Value) { m_TimeOut = Value; };

            int Wait(const sigset_t *ASigmask = nullptr);

            CPollEvent *EventList() { return m_EventList; };

            size_t EventSize() { return m_EventSize; };

            CPollEvent *Events(int Index) { return GetEvent(Index); };

            CPollEvent *operator[] (int Index) { return Events(Index); };

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPollEventHandler -----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPollEventHandler *AHandler)> COnPollEventHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        enum CPollEvenType { etDisabled, etConnect, etWork, etClosed };
        //--------------------------------------------------------------------------------------------------------------

        class CPollServer;
        class CPollEventHandlers;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CPollEventHandler: public CCollectionItem {
            typedef CCollectionItem inherited;

            friend CPollServer;

        protected:

            CSocket m_Socket;

            CPollEvenType m_EventType;

            CPollConnection *m_PollConnection;

            CPollEventHandlers *m_EventHandlers;

            COnPollEventHandlerEvent m_OnTimeOutEvent;
            COnPollEventHandlerEvent m_OnReadEvent;
            COnPollEventHandlerEvent m_OnWriteEvent;

            void SetEventType(CPollEvenType Value);

            void SetPollConnection(CPollConnection *Value);

            void DoTimeOutEvent();
            void DoReadEvent();
            void DoWriteEvent();

        public:

            explicit CPollEventHandler(CPollEventHandlers *AEventHandlers, CSocket ASocket);

            ~CPollEventHandler() override;

            CSocket Socket() { return m_Socket; }

            CPollConnection *PollConnection() { return m_PollConnection; }
            void PollConnection(CPollConnection *Value) { SetPollConnection(Value); }

            CPollEvenType EventType() { return m_EventType; }
            void EventType (CPollEvenType Value) { SetEventType(Value); }

            void Close() { SetEventType(etClosed); };
            bool Closed() { return m_EventType == etClosed; };

            const COnPollEventHandlerEvent &OnTimeOutEvent() { return m_OnTimeOutEvent; }
            void OnTimeOutEvent(COnPollEventHandlerEvent && Value) { m_OnTimeOutEvent = Value; }

            const COnPollEventHandlerEvent &OnReadEvent() { return m_OnReadEvent; }
            void OnReadEvent(COnPollEventHandlerEvent && Value) { m_OnReadEvent = Value; }

            const COnPollEventHandlerEvent &OnWriteEvent() { return m_OnWriteEvent; }
            void OnWriteEvent(COnPollEventHandlerEvent && Value) { m_OnWriteEvent = Value; }
        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CPollEventHandlers ----------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CPollServer;
        //--------------------------------------------------------------------------------------------------------------

        typedef std::function<void (CPollEventHandler *AHandler, Exception::Exception *AException)> COnPollEventHandlerExceptionEvent;
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CPollEventHandlers: public CCollection {
            typedef CCollection inherited;

            friend CPollEventHandler;

        private:

            CPollStack *m_PollStack;

            COnPollEventHandlerExceptionEvent m_OnException;

        protected:

            CPollEventHandler *GetItem(int AIndex) override;
            void SetItem(int AIndex, CPollEventHandler *AValue);

            void Enable(CPollEventHandler *AHandler);
            void Disable(CPollEventHandler *AHandler);

            void DoException(CPollEventHandler *AHandler, Exception::Exception *AException);

        public:

            explicit CPollEventHandlers(CPollStack *APollStack);

            CPollStack *PollStack() { return m_PollStack; };
            void PollStack(CPollStack *Value) { m_PollStack = Value; };

            CPollEventHandler *Add(CSocket ASocket);

            CPollEventHandler *FindHandlerBySocket(CSocket ASocket);

            CPollEventHandler *Handlers(int Index) { return GetItem(Index); }
            void Handlers(int Index, CPollEventHandler *Value) { SetItem(Index, Value); }

            CPollEventHandler *operator[] (int Index) override { return Handlers(Index); };

            const COnPollEventHandlerExceptionEvent& OnException() { return m_OnException; }
            void OnException(COnPollEventHandlerExceptionEvent && Value) { m_OnException = Value; }

        }; // CCommandHandlers

        //--------------------------------------------------------------------------------------------------------------

        //-- CPollServer -----------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        enum CActiveLevel {alShutDown, alBinding, alActive};
        //--------------------------------------------------------------------------------------------------------------

        class LIB_DELPHI CPollServer: public CEventSocketServer {
            typedef CEventSocketServer inherited;

        private:

            CPollStack *m_PollStack;

            CServerIOHandler *m_IOHandler;

            CCommandHandlers *m_CommandHandlers;

            CPollEventHandlers *m_EventHandlers;

            CActiveLevel m_ActiveLevel;

            COnPollEventHandlerExceptionEvent m_OnEventHandlerException;

            bool m_FreePollStack;

            void SetPollStack(CPollStack *Value);

            void CheckHandler(CPollEventHandler* AHandler);

            void CreatePollEventHandlers();

        protected:

            int GetTimeOut();

            void SetTimeOut(int Value);

            virtual void SetActiveLevel(CActiveLevel AValue);

            virtual void SetIOHandler(CServerIOHandler *Value);

            virtual void InitializeCommandHandlers() {};

            virtual void DoTimeOut(CPollEventHandler *AHandler);

            virtual void DoAccept(CPollEventHandler* AHandler);

            virtual void DoRead(CPollEventHandler* AHandler);

            virtual void DoWrite(CPollEventHandler* AHandler);

            void DoEventHandlersException(CPollEventHandler *AHandler, Exception::Exception *AException);

            bool DoExecute(CTCPServerConnection *AConnection) override;

        public:

            explicit CPollServer();

            ~CPollServer() override;

            CPollStack *PollStack() { return m_PollStack; };
            void PollStack(CPollStack *Value) { SetPollStack(Value); };

            CActiveLevel ActiveLevel() { return m_ActiveLevel; }
            void ActiveLevel(CActiveLevel Value) { SetActiveLevel(Value); }

            int TimeOut() { return GetTimeOut(); };
            void TimeOut(int Value) { SetTimeOut(Value); };

            bool Wait();

            CServerIOHandler *IOHandler() { return m_IOHandler; }
            void IOHandler(CServerIOHandler *Value) { SetIOHandler(Value); }

            CPollEventHandlers *EventHandlers() { return m_EventHandlers; }
            void EventHandlers(CPollEventHandlers *Value) { m_EventHandlers = Value; }

            CCommandHandlers *CommandHandlers() { return m_CommandHandlers; }
            void CommandHandlers(CCommandHandlers *Value) { m_CommandHandlers = Value; }

            const COnPollEventHandlerExceptionEvent &OnEventHandlerException() { return m_OnEventHandlerException; }
            void OnEventHandlerException(COnPollEventHandlerExceptionEvent && Value) { m_OnEventHandlerException = Value; }

        };

    }; // namespace Socket

}; // namespace Delphi

using namespace Delphi::Socket;

#endif // DELPHI_SOCKETS_HPP
