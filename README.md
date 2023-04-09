[![ru](https://img.shields.io/badge/lang-ru-green.svg)](https://github.com/apostoldevel/apostol/blob/master/README.ru-RU.md)

# Apostol

**Apostol** is an HTTP server with direct access to the [PostgreSQL](https://www.postgresql.org/), with source code in C++.

* The main idea is to connect the HTTP server directly to the database, eliminating intermediary scripts in the form of programming languages from the `HTTP request` processing chain.


* The main advantages:
    * **Autonomy**: After building, you get a fully ready-to-use binary file in the form of a system service (daemon) under Linux OS;
    * **Speed**: Queries to the HTTP server and the database are executed as fast as the operating system and DBMS allow;
    * **Connection pool**: Apostle has its own customizable connection pool with PostgreSQL.
     
DESCRIPTION
-
**Apostol** is implemented as a framework for developing server software (system services) using an asynchronous programming model based on the epoll API with direct access to the [PostgreSQL](https://www.postgresql.org/) (through the library: `libpq`), specifically for highly loaded systems.

MODULES
-

The framework has a modular design, built-in HTTP server, and [PostgreSQL](https://www.postgresql.org/) client.

### This build comes with two modules:

- [WebServer](https://github.com/apostoldevel/module-WebServer) (Web server);
    * Provides the [Swagger UI](https://swagger.io/tools/swagger-ui) which will be available at [http://localhost:8080](http://localhost:8080) in your browser after launching **Apostol**.


- [PGFetch](https://github.com/apostoldevel/module-PGFetch) (Postgres Fetch);
    * Enables receiving and sending `HTTP requests` in `the PL/pgSQL` programming language.
  
### With additional modules, Apostol can be turned into:

- [AuthServer](https://github.com/apostoldevel/module-AuthServer) (Authorization server OAuth 2.0);
- [AppServer](https://github.com/apostoldevel/module-AppServer) (Application server);
- [MessageServer](https://github.com/apostoldevel/process-MessageServer) (Message server: SMTP/FCM/API);
- [FileServer](https://github.com/apostoldevel/module-FileServer) (File server);
- [Replication](https://github.com/apostoldevel/module-Replication) (Replication);
- [StreamServer](https://github.com/apostoldevel/process-StreamServer) (Streaming data server).

Apostol has built-in WebSocket support: [WebSocket API](https://github.com/apostoldevel/module-WebSocketAPI).

Combining all the above, you can create an information system [Apostol CRM](https://github.com/apostoldevel/apostol-crm) or [Central System for Electric Vehicle Charging Stations](https://github.com/apostoldevel/apostol-cs) why not ;-).

_With Apostol your possibilities are only limited by your imagination._
 
Docker
-

Вы можете собрать образ самостоятельно или получить уже готовый из докер-хаб:

### Собрать

~~~
docker build -t apostol .
~~~

### Получить

~~~
docker pull apostoldevel/apostol
~~~

### Запустить

Если собрали самомтоятельно:
~~~
docker run -d -p 8080:8080 --rm --name apostol apostol
~~~

Если получили готовый образ:
~~~
docker run -d -p 8080:8080 --rm --name apostol apostoldevel/apostol
~~~

Swagger UI будет доступен по адресу http://localhost:8080 или http://host-ip:8080 в вашем браузере.

СТРУКТУРА КАТАЛОГОВ
-
    auto/                       содержит файлы со скриптами
    cmake-modules/              содержит файлы с модулями CMake
    conf/                       содержит файлы с настройками
    src/                        содержит файлы с исходным кодом
    ├─app/                      содержит файлы с исходным кодом: Apostol
    ├─core/                     содержит файлы с исходным кодом: Apostol Core
    ├─lib/                      содержит файлы с исходным кодом библиотек
    | └─delphi/                 содержит файлы с исходным кодом библиотеки*: Delphi classes for C++
    └─modules/                  содержит файлы с исходным кодом дополнений (модулей)
    www/                        содержит файлы с Веб-сайтом

СБОРКА И УСТАНОВКА
-
Для установки **Апостол** Вам потребуется:

1. Компилятор C++;
1. [CMake](https://cmake.org) или интегрированная среда разработки (IDE) с поддержкой [CMake](https://cmake.org);
1. Библиотека [libpq-dev](https://www.postgresql.org/download) (libraries and headers for C language frontend development);
1. Библиотека [postgresql-server-dev-all](https://www.postgresql.org/download) (libraries and headers for C language backend development).

### Linux (Debian/Ubuntu)

Для того чтобы установить компилятор C++ и необходимые библиотеки на Ubuntu выполните:
~~~
sudo apt-get install build-essential libssl-dev libcurl4-openssl-dev make cmake gcc g++
~~~

###### Подробное описание установки C++, CMake, IDE и иных компонентов необходимых для сборки проекта не входит в данное руководство.

#### PostgreSQL

Для того чтобы установить PostgreSQL воспользуйтесь инструкцией по [этой](https://www.postgresql.org/download/) ссылке.

#### База данных

Для того чтобы установить базу данных необходимо выполнить:

1. Прописать наименование базы данных в файле db/sql/sets.conf (по умолчанию: web)
1. Прописать пароли для пользователей СУБД [libpq-pgpass](https://postgrespro.ru/docs/postgrespro/13/libpq-pgpass):
   ~~~
   $ sudo -iu postgres -H vim .pgpass
   ~~~
   ~~~
   *:*:*:http:http
   ~~~
1. Указать в файле настроек `/etc/postgresql/{version}/main/pg_hba.conf`:
   ~~~
   # TYPE  DATABASE        USER            ADDRESS                 METHOD
   local	web		http					md5
   ~~~
1. Применить настройки:
   ~~~
   $ sudo pg_ctlcluster <version> main reload
   ~~~   
1. Выполнить:
   ~~~
   $ cd db/
   $ ./install.sh --make
   ~~~

###### Параметр `--make` необходим для установки базы данных в первый раз. Далее установочный скрипт можно запускать или без параметров или с параметром `--install`.

Для установки **Апостол** (без Git) необходимо:

1. Скачать [Апостол](https://github.com/ufocomp/apostol/archive/master.zip);
1. Распаковать;
1. Настроить `CMakeLists.txt` (по необходимости);
1. Собрать и скомпилировать (см. ниже).

Для установки **Апостол** с помощью Git выполните:
~~~
git clone https://github.com/ufocomp/apostol.git
~~~

###### Сборка:
~~~
cd apostol
./configure
~~~

###### Компиляция и установка:
~~~
cd cmake-build-release
make
sudo make install
~~~

По умолчанию бинарный файл `apostol` будет установлен в:
~~~
/usr/sbin
~~~

Файл конфигурации и необходимые для работы файлы, в зависимости от варианта установки, будут расположены в:
~~~
/etc/apostol
или
~/apostol
~~~

ЗАПУСК
-

###### Если `INSTALL_AS_ROOT` установлено в `ON`.

`apostol` - это системная служба (демон) Linux.
Для управления `apostol` используйте стандартные команды управления службами.

Для запуска `apostol` выполните:
~~~
sudo systemctl start apostol
~~~

Для проверки статуса выполните:
~~~
sudo systemctl status apostol
~~~

Результат должен быть **примерно** таким:
~~~
● apostol.service - Apostol
     Loaded: loaded (/etc/systemd/system/apostol.service; enabled; vendor preset: enabled)
     Active: active (running) since Sat 2019-04-06 00:00:00 MSK; 3y ago
    Process: 461158 ExecStartPre=/usr/bin/rm -f /run/apostol.pid (code=exited, status=0/SUCCESS)
    Process: 461160 ExecStartPre=/usr/sbin/apostol -t (code=exited, status=0/SUCCESS)
    Process: 461162 ExecStart=/usr/sbin/apostol (code=exited, status=0/SUCCESS)
   Main PID: 461163 (apostol)
      Tasks: 2 (limit: 77011)
     Memory: 2.6M
        CPU: 44ms
     CGroup: /system.slice/apostol.service
             ├─461163 apostol: master process /usr/sbin/apostol
             └─461164 apostol: worker process ("pq fetch", "web server")
~~~

УПРАВЛЕНИЕ
-

Управлять `apostol` можно с помощью сигналов.
Номер главного процесса по умолчанию записывается в файл `/run/apostol.pid`.
Изменить имя этого файла можно при конфигурации сборки или же в `apostol.conf` секция `[daemon]` ключ `pid`.

Главный процесс поддерживает следующие сигналы:

|Сигнал   |Действие          |
|---------|------------------|
|TERM, INT|быстрое завершение|
|QUIT     |плавное завершение|
|HUP	  |изменение конфигурации, запуск новых рабочих процессов с новой конфигурацией, плавное завершение старых рабочих процессов|
|WINCH    |плавное завершение рабочих процессов|	

Управлять рабочими процессами по отдельности не нужно. Тем не менее, они тоже поддерживают некоторые сигналы:

|Сигнал   |Действие          |
|---------|------------------|
|TERM, INT|быстрое завершение|
|QUIT	  |плавное завершение|