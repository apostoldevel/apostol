@ECHO OFF

REM SET PGPATH=D:\PostgreSQL\10
REM SET PGDATA=%PGPATH%\data
REM SET PGLOCALEDIR=%PGPATH%\share\locale

REM Кодировка, используемая клиентом (WIN1251)
SET PGCLIENTENCODING=WIN1251

REM Имя компьютера для подключения (localhost)
SET PGHOST=localhost

REM TCP-порт для подключения (5432)
SET PGPORT=5432

REM Имя базы данных для подключения (postgres)
SET PGDATABASE=postgres

REM Имя пользователя для подключения (postgres)
SET PGUSER=postgres

REM Пароль пользователя для подключения ()
SET PGPASSWORD=PostgreSQL

REM Имя НОВОЙ (создаваемой) базы данных (bedb)
SET PGCREATEDB=client365

REM SET PGCREATEDB=
REM SET PGDATABASE=client365
REM SET PGUSER=kernel
REM SET PGPASSWORD=
