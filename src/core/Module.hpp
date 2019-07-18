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

        class CModuleManager;

        //--------------------------------------------------------------------------------------------------------------

        //-- CApostolModule --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CApostolModule: public CCollectionItem, public CGlobalComponent {
        protected:

            virtual void DoPostgresQueryExecuted(CPQPollQuery *APollQuery) abstract;
            virtual void DoPostgresQueryException(CPQPollQuery *APollQuery, Delphi::Exception::Exception *AException) abstract;

        public:

            CApostolModule();

            explicit CApostolModule(CModuleManager *AManager);

            ~CApostolModule() override = default;

            virtual bool CheckUrerArent(const CString& Value) abstract;

            virtual void Execute(CHTTPServerConnection *AConnection) abstract;

            CPQPollQuery *GetQuery(CPollConnection *AConnection);

        };

        //--------------------------------------------------------------------------------------------------------------

        //-- CModuleManager --------------------------------------------------------------------------------------------

        //--------------------------------------------------------------------------------------------------------------

        class CModuleManager: public CCollection {
            typedef CCollection inherited;

        protected:

            virtual void DoBeforeExecuteModule(CApostolModule *AModule) abstract;
            virtual void DoAfterExecuteModule(CApostolModule *AModule) abstract;

        public:

            explicit CModuleManager(): CCollection(this) {

            };

            bool ExecuteModule(CHTTPServerConnection *AConnection);

            int ModuleCount() { return inherited::Count(); };
            void DeleteModule(int Index) { inherited::Delete(Index); };

            CApostolModule *Modules(int Index) { return (CApostolModule *) inherited::GetItem(Index); }
            void Modules(int Index, CApostolModule *Value) { inherited::SetItem(Index, (CCollectionItem *) Value); }
        };

    }
}

using namespace Apostol::Module;
}
//----------------------------------------------------------------------------------------------------------------------

#endif //APOSTOL_MODULE_HPP
