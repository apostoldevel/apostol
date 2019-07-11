/*++

Library name:

  apostol-core

Module Name:

  Module.hpp

Notices:

  Apostol Core

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#ifndef APOSTOL_MODULE_HPP
#define APOSTOL_MODULE_HPP

extern "C++" {

namespace Apostol {

    namespace Module {

        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CApostolModule: public CObject, public CGlobalComponent {
        private:

            bool m_bFreeAfterExecute;

        protected:

            virtual void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) abstract;
            virtual void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) abstract;

        public:

            CApostolModule(): CObject(), CGlobalComponent(), m_bFreeAfterExecute(true) {};

            ~CApostolModule() override = default;

            virtual void Execute(CHTTPConnection *AConnection) abstract;

            CPQPollQuery *GetQuery(CPollConnection *AConnection);

            bool FreeAfterExecute() { return m_bFreeAfterExecute; };
            void FreeAfterExecute(bool Value) { m_bFreeAfterExecute = Value; };

        };

    }
}

using namespace Apostol::Module;
}
//----------------------------------------------------------------------------------------------------------------------

#endif //APOSTOL_MODULE_HPP
