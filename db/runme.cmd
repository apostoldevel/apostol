@ECHO OFF

REM Copyright (c) 2009-2019 ООО "Байт-Энерго"
REM Автор: Преподобный Ален Алексеевич

setlocal

SET CHOICE=%CD%\bin\Choice.exe
SET SCRIPTS=%CD%\scripts

SET SQL=%CD%\sql
SET LOG=%CD%\log

if NOT EXIST %LOG% MD %LOG%

if EXIST %SCRIPTS%\sets.cmd (
  call %SCRIPTS%\sets.cmd
) else (
  echo ОШИБКА: Не найден файл настроек: %SCRIPTS%\sets.cmd
  goto :eof
)

SET LABEL_PGCREATEDB=Имя НОВОЙ (создаваемой) базы данных
SET LABEL_PGDATABASE=Имя базы данных для подключения
SET LABEL_PGUSER=Имя пользователя для подключения
SET LABEL_PGHOST=Имя компьютера для подключения
SET LABEL_PGPORT=TCP-порт для подключения

SET SAVE_PGCREATEDB=%PGCREATEDB%
SET SAVE_PGDATABASE=%PGDATABASE%
SET SAVE_PGUSER=%PGUSER%

:main

  TITLE Менеджер базы данных PostgreSQL.

  cls

  echo   Меню
  echo   ═════════════════════════════════════════════════
  echo.
  echo     1. Создание новой базы данных и ядра...
  echo     2. Создание ядра в уже созданной безе данных...
  echo     3. Создание пустой базы данных...
  echo     4. Обновление базы данных...
  echo.
  echo     Q. Выход.
  echo.

  %CHOICE% /C:1234Q "  Ваш выбор: "

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

  echo   Создание новой базы данных и ядра
  echo   ═════════════════════════════════════════════════
  echo.
  echo     1. %LABEL_PGCREATEDB%: %PGCREATEDB%
  echo.
  echo     2. %LABEL_PGDATABASE%: %PGDATABASE%
  echo     3. %LABEL_PGUSER%: %PGUSER%
  echo.
  echo     4. %LABEL_PGHOST%: %PGHOST%
  echo     5. %LABEL_PGPORT%: %PGPORT%
  echo.
  echo     Y. Подключиться к базе данных %PGDATABASE%,
  echo        чтобы создать базу данных %PGCREATEDB% и ядро.
  echo.
  echo        Ядро будет создано в %PGCREATEDB%.
  echo.
  echo     M. Меню...
  echo     Q. Выход.
  echo.

  %CHOICE% /C:12345YMQ "  Ваш выбор: "

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

  echo   Создание ядра в уже созданной безе данных
  echo   ═════════════════════════════════════════════════
  echo.
  echo     1. %LABEL_PGDATABASE%: %PGDATABASE%
  echo     2. %LABEL_PGUSER%: %PGUSER%
  echo.
  echo     3. %LABEL_PGHOST%: %PGHOST%
  echo     4. %LABEL_PGPORT%: %PGPORT%
  echo.
  echo     Y. Подключиться к базе данных %PGDATABASE% и создать ядро.
  echo.
  echo     M. Меню...
  echo     Q. Выход.
  echo.

  %CHOICE% /C:1234YMQ "  Ваш выбор: "

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

  echo   Создание новой базы данных
  echo   ═════════════════════════════════════════════════
  echo.
  echo     1. %LABEL_PGCREATEDB%: %PGCREATEDB%
  echo.
  echo     2. %LABEL_PGDATABASE%: %PGDATABASE%
  echo     3. %LABEL_PGUSER%: %PGUSER%
  echo.
  echo     4. %LABEL_PGHOST%: %PGHOST%
  echo     5. %LABEL_PGPORT%: %PGPORT%
  echo.
  echo     Y. Подключиться к базе данных %PGDATABASE%,
  echo        чтобы создать базу данных %PGCREATEDB%.
  echo.
  echo        Ядро НЕ будет создано (база данных будет пустой).
  echo.
  echo     M. Меню...
  echo     Q. Выход.
  echo.

  %CHOICE% /C:12345YMQ "  Ваш выбор: "

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
    echo ВНИМАНИЕ: 
    echo.
    echo Если база данных %PGCREATEDB% уже существует то, данная операция приведет к её уничтожению.
    echo Все данные из %PGCREATEDB% будут потеряны!
    echo.
  )

  if "%MAKE_PARAM%" == "/kernel" (
    echo ВНИМАНИЕ: 
    echo.
    echo Все таблицы, функции и прочие данные ядра в базе данных %PGDATABASE% будут пересозданы!
    echo.
  )

  if "%MAKE_PARAM%" == "/create" (
    echo ВНИМАНИЕ: 
    echo.
    echo Если база данных %PGCREATEDB% уже существует то, данная операция приведет к её уничтожению.
    echo Все данные из %PGCREATEDB% будут потеряны!
    echo.
  )

  if "%MAKE_PARAM%" == "/update" (
    echo ВНИМАНИЕ: 
    echo.
    echo База данных %PGDATABASE% будут обновлена!
    echo.
  )

  %CHOICE% /C:YN "Продолжить? "

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
