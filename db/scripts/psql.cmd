@echo off

if EXIST %CD%\sets.cmd (
  if NOT DEFINED PGCREATEDB call %CD%\sets.cmd
)

if NOT DEFINED PGPASSFILE (
  SET PGPASSFILE=%CD%\pgpass.conf
  call :pgpass 
) 

if NOT DEFINED LOG (
  for %%I in (%CD%) do SET LOG=%%~dpIlog
)

if NOT "%2" == "" (
  SET PGCLIENTENCODING=%2
)

call :psql %1

goto eof

:pgpass

  for %%I in (%PGPASSFILE%) do SET PGPF=%%~dpI

  if EXIST %PGPASSFILE% (
    if "%PGPF%" == "%CD%\" DEL %PGPASSFILE%
  )

  if NOT DEFINED PGHOST SET PGHOST=*
  if NOT DEFINED PGPORT SET PGPORT=*

  SET PGDB=*

  if DEFINED PGCREATEDB (
    SET PGDB=%PGCREATEDB%
  ) else SET PGDB=%PGDATABASE%

  if DEFINED PGPASSWORD (
    echo %PGHOST%:%PGPORT%:%PGDATABASE%:%PGUSER%:%PGPASSWORD%>%PGPASSFILE%
    echo %PGHOST%:%PGPORT%:%PGDB%:%PGUSER%:%PGPASSWORD%>>%PGPASSFILE%
    SET PGPASSWORD=
  )

  echo %PGHOST%:%PGPORT%:%PGDB%:kernel:kernel>>%PGPASSFILE%
  echo %PGHOST%:%PGPORT%:%PGDB%:daemon:daemon>>%PGPASSFILE%
  echo %PGHOST%:%PGPORT%:%PGDB%:admin:admin>>%PGPASSFILE%

  if "%PGHOST%" == "*" SET PGHOST=
  if "%PGPORT%" == "*" SET PGPORT=

  SET PGDB=

goto :eof


:psql

  if "%1" == "" (

    chcp 1251
    psql.exe postgresql://%PGUSER%@%PGHOST%:%PGPORT%/%PGDATABASE%
    chcp 866

  ) else (

    if EXIST "%1" (
      chcp 1251
      psql.exe -b -L %LOG%\psql.log -f "%1" 2>%LOG%\%~n1.log
      chcp 866
    ) else echo ОШИБКА: Не найден файл %1

  )

goto :eof

:eof

goto :eof