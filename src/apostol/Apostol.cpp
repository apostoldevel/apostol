/*++

Programm name:

  Apostol

Module Name:

  Apostol.cpp

Notices:

  Apostol Web Service

Author:

  Copyright (c) Prepodobny Alen

  mailto: alienufo@inbox.ru
  mailto: ufocomp@gmail.com

--*/

#include "Apostol.hpp"
//----------------------------------------------------------------------------------------------------------------------

#define exit_failure(msg) {                                 \
  if (GLog != nullptr)                                      \
    GLog->Error(LOG_EMERG, 0, msg);                         \
  else                                                      \
    std::cerr << AWS_NAME << ": " << (msg) << std::endl;    \
  exitcode = EXIT_FAILURE;                                  \
}
//----------------------------------------------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

    int exitcode;

    DefaultLocale.SetLocale(setlocale(LC_ALL, ""));

    try
    {
        Application = CApplication::Create(argc, argv);
        Application->Run();
        exitcode = Application->ExitCode();
    }
    catch (std::exception& e)
    {
        exit_failure(e.what());
    }
    catch (...)
    {
        exit_failure("Unknown error...");
    }

    Application->Destroy();

    exit(exitcode);
}
//----------------------------------------------------------------------------------------------------------------------
