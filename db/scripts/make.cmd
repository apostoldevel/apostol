@ECHO OFF

REM Copyright (c) 2009-2017 ООО "Байт-Энерго"
REM Автор: Преподобный Ален Алексеевич

if "%1" == "" goto help
if "%1" == "/?" goto help

if EXIST %CD%\sets.cmd (
  if NOT DEFINED PGDATABASE call %CD%\sets.cmd
) else (
  echo ОШИБКА: Не найден файл настроек: %CD%\sets.cmd
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

SET MSG1=База данных которую вы создаёте недолжна совпадать с базой данных к которой вы подключаетесь
SET MSG2=Недопустимо создание объектов в базе данных postgres
SET MSG3=Создание объектов в базе данных %PGDATABASE% недопустимо при установленной переменной PGCREATEDB

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

      echo ВНИМАНИЕ: %MSG1%.
      echo \echo %MSG1%>>%MAKE%
    )

    echo \ir kernel.sql>>%MAKE%

  ) else (

    echo ВНИМАНИЕ: %MSG2%.
    echo \echo %MSG2%>>%MAKE%
  )

  echo \q>>%MAKE%

  call psql.cmd %MAKE% UTF8

goto eof

:kernel

  if DEFINED PGCREATEDB (
    echo ОШИБКА: %MSG3%.
    goto eof
  )

  echo \echo kernel.conf>%KERNEL%

  if NOT "%PGDATABASE%" == "postgres" (

    echo \set dbname %PGDATABASE%>%SQL%\dbname.conf

    echo \ir dbname.conf>>%KERNEL%
    echo \ir kernel.sql>>%KERNEL%

  ) else (

    echo ВНИМАНИЕ: %MSG2%.
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

      echo ВНИМАНИЕ: %MSG1%.
      echo \echo %MSG1%>>%CREATE%
    )

  ) else (

    echo ВНИМАНИЕ: %MSG2%.
    echo \echo %MSG2%>>%CREATE%
  )

  echo \q>>%CREATE%

  call psql.cmd %CREATE% UTF8

goto eof

:update

  if DEFINED PGCREATEDB (
    echo ОШИБКА: %MSG3%.
    goto eof
  )

  echo \echo path.conf>%UPDATE%

  if NOT "%PGDATABASE%" == "postgres" (

    echo \set dbname %PGDATABASE%>%SQL%\dbname.conf

    echo \ir dbname.conf>>%UPDATE%
    echo \ir path.sql>>%UPDATE%

  ) else (

    echo ВНИМАНИЕ: %MSG2%.
    echo \echo %MSG2%>>%UPDATE%
  )

  echo \q>>%UPDATE%

  call psql.cmd %UPDATE% UTF8

goto eof

:error

  echo ОШИБКА: Не установлена переменная среды окружения %1, продолжение невозможно.

goto :eof

:help

  echo [H] Copyright (c) 2009-2017 ООО "Байт-Энерго"
  echo [H] 
  echo [H] Создание базы данных в PostgreSQL.
  echo [H] 
  echo [H] Использование: %~nx0 /{параметр}
  echo [H] 
  echo [H] Где {параметр}:
  echo [H] 
  echo [H]   make	- Создание: Самой базы данных (согласно PGCREATEDB), пользователей (kernel, daemon и admin),
  echo [H]		  таблиц, функций и прочего.
  echo [H] 
  echo [H]		  В sets.cmd должна быть определена PGDATABASE (используется для подключения), по умолчению postgres.
  echo [H]		  В sets.cmd должна быть определена PGCREATEDB (все изменения происходят тут), по умолчению bedb.
  echo [H] 
  echo [H]   kernel	- Создание в базе данных (согласно PGDATABASE): только таблиц, функций и прочего.
  echo [H] 
  echo [H]		  В sets.cmd должна быть определена PGDATABASE.
  echo [H]		  В sets.cmd НЕ должна быть определена PGCREATEDB.
  echo [H] 
  echo [H] Запуск без параметров приведет к выводу этого сообщения.

goto :eof

:eof

goto :eof
