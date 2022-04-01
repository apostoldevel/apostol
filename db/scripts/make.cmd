@ECHO OFF

REM Copyright (c) 2009-2017 ��� "����-���࣮"
REM ����: �९������ ���� ����ᥥ���

if "%1" == "" goto help
if "%1" == "/?" goto help

if EXIST %CD%\sets.cmd (
  if NOT DEFINED PGDATABASE call %CD%\sets.cmd
) else (
  echo ������: �� ������ 䠩� ����஥�: %CD%\sets.cmd
  goto :eof
)

if NOT DEFINED PGDATABASE (
  call :error PGDATABASE
  goto eof
)

if NOT DEFINED SQL (
  for %%I in (%CD%) do SET SQL=%%~dpIsql
)

SET MAKE=%SQL%\make.conf
SET KERNEL=%SQL%\kernel.conf
SET CREATE=%SQL%\createdb.conf
SET UPDATE=%SQL%\path.conf

SET MSG1=���� ������ ������ �� ᮧ���� �������� ᮢ������ � ����� ������ � ���ன �� ������砥���
SET MSG2=�������⨬� ᮧ����� ��ꥪ⮢ � ���� ������ postgres
SET MSG3=�������� ��ꥪ⮢ � ���� ������ %PGDATABASE% �������⨬� �� ��⠭�������� ��६����� PGCREATEDB

echo \echo dbname.conf>%SQL%\dbname.conf

if "%1" == "/make" goto make
if "%1" == "/kernel" goto kernel
if "%1" == "/create" goto create
if "%1" == "/update" goto update

goto help

:make

  if NOT DEFINED PGCREATEDB (
    call :error PGCREATEDB
    goto eof
  )

  echo \set dbname %PGCREATEDB%>%SQL%\dbname.conf

  echo \echo make.conf>%MAKE%
  echo \encoding UTF8>>%MAKE%

  if NOT "%PGCREATEDB%" == "postgres" (

    echo \ir dbname.conf>>%MAKE%

    if NOT "%PGCREATEDB%" == "%PGDATABASE%" (

      echo \ir make.sql>>%MAKE%

    ) else (

      echo ��������: %MSG1%.
      echo \echo %MSG1%>>%MAKE%
    )

    echo \ir kernel.sql>>%MAKE%

  ) else (

    echo ��������: %MSG2%.
    echo \echo %MSG2%>>%MAKE%
  )

  echo \q>>%MAKE%

  call psql.cmd %MAKE% UTF8

goto eof

:kernel

  if DEFINED PGCREATEDB (
    echo ������: %MSG3%.
    goto eof
  )

  echo \echo kernel.conf>%KERNEL%

  if NOT "%PGDATABASE%" == "postgres" (

    echo \set dbname %PGDATABASE%>%SQL%\dbname.conf

    echo \ir dbname.conf>>%KERNEL%
    echo \ir kernel.sql>>%KERNEL%

  ) else (

    echo ��������: %MSG2%.
    echo \echo %MSG2%>>%KERNEL%
  )

  echo \q>>%KERNEL%

  call psql.cmd %KERNEL% UTF8

goto eof

:create

  if NOT DEFINED PGCREATEDB (
    call :error PGCREATEDB
    goto eof
  )

  echo \set dbname %PGCREATEDB%>%SQL%\dbname.conf

  echo \echo createdb.conf>%CREATE%
  echo \encoding UTF8>>%CREATE%

  if NOT "%PGCREATEDB%" == "postgres" (

    echo \ir dbname.conf>>%CREATE%

    if NOT "%PGCREATEDB%" == "%PGDATABASE%" (

      echo \ir createdb.sql>>%CREATE%

    ) else (

      echo ��������: %MSG1%.
      echo \echo %MSG1%>>%CREATE%
    )

  ) else (

    echo ��������: %MSG2%.
    echo \echo %MSG2%>>%CREATE%
  )

  echo \q>>%CREATE%

  call psql.cmd %CREATE% UTF8

goto eof

:update

  if DEFINED PGCREATEDB (
    echo ������: %MSG3%.
    goto eof
  )

  echo \echo path.conf>%UPDATE%

  if NOT "%PGDATABASE%" == "postgres" (

    echo \set dbname %PGDATABASE%>%SQL%\dbname.conf

    echo \ir dbname.conf>>%UPDATE%
    echo \ir path.sql>>%UPDATE%

  ) else (

    echo ��������: %MSG2%.
    echo \echo %MSG2%>>%UPDATE%
  )

  echo \q>>%UPDATE%

  call psql.cmd %UPDATE% UTF8

goto eof

:error

  echo ������: �� ��⠭������ ��६����� �।� ���㦥��� %1, �த������� ����������.

goto :eof

:help

  echo [H] Copyright (c) 2009-2017 ��� "����-���࣮"
  echo [H] 
  echo [H] �������� ���� ������ � PostgreSQL.
  echo [H] 
  echo [H] �ᯮ�짮�����: %~nx0 /{��ࠬ���}
  echo [H] 
  echo [H] ��� {��ࠬ���}:
  echo [H] 
  echo [H]   make	- ��������: ����� ���� ������ (ᮣ��᭮ PGCREATEDB), ���짮��⥫�� (kernel, daemon � admin),
  echo [H]		  ⠡���, �㭪権 � ��祣�.
  echo [H] 
  echo [H]		  � sets.cmd ������ ���� ��।����� PGDATABASE (�ᯮ������ ��� ������祭��), �� 㬮�祭�� postgres.
  echo [H]		  � sets.cmd ������ ���� ��।����� PGCREATEDB (�� ��������� �ந�室�� ���), �� 㬮�祭�� bedb.
  echo [H] 
  echo [H]   kernel	- �������� � ���� ������ (ᮣ��᭮ PGDATABASE): ⮫쪮 ⠡���, �㭪権 � ��祣�.
  echo [H] 
  echo [H]		  � sets.cmd ������ ���� ��।����� PGDATABASE.
  echo [H]		  � sets.cmd �� ������ ���� ��।����� PGCREATEDB.
  echo [H] 
  echo [H] ����� ��� ��ࠬ��஢ �ਢ���� � �뢮�� �⮣� ᮮ�饭��.

goto :eof

:eof

goto :eof
