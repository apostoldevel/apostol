@ECHO OFF

REM SET PGPATH=D:\PostgreSQL\10
REM SET PGDATA=%PGPATH%\data
REM SET PGLOCALEDIR=%PGPATH%\share\locale

REM ����஢��, �ᯮ��㥬�� �����⮬ (WIN1251)
SET PGCLIENTENCODING=WIN1251

REM ��� �������� ��� ������祭�� (localhost)
SET PGHOST=localhost

REM TCP-���� ��� ������祭�� (5432)
SET PGPORT=5432

REM ��� ���� ������ ��� ������祭�� (postgres)
SET PGDATABASE=postgres

REM ��� ���짮��⥫� ��� ������祭�� (postgres)
SET PGUSER=postgres

REM ��஫� ���짮��⥫� ��� ������祭�� ()
SET PGPASSWORD=PostgreSQL

REM ��� ����� (ᮧ��������) ���� ������ (bedb)
SET PGCREATEDB=client365

REM SET PGCREATEDB=
REM SET PGDATABASE=client365
REM SET PGUSER=kernel
REM SET PGPASSWORD=
