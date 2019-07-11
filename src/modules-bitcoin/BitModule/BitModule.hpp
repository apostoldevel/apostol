/*++

Library name:

  modules-bitcoin

Module Name:

  BitModule.hpp

Notices:

  Apostol Bitcoin (BitModule)

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_BITMODULE_HPP
#define APOSTOL_BITMODULE_HPP
//----------------------------------------------------------------------------------------------------------------------

extern "C++" {

namespace Apostol {

    namespace BitModule {

        typedef std::function<void (CHTTPConnection *AConnection)> COnHeaderHandlerEvent;
        //--------------------------------------------------------------------------------------------------------------

        class CHeaderHandler: CObject {
        private:

            bool m_Allow;
            COnHeaderHandlerEvent m_Handler;

        public:

            CHeaderHandler(bool Allow, COnHeaderHandlerEvent && Handler): CObject(),
                m_Allow(Allow), m_Handler(Handler) {};

            bool Allow() { return m_Allow; };

            void Handler(CHTTPConnection *AConnection) {
                if (m_Allow && m_Handler)
                    m_Handler(AConnection);
            }
        };
        //--------------------------------------------------------------------------------------------------------------

        class CBitModule: public CApostolModule {
        private:

            CStringList *m_Headers;

            CString m_AllowedMethods;

            void DoOptions(CHTTPConnection *AConnection);
            void DoGet(CHTTPConnection *AConnection);
            void MethodNotAllowed(CHTTPConnection *AConnection);

        protected:

            void InitHeaders();

            const CString& GetAllowedMethods();

            void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) override {};
            void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) override {};

        public:

            CBitModule();

            ~CBitModule() override;

            static class CBitModule *CreateModule() {
                return new CBitModule();
            }

            void Execute(CHTTPConnection *AConnection) override;

            const CString& AllowedMethods() { return GetAllowedMethods(); };

        };

    }
}

using namespace Apostol::BitModule;
}
#endif //APOSTOL_BITMODULE_HPP
