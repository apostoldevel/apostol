@ECHO OFF

REM Copyright (c) 2009-2019 ��� "����-���࣮"
REM ����: �९������ ���� ����ᥥ���

setlocal

SET CHOICE=%CD%\bin\Choice.exe
SET SCRIPTS=%CD%\scripts

SET SQL=%CD%\sql
SET LOG=%CD%\log

if NOT EXIST %LOG% MD %LOG%

if EXIST %SCRIPTS%\sets.cmd (
  call %SCRIPTS%\sets.cmd
) else (
  echo ������: �� ������ 䠩� ����஥�: %SCRIPTS%\sets.cmd
  goto :eof
)

SET LABEL_PGCREATEDB=��� ����� (ᮧ��������) ���� ������
SET LABEL_PGDATABASE=��� ���� ������ ��� ������祭��
SET LABEL_PGUSER=��� ���짮��⥫� ��� ������祭��
SET LABEL_PGHOST=��� �������� ��� ������祭��
SET LABEL_PGPORT=TCP-���� ��� ������祭��

SET SAVE_PGCREATEDB=%PGCREATEDB%
SET SAVE_PGDATABASE=%PGDATABASE%
SET SAVE_PGUSER=%PGUSER%

:main

  TITLE �������� ���� ������ PostgreSQL.

  cls

  echo   ����
  echo   �������������������������������������������������
  echo.
  echo     1. �������� ����� ���� ������ � ��...
  echo     2. �������� �� � 㦥 ᮧ������ ���� ������...
  echo     3. �������� ���⮩ ���� ������...
  echo     4. ���������� ���� ������...
  echo.
  echo     Q. ��室.
  echo.

  %CHOICE% /C:1234Q "  ��� �롮�: "

  if errorlevel 5 goto end
  if errorlevel 4 goto update
  if errorlevel 3 goto create
  if errorlevel 2 goto kernel
  if errorlevel 1 goto install

goto end

:install

  if NOT "%SAVE_PGCREATEDB%" == "%PGCREATEDB%" SET PGCREATEDB=%SAVE_PGCREATEDB%
  if NOT "%SAVE_PGDATABASE%" == "%PGDATABASE%" SET PGDATABASE=%SAVE_PGDATABASE%
  if NOT "%SAVE_PGUSER%" == "%PGUSER%" SET PGUSER=%SAVE_PGUSER%

  SET MAKE_PARAM=/make

  cls

  echo   �������� ����� ���� ������ � ��
  echo   �������������������������������������������������
  echo.
  echo     1. %LABEL_PGCREATEDB%: %PGCREATEDB%
  echo.
  echo     2. %LABEL_PGDATABASE%: %PGDATABASE%
  echo     3. %LABEL_PGUSER%: %PGUSER%
  echo.
  echo     4. %LABEL_PGHOST%: %PGHOST%
  echo     5. %LABEL_PGPORT%: %PGPORT%
  echo.
  echo     Y. ����������� � ���� ������ %PGDATABASE%,
  echo        �⮡� ᮧ���� ���� ������ %PGCREATEDB% � ��.
  echo.
  echo        ��� �㤥� ᮧ���� � %PGCREATEDB%.
  echo.
  echo     M. ����...
  echo     Q. ��室.
  echo.

  %CHOICE% /C:12345YMQ "  ��� �롮�: "

  if errorlevel 8 goto end
  if errorlevel 7 goto main
  if errorlevel 6 goto make
  if errorlevel 5 goto pgport
  if errorlevel 4 goto pghost
  if errorlevel 3 goto pguser
  if errorlevel 2 goto pgdatabase
  if errorlevel 1 goto pgcreatedb

goto end

:kernel

  SET MAKE_PARAM=/kernel

  if DEFINED PGCREATEDB (
    if "%PGDATABASE%" == "postgres" SET PGDATABASE=%PGCREATEDB%
    SET PGCREATEDB=
  )

  if "%PGUSER%" == "postgres" SET PGUSER=kernel

  cls

  echo   �������� �� � 㦥 ᮧ������ ���� ������
  echo   �������������������������������������������������
  echo.
  echo     1. %LABEL_PGDATABASE%: %PGDATABASE%
  echo     2. %LABEL_PGUSER%: %PGUSER%
  echo.
  echo     3. %LABEL_PGHOST%: %PGHOST%
  echo     4. %LABEL_PGPORT%: %PGPORT%
  echo.
  echo     Y. ����������� � ���� ������ %PGDATABASE% � ᮧ���� ��.
  echo.
  echo     M. ����...
  echo     Q. ��室.
  echo.

  %CHOICE% /C:1234YMQ "  ��� �롮�: "

  if errorlevel 7 goto end
  if errorlevel 6 goto main
  if errorlevel 5 goto make
  if errorlevel 4 goto pgport
  if errorlevel 3 goto pghost
  if errorlevel 2 goto pguser
  if errorlevel 1 goto pgdatabase

goto end

:create

  if NOT "%SAVE_PGCREATEDB%" == "%PGCREATEDB%" SET PGCREATEDB=%SAVE_PGCREATEDB%
  if NOT "%SAVE_PGDATABASE%" == "%PGDATABASE%" SET PGDATABASE=%SAVE_PGDATABASE%
  if NOT "%SAVE_PGUSER%" == "%PGUSER%" SET PGUSER=%SAVE_PGUSER%

  SET MAKE_PARAM=/create

  cls

  echo   �������� ����� ���� ������
  echo   �������������������������������������������������
  echo.
  echo     1. %LABEL_PGCREATEDB%: %PGCREATEDB%
  echo.
  echo     2. %LABEL_PGDATABASE%: %PGDATABASE%
  echo     3. %LABEL_PGUSER%: %PGUSER%
  echo.
  echo     4. %LABEL_PGHOST%: %PGHOST%
  echo     5. %LABEL_PGPORT%: %PGPORT%
  echo.
  echo     Y. ����������� � ���� ������ %PGDATABASE%,
  echo        �⮡� ᮧ���� ���� ������ %PGCREATEDB%.
  echo.
  echo        ��� �� �㤥� ᮧ���� (���� ������ �㤥� ���⮩).
  echo.
  echo     M. ����...
  echo     Q. ��室.
  echo.

  %CHOICE% /C:12345YMQ "  ��� �롮�: "

  if errorlevel 8 goto end
  if errorlevel 7 goto main
  if errorlevel 6 goto make
  if errorlevel 5 goto pgport
  if errorlevel 4 goto pghost
  if errorlevel 3 goto pguser
  if errorlevel 2 goto pgdatabase
  if errorlevel 1 goto pgcreatedb

goto make

:update

  SET MAKE_PARAM=/update

  if DEFINED PGCREATEDB (
    if "%PGDATABASE%" == "postgres" SET PGDATABASE=%PGCREATEDB%
    SET PGCREATEDB=
  )

  if "%PGUSER%" == "postgres" SET PGUSER=kernel

goto make

:make

  if "%MAKE_PARAM%" == "/make" (
    echo ��������: 
    echo.
    echo �᫨ ���� ������ %PGCREATEDB% 㦥 ������� �, ������ ������ �ਢ���� � �� 㭨�⮦����.
    echo �� ����� �� %PGCREATEDB% ���� ������!
    echo.
  )

  if "%MAKE_PARAM%" == "/kernel" (
    echo ��������: 
    echo.
    echo �� ⠡����, �㭪樨 � ��稥 ����� �� � ���� ������ %PGDATABASE% ���� ���ᮧ����!
    echo.
  )

  if "%MAKE_PARAM%" == "/create" (
    echo ��������: 
    echo.
    echo �᫨ ���� ������ %PGCREATEDB% 㦥 ������� �, ������ ������ �ਢ���� � �� 㭨�⮦����.
    echo �� ����� �� %PGCREATEDB% ���� ������!
    echo.
  )

  if "%MAKE_PARAM%" == "/update" (
    echo ��������: 
    echo.
    echo ���� ������ %PGDATABASE% ���� ���������!
    echo.
  )

  %CHOICE% /C:YN "�த������? "

  if errorlevel 2 goto main
  if errorlevel 1 (
    pushd %SCRIPTS%
    call make.cmd %MAKE_PARAM%
    popd
  )

goto end

:pgcreatedb

  cls
  SET /P PGCREATEDB=%LABEL_PGCREATEDB% [%PGCREATEDB%]:

goto install

:pgdatabase

  cls
  SET /P PGDATABASE=%LABEL_PGDATABASE% [%PGDATABASE%]:

goto install

:pguser

  cls
  SET /P PGUSER=%LABEL_PGUSER% [%PGUSER%]:

goto install

:pghost

  cls
  SET /P PGHOST=%LABEL_PGHOST% [%PGHOST%]:

goto install

:pgport

  cls
  SET /P PGPORT=%LABEL_PGPORT% [%PGPORT%]:

goto install

:end

  endlocal
  
  exit
